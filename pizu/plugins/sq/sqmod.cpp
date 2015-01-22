/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/
#include <matypes.h>
#include <assert.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utime.h>
#include <pthread.h>
#include <limits.h>
#include <dlfcn.h>
#include <strutils.h>
#include "globalfoos.h"
#include "sqmod.h"


static kchar HTTP_200[] = "HTTP/1.1 200 OK\r\n"
                               "Connection: close\r\n"
                               "%s\r\n"
                               "Server: mariux/1.0\r\n\r\n";

static kchar HTTP_404[] = "HTTP/1.1 404 Not Found\r\n"
                               "Connection: close\r\n"
                               "Content-Type: text/html; charset=utf-8\r\n\r\n"
                               "Server: mariux/1.0\r\n\r\n"
                               "<H2>404 %s Not Found!</H2>\r\n";

static char __thread     sHTTP_ERR[512];
static inline kchar* HTTP_ERR(kchar* _chr)
{
    sprintf(sHTTP_ERR,HTTP_404,_chr);
    return sHTTP_ERR;
}
//----------------------------------------------------------
__thread SqMod*    __pSQ = 0; //this is one per thread.
__thread int       __cnt = 0; //this is one per thread.
//static   mutex     __mut;

static void webPrint(const SQChar* str)
{
    if(__pSQ)
    {
        __pSQ->swrite("\n<pre><font color='red'>\n");
        __pSQ->swrite(str);
        __pSQ->swrite("\n</font></pre>\n");
    }
    else
    {
        std::cout << str;
    }
}


IModule*  SqMod::get_threaded_singleton(kchar* cookie)
{
    //AutoLock __l(&__mut);
    if(0 == __pSQ)
    {
        __pSQ = new SqMod(cookie);
    }
    ++__cnt;
    return __pSQ;
}

void SqMod::release_threaded_singleton(IModule* pm, kchar* cookie)
{
    //AutoLock __l(&__mut);
    --__cnt;
    if(0 == __cnt)
    {
        delete __pSQ;
    }
}

SqMod::SqMod(kchar* cook):IModule(cook),_header_sent(false)
{
    _thread_id = pthread_self();
    _penv = new SqEnv();//

    regGlobals(_penv->theVM());

    SqEnv::set_print_foo(&webPrint);
    cout << "->SQMOD\n";
}

SqMod::~SqMod()
{
    std::vector<void*>::iterator b = _exts.begin();
    for(; b!= _exts.end(); ++b)
    {
        dlclose(*b);
    }
    delete _penv;
    cout << "<-SQMOD\n";
}

void   SqMod::destroy()
{
    if(!_tmpfile.empty())
    {
        unlink(_tmpfile.c_str());
        _tmpfile.clear();
    }
    //_penv->reset();
}

bool  SqMod::construct(CtxMod* pctx)
{
    _pctx = pctx;
    _header_sent=false;
    if(_penv)
    {
        _penv->acquire();

        Sqrat::Class<SqMod> module;
        module.Func(_SC("write"), &SqMod::swrite);
        module.Func(_SC("save_file"), &SqMod::save_file);
        Sqrat::RootTable().SetInstance(_SC("_this"), this);

        //
        // _context
        //
        Sqrat::Table context;
        context.SetValue(_SC("urldoc"), pctx->_url_doc)
            .SetValue(_SC("urldir"),  pctx->_url_path)
            .SetValue(_SC("home"),    pctx->_dir_home)
            .SetValue(_SC("index"),   pctx->_doc_index)
            .SetValue(_SC("ip"),      pctx->_cli_ip);
        Sqrat::RootTable().Bind(_SC("_context"), context);

        //
        // _REQ table with _ARGS, GET, COOKIE and so on
        //
        Sqrat::Table headers;
        const KeyValx* ps = _pctx->_hdrs;
        while((ps)->key)
        {
            if(ps->val && *ps->val)
            {
                headers.SetValue(_SC(ps->key), _SC(ps->val));
            }
            ++ps;
        }
        Sqrat::RootTable().Bind(_SC("_request"), headers);


        Sqrat::Table ckooks;
        const KeyValx* psc = _pctx->_cookies;
        while((psc)->key)
        {
            if(psc->val && *psc->val)
            {
                ckooks.SetValue(_SC(psc->key), _SC(psc->val));
            }
            ++psc;
        }
        Sqrat::RootTable().Bind(_SC("_cookies"), ckooks);
        Sqrat::Table get;
        const KeyValx* psv = _pctx->_get;
        while((psv)->key)
        {
            if(psv->val && *psv->val)
            {
                get.SetValue(_SC(psv->key), _SC(psv->val));
            }
            ++psv;
        }
        Sqrat::RootTable().Bind(_SC("_get"), get);

        Sqrat::Table post;
        Sqrat::Table put;
        if(pctx->_tempfile)
        {
            const KeyValx* pct = _getHdr("CONTENT-TYPE");
            const KeyValx* pcl = _getHdr("CONTENT-LENGTH");
            if(pct && pcl)
            {
                kchar* astype = pct->val;
                int64_t len = atoll(pcl->val);
                if(len == 0)
                {
                    kchar *p = HTTP_ERR("invalid POST");
                    return _pctx->socket_write(p, strlen(p));
                    return false;
                }
                if(pctx->_emethod == HASH_METH::ePOST)
                {
                    if(!str_cmp(astype,"application/x-www-form-urlencoded") && len < 16384)
                    {
                        _fill_form_data(post, pctx->_tempfile, len);
                    }
                    else  if(!strncmp(astype,"multipart/form-data;",20))
                    {
                        _fill_mpart_data(post, pctx->_tempfile, len, astype);
                    }
                }
                else if(pctx->_emethod == HASH_METH::ePUT)
                {
                    _save_put_data(put, pctx->_tempfile, len);
                }
            }//has headers
        }
        Sqrat::RootTable().Bind(_SC("_post"), post);
        Sqrat::RootTable().Bind(_SC("_put"), put);
        return true;
    }
    return false;
}

size_t  SqMod::swrite(kchar* s)
{
    //printf("swrite called %p  %p\n", _pctx, this);
    //printf("swriting=%s\n",s);
    usleep(256);
    if(!_header_sent)
    {
        header("Content-Type: text/html; charset=utf-8");
        _header_sent = true;
    }
    if(s && *s && _pctx)
        return _pctx->socket_write(s, strlen(s));
    return 0;
}

size_t  SqMod::save_file(kchar* where)
{
    if(!_tmpfile.empty())
    {
        struct stat s;
        char cmd[256];
        sprintf(cmd,"rm -f %s; mv %s %s",where, _tmpfile.c_str(), where);
        system(cmd);
        if(stat(where, &s) == 0)
        {
            return s.st_size;
        }
    }
    return 0;
}

size_t SqMod::header(kchar* hdr)
{
    char    hdrout[4096];
    size_t  bytes = 0;

    if(!_cookies.empty())
    {
        _cookies += hdr;
        bytes =sprintf(hdrout,HTTP_200, _cookies.c_str());
    }
    else
    {
        bytes = sprintf(hdrout,HTTP_200,hdr);
    }
    _header_sent=true;
    if(bytes)
        return _pctx->socket_write(hdrout, bytes);
    kchar *p = HTTP_ERR("empty content");
    return _pctx->socket_write(p, strlen(p));
}

size_t SqMod::setCookie(kchar* key, kchar* val, int years)
{
    time_t t;
    time(&t);
    struct tm *date = gmtime(&t);
    char   cookie[512];
    date->tm_year+=years;
    if(date->tm_mday>28)
    {
        date->tm_mday=28;    // fix feb and days a year from now
    }
    sprintf(cookie, "Set-Cookie: %s=%s;"
            " expires=%s,%02d-%s-%d %d:%d:%d GMT;"
            "path=%s;\r\n",
            key,val,
            str_days(date->tm_wday),
            date->tm_mday,
            str_months(date->tm_mon),
            date->tm_year+1900, //one year from now
            date->tm_hour,
            date->tm_min,
            date->tm_sec,
            _pctx->_url_path);
    _cookies = cookie;
    return 0;
}

inline int64_t  simple_hash(kchar* s)
{
    int64_t  hash = 0;
    int64_t c;
    while((c = (int64_t)*s++))
    {
        hash = ((hash << 5) + hash) ^ c;
    }
    return hash;
}

int  SqMod::work(CtxMod* pctx, CAN_RW rw)
{
    char   docu[512];
    char   docu_hash[512];
    struct stat sdocu;
    struct stat sdocuh;

    ::sprintf(docu,"%s%s%s", pctx->_dir_home, pctx->_url_path, pctx->_url_doc);
    if(stat(docu, &sdocu)!=0) //file existence
    {
        kchar* p = HTTP_ERR(pctx->_url_doc);
        _pctx->socket_write(p, strlen(p));
        return _ERR(1);
    }
    ::sprintf(docu_hash, "%s%zu.nb", pctx->_dir_cache, simple_hash(docu));
    if( _pctx->_settings==0 ||
            stat(docu_hash, &sdocuh)!=0 ||
            sdocuh.st_mtime != sdocu.st_mtime)
    {
        return _runScript(docu, docu_hash, sdocu.st_mtime, pctx->_url_doc[0]=='_');
    }
    return _runScript(0, docu_hash, 0, pctx->_url_doc[0]=='_');
}

int  SqMod::_runScript(kchar* docu,
                       kchar* docu_hash,
                       time_t script_time, bool istmpl)
{
    std::string buffer;

    _penv->acquire(); //on this thread
    try
    {
        if(docu)   //compile it in docu_hash
        {
            struct utimbuf  utb = {script_time, script_time};

            if(istmpl) //is a tempplate file
            {

                _processTemplate(docu, buffer);
                MyScript s = _penv->compile_buffer(buffer.c_str());
                s.run_script();

                s.WriteCompiledFile(docu_hash);
                utime(docu_hash, &utb);
            }
            else
            {
                MyScript s = _penv->compile_script(docu);
                s.run_script();

                s.WriteCompiledFile(docu_hash);
                utime(docu_hash, &utb);
            }
            return _DONE;
        }
        else
        {
            MyScript s = _penv->compile_script(docu_hash);
            s.run_script();
            return _DONE;
        }
    }
    catch(Sqrat::Exception ex)
    {
        swrite("<H1>EXCEPTION</H1>");
        webPrint(ex.Message().c_str());
        if(!buffer.empty())
            webPrint(buffer.c_str());

    }
    catch(...)
    {
        ;
    }
    return _ERR(1);
}

bool   SqMod::is_singleton()
{
    return true;
}

char* SqMod::_newFileCntn(kchar* fname, size_t sz)
{
    FILE* pfd = fopen(fname,"rb");
    if(!pfd)
    {
        kchar* p = HTTP_ERR("pre-compiled");
        _pctx->socket_write(p, strlen(p));
        return 0;
    }
    char* p = new char[sz+1];
    size_t l = fread(p, 1, sz, pfd);
    p[l] = 0;
    fclose(pfd);
    pfd=0;
    return p;
}


bool   SqMod::uses_fds()const
{
    return false;
};
int    SqMod::fd_action (FD_OPS op, fd_set& rd, fd_set& wr)
{
    return 0;
};


const KeyValx* SqMod::_getHdr(kchar* key)const
{
    const KeyValx* ph = _pctx->_hdrs;

    for(size_t k = 0 ; ph[k].key; k++)
    {
        if(!str_cmp(key, ph[k].key))
        {
            return &ph[k];
        }
    }
    return 0;
}

int  SqMod::_include( kchar* fin, FILE* pfo, size_t ninsz)
{
    char* p =_newFileCntn(fin, ninsz);
    if(0==p) return _ERR(errno);
    fwrite(p, ninsz, 1, pfo);
    delete []p;
    return 0;
}

bool  SqMod::_fill_form_data(Sqrat::Table& post, FILE* tempfile, size_t len)
{
    char* buff = new char[len + 1];
    if(buff)
    {
        int nposts = 0;
        size_t bytes = fread(buff, 1, len, tempfile);
        char* pwalk = buff;

        assert(bytes == len);
        buff[bytes] = 0;
        do
        {
            kchar* key = str_up2chr(pwalk, '=');
            kchar* val = str_up2chr(pwalk, '&');
            if(!(*key) ||  !(*val))
                break;
            post.SetValue(_SC(key), _SC(val));
        }
        while(nposts++ < 64);
        delete []buff;
        return true;
    }
    return false;
}

bool  SqMod::_fill_mpart_data(Sqrat::Table& post, FILE* tempfile, size_t len, kchar* delim)
{
    char postfn[256];
    char tmpfn[256];

    ::sprintf(postfn,"/proc/self/fd/%d",fileno(tempfile));
    if(::readlink(postfn, tmpfn, sizeof(tmpfn)-8))
    {
        char* ptmpfn = tmpfn;
        str_up2any(ptmpfn," (");
        strcat(tmpfn,".mpd");
        FILE* pf = fopen(tmpfn,"wb");
        if(pf)
        {
#define WSZ    128
#define BSZ    4096
            char    buff[BSZ];
            char    delim[WSZ];
            char    baccum[WSZ];
            size_t  off = 0;

            _tmpfile = tmpfn;

            // get mime delimiter
            size_t  all_bytes=0;
            fgets(buff, 64, tempfile);
            char* pbuff = buff;
            sprintf(delim, "\r\n%s", str_up2any(pbuff, "\r\n"));
            size_t ndelim = strlen(delim);
            size_t ntest_delim = 0;
            fseek(tempfile, 0, SEEK_SET);

            while(!feof(tempfile))
            {
                ::fgets(buff, BSZ, tempfile);
                ::str_deleol(buff);
                if(!::strcmp(buff, delim+2))
                {
                    ::fgets(buff, BSZ, tempfile);
                    if(!::strstr(buff,"form-data"))break;

                    kchar* key = ::str_after(buff,"filename=",'\"');
                    if(0 == key)
                        key = ::str_after(buff,"name=",'\"');

                    if(0 == key)
                        break;

                    ::fgets(buff, BSZ, tempfile);
                    if(::strstr(buff,"Content-Type:"))
                    {
                        ::fgets(buff, BSZ, tempfile);
                        post.SetValue(_SC("FILENAME"),_SC(key));
                        post.SetValue(_SC("TEMPFILE"),_SC(tmpfn));

                        bool bdelim=false;
                        size_t acum = 1;
                        const char* pfrom = buff;
                        while(!feof(pf))
                        {
                            size_t bytes = ::fread(buff, 1, BSZ, tempfile);
                            for(size_t b=0; b < bytes; ++b)
                            {
                                if(buff[b] == delim[ntest_delim])
                                {
                                    baccum[ntest_delim++] = buff[b];
                                }
                                else
                                {
                                    if(ntest_delim)
                                        all_bytes += ::fwrite(baccum, 1, ntest_delim, pf);
                                    all_bytes += ::fwrite(&buff[b], 1, 1, pf);
                                }
                                bdelim = (ntest_delim==ndelim);
                                if(bdelim)
                                    break; //for break only
                            }

                            if(feof(tempfile) || bdelim)
                                break;
                        }
                        post.SetValue(_SC("FILESIZE"),_SC(all_bytes));
                    }
                    else if(buff[0]=='\r' && buff[1]=='\n')
                    {
                        ::fgets(buff, BSZ, tempfile);
                        kchar* val = str_deleol(buff);

                        post.SetValue(_SC(key),_SC(val));
                    }else{
                        break;
                    }
                }
            }
            fclose(pf);
        }
    }

    return feof(tempfile);
}


bool  SqMod::_save_put_data(Sqrat::Table& put, FILE* tempfile, size_t len)
{
    char postfn[256];
    char tmpfn[256];

    ::sprintf(postfn,"/proc/self/fd/%d",fileno(tempfile));
    if(::readlink(postfn, tmpfn, sizeof(tmpfn)-8))
    {
        #define BSZ    4096
        size_t all_bytes = 0;
        char* ptmpfn = tmpfn;
        str_up2any(ptmpfn," (");
        strcat(tmpfn,".mpd");
        FILE* pf = fopen(tmpfn,"wb");
        if(pf)
        {
            _tmpfile = tmpfn;
            char    buff[BSZ];
            while(!feof(tempfile))
            {
                size_t bytes = ::fread(buff, 1, 4096, tempfile);
                if(bytes > 0)
                    all_bytes+=::fwrite(buff, 1, bytes, pf);
                if(feof(tempfile))
                    break;
            }
            fclose(pf);
            const char* pfile = _pctx->_url_doc;
            if(pfile && *pfile)
                put.SetValue(_SC("FILENAME"),_SC(pfile));
            put.SetValue(_SC("TEMPFILE"),_SC(tmpfn));
            put.SetValue(_SC("FILESIZE"),_SC(all_bytes));

        }
    }
    return true;
}


bool  SqMod::_processTemplate(const char* docuname, std::string& buffer)
{
    //docyname @
    char  scriptname[128];
    char  buff[4096];

    strcpy(scriptname, docuname); // this is the script;
    char* pdash = strchr(scriptname,'_');
    int l = strlen(pdash+1);
    memmove( pdash, pdash+1, l+1);

    FILE* pf = fopen(scriptname,"rb");
    while(pf && !feof(pf))
    {
        int bytes = fread(buff, 1, 512, pf);

        buff[bytes]=0;
        buffer.append(buff);
        if(feof(pf)) break;
    }
    fclose(pf);

    pf = fopen(docuname,"rb");  //template
    buffer.append("echo (@\"\n");
    bool        interpret = false;

    while(pf && !feof(pf))
    {
        int bytes = fread(buff, 1, 512, pf);
        buff[bytes]=0;

        for(int b = 0; b < bytes; b++)
        {
            char c = buff[b];
            if(c=='`')
            {
                if(!interpret)
                {
                    buffer += "\");";
                }
                else
                {
                    buffer += "echo (@\"" ;
                }
                interpret=!interpret;
                continue;
            }
            if(!interpret && c=='\"')
                c='\'';

            buffer+=c;
        }

        if(feof(pf)) break;
    }
    fclose(pf);
    if(!interpret)
         buffer += "\");\n";

    cout << "----------------------------------------------------------\n";
    cout << buffer << "\n";
    cout << "----------------------------------------------------------\n";

    return true;
}



