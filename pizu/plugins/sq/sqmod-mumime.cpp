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


static const char HTTP_200[] = "HTTP/1.1 200 OK\r\n"
                               "Connection: close\r\n"
                               "%s\r\n"
                               "Server: mariux/1.0\r\n\r\n";

static const char HTTP_404[] = "HTTP/1.1 404 Not Found\r\n"

                               "Connection: close\r\n"
                               "Content-Type: text/html; charset=utf-8\r\n\r\n"
                               "Server: mariux/1.0\r\n\r\n"
                               "<H2>404 %s Not Found!</H2>\r\n";

static char __thread     sHTTP_ERR[512];
static inline const char* HTTP_ERR(const char* _chr)
{
    sprintf(sHTTP_ERR,HTTP_404,_chr);
    return sHTTP_ERR;
}
//----------------------------------------------------------
__thread SqMod*    __pSQ = 0; //this is one per thread.
__thread int       __cnt = 0; //this is one per thread.

static void webPrint(const SQChar* str)
{
    if(__pSQ)
    {
        __pSQ->swrite("<pre><font color='red'>");
        __pSQ->swrite(str);
        __pSQ->swrite("</font></pre>");
    }
    else
    {
        std::cout << str;
    }
}


IModule*  SqMod::get_threaded_singleton(const char* cookie)
{
    if(0 == __pSQ)
    {
        __pSQ = new SqMod(cookie);
    }
    ++__cnt;
    return __pSQ;
}

void SqMod::release_threaded_singleton(IModule* pm, const char* cookie)
{
    --__cnt;
    if(0 == __cnt)
    {
        delete __pSQ;
    }
}

SqMod::SqMod(const char* cook):IModule(cook),_header_sent(false)
{
    _thread_id = pthread_self();
    _penv = new SqEnv();//

    regGlobals(_penv->theVM());
    /** LOADING EXTS
        typedef bool (*load_exts)();
        void  *dll = dlopen("../lib/libswexts.so",RTLD_LAZY);
        if(dll) {
            load_exts pld = (load_exts)dlsym(dll, "load_exts");
            pld();
            _exts.push_back(dll);
        }
    */
    SqEnv::set_print_foo(&webPrint);
}

SqMod::~SqMod()
{
    std::vector<void*>::iterator b = _exts.begin();
    for(; b!= _exts.end(); ++b)
    {
        dlclose(*b);
    }
    delete _penv;
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
        .SetValue(_SC("home"),    "hidden")//pctx->_dir_home)
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
        if(pctx->_tempfile)
        {
            const KeyValx* pct = _getHdr("CONTENT-TYPE");
            const KeyValx* pcl = _getHdr("CONTENT-LENGTH");
            if(pct && pcl)
            {
                const char* astype = pct->val;
                int64_t len = atoll(pcl->val);
                if(len == 0)
                {
                    const char *p = HTTP_ERR("invalid POST");
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

                }
            }//has headers
        }
        Sqrat::RootTable().Bind(_SC("_post"), post);
        return true;
    }
    return false;
}

size_t  SqMod::swrite(const char* s)
{
    if(!_header_sent)
    {
        header("Content-Type: text/html; charset=utf-8");
        _header_sent = true;
    }
    return _pctx->socket_write(s, strlen(s));
}

size_t  SqMod::save_file(const char* where)
{
    if(!_tmpfile.empty())
    {
        struct stat s;
        char cmd[256];
        sprintf(cmd,"mv %s %s", _tmpfile.c_str(), where);
        system(cmd);
        if(stat(where, &s) == 0)
        {
            return s.st_size;
        }
    }
    return 0;
}

size_t SqMod::header(const char* hdr)
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
    const char *p = HTTP_ERR("empty content");
    return _pctx->socket_write(p, strlen(p));
}

size_t SqMod::setCookie(const char* key, const char* val, int years)
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

inline int64_t  simple_hash(const char* s)
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
        const char* p = HTTP_ERR(pctx->_url_doc);
        _pctx->socket_write(p, strlen(p));
        return _ERR(1);
    }
    ::sprintf(docu_hash, "%s%zu.nb", pctx->_dir_cache, simple_hash(docu));
    if( _pctx->_settings==0 ||
            stat(docu_hash, &sdocuh)!=0 ||
            sdocuh.st_mtime != sdocu.st_mtime)
    {
        return _runScript(docu, docu_hash, sdocu.st_mtime);
    }
    return _runScript(0, docu_hash, 0);
}

int  SqMod::_runScript(const char* docu,
                       const char* docu_hash,
                       time_t script_time)
{
    _penv->acquire(); //on this thread
    try
    {
        if(docu)   //compile it in docu_hash
        {
            struct utimbuf  utb = {script_time, script_time};

            MyScript s = _penv->compile_script(docu);
            s.run_script();

            s.WriteCompiledFile(docu_hash);
            utime(docu_hash, &utb);
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
        swrite(ex.Message().c_str());
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

char* SqMod::_newFileCntn(const char* fname, size_t sz)
{
    FILE* pfd = fopen(fname,"rb");
    if(!pfd)
    {
        const char* p = HTTP_ERR("pre-compiled");
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


const KeyValx* SqMod::_getHdr(const char* key)const
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

int  SqMod::_include( const char* fin, FILE* pfo, size_t ninsz)
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
            const char* key = str_up2chr(pwalk, '=');
            const char* val = str_up2chr(pwalk, '&');
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

bool  SqMod::_fill_mpart_data(Sqrat::Table& post, FILE* tempfile, size_t len, const char* delim)
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
#define WSZ    64
#define BSZ    4096
            char    buff[BSZ];
            char    delim[WSZ];
            size_t  off = 0;
            // get mime delimiter
            fgets(buff, 64, tempfile);
            char* pbuff = buff;
            strcpy(delim, str_up2any(pbuff, "\r\n"));
            size_t ndelim = strlen(delim);
            fseek(tempfile, 0, SEEK_SET);

            while(!feof(tempfile))
            {
                fgets(buff, 4096, tempfile);
                str_deleol(buff);
                if(!strcmp(buff, delim))
                {
                    fgets(buff, 4096, tempfile);
                    if(!strstr(buff,"form-data"))break;

                    const char* key = str_after(buff,"filename=",'\"');
                    if(0 == key)
                        key = str_after(buff,"name=",'\"');
                    if(0 == key)
                        break;

                    if(key[0]=='f')
                    {
                        fgets(buff, 256, tempfile);
                        if(!strstr(buff,"Content-Type:"))
                            break;
                        post.SetValue(_SC("filename"),_SC(key));
                        post.SetValue(_SC("tmpfilename"),_SC(tmpfn));

                        bool delim=false;
                        do
                        {
                            size_t b;
                            size_t bytes = fread(buff+off, 1, BSZ-off, tempfile);
                            for( b=0 ; b < bytes; b++)
                            {
                                if(!memcmp(buff+b, (void*)delim, ndelim))
                                {
                                    delim = true;
                                    break;
                                }
                            }
                            if(delim){
                                fwrite(buff,1,b,pf);
                                fseek(tempfile, -(bytes-b),SEEK_CUR);
                                break;
                            }
                            fwrite(buff, 1, bytes-WSZ, pf);
                            memmove(buff, buff+bytes-WSZ, WSZ);
                            off = WSZ;
                        }while(!feof(pf));
                    }
                    else
                    {
                        fgets(buff, 32, tempfile);
                        assert(buff[0]=='\r' && buff[1]=='\n');
                        if(buff[0]!='\r')
                            break;

                        fgets(buff, 4096, tempfile);
                        const char* val = str_deleol(buff);
                        post.SetValue(_SC(key),_SC(val));
                    }
                }
            }
            fclose(pf);
        }
    }
    return feof(tempfile);
}

