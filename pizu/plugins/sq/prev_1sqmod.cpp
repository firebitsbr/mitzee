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
static size_t __thread  nHTTP_ERR;
static inline const char* HTTP_ERR(const char* _chr)
{
    nHTTP_ERR=sprintf(sHTTP_ERR,HTTP_404,_chr);
    return sHTTP_ERR;
}
//----------------------------------------------------------
__thread SqMod*    __pSQ = 0; //this is one per thread.
__thread int       __cnt = 0; //this is one per thread.

static void webPrint(const SQChar* str)
{
    if(__pSQ){
        __pSQ->swrite("<pre><font color='red'>");
        __pSQ->swrite(str);
        __pSQ->swrite("</font></pre>");
    }else{
        std::cout << str;
    }
}


IModule*  SqMod::get_threaded_singleton(const char* cookie)
{
    if(0 == __pSQ) {
        __pSQ = new SqMod(cookie);
    }
    ++__cnt;
    return __pSQ;
}

void SqMod::release_threaded_singleton(IModule* pm, const char* cookie)
{
    --__cnt;
    if(0 == __cnt) {
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
    SqEnv::setPrintFoo(&webPrint);
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
    //_penv->reset();
}

bool  SqMod::construct(CtxMod* pctx)
{
    _pctx = pctx;
    _header_sent=false;
    if(_penv) {
        _penv->acquire();

        Sqrat::Class<SqMod> module;
        module.Func(_SC("write"), &SqMod::swrite);
        Sqrat::RootTable().SetInstance(_SC("_this"), this);

        //
        // _context
        //
        Sqrat::Table context;
        //Sqrat::Class<CtxMod> context;
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
        while((ps)->key) {
            if(ps->val && *ps->val) {
                headers.SetValue(_SC(ps->key), _SC(ps->val));
            }
            ++ps;
        }
        Sqrat::RootTable().Bind(_SC("_request"), headers);


        Sqrat::Table ckooks;
        const KeyValx* psc = _pctx->_cookies;
        while((psc)->key) {
            if(psc->val && *psc->val) {
                ckooks.SetValue(_SC(psc->key), _SC(psc->val));
            }
            ++psc;
        }
        Sqrat::RootTable().Bind(_SC("_cookies"), ckooks);

        Sqrat::Table get;
        const KeyValx* psv = _pctx->_get;
        while((psv)->key) {
            if(psv->val && *psv->val) {
                get.SetValue(_SC(psv->key), _SC(psv->val));
            }
            ++psv;
        }
        Sqrat::RootTable().Bind(_SC("_get"), get);
        //Sqrat::RootTable().SetInstance(_SC("_ctx"), pctx);

        Sqrat::Table post;

        if(pctx->_tempfile) {
            // parse the post
            /*
            Content-Type: application/x-www-form-urlencoded
            Content-Length: 53

            realname=ssssssss&email=ddddddddddd&comments=wwwwwwww
            */
            const char* astype = _getHdr("CONTENT-TYPE")->val;
            int64_t len = atoll(_getHdr("CONTENT-LENGTH")->val);
            if(!str_cmp(astype,"application/x-www-form-urlencoded") && len < 16384) {
                char* buff = new char[len + 1];
                if(buff) {
                    int bytes = fread(buff, 1, len, pctx->_tempfile);
                    char* pwalk = buff;
                    assert(bytes == len);
                    buff[bytes] = 0;
                    do {
                        const char* key = str_up2chr(&pwalk, '=');
                        const char* val = str_up2chr(&pwalk, '&');
                        if(!valid_ptr(key) ||  !valid_ptr(val))
                            break;
                        post.SetValue(_SC(key), _SC(val));
                    } while(1);
                    delete []buff;
                }
            }
        }
        Sqrat::RootTable().Bind(_SC("_post"), post);
        return true;
    }
    return false;
}

size_t  SqMod::swrite(const char* s)
{
    if(!_header_sent) {
        header("Content-Type: text/html; charset=utf-8");
        _header_sent = true;
    }
    return _pctx->socket_write(s, strlen(s));
}


size_t SqMod::header(const char* hdr)
{
    char    hdrout[2048];
    size_t  bytes = 0;

    if(!_cookies.empty()){
        _cookies += hdr;
        bytes =sprintf(hdrout,HTTP_200, _cookies.c_str());
    }
    else{
        bytes = sprintf(hdrout,HTTP_200,hdr);
    }
    _header_sent=true;
    if(bytes)
        return _pctx->socket_write(hdrout, bytes);

    return _pctx->socket_write(HTTP_ERR("empty content"), nHTTP_ERR);
}

size_t SqMod::setCookie(const char* key, const char* val, int exptime)
{
 	time_t t;
    time(&t);
	struct tm *date = gmtime(&t);
	char   cookie[512];
	sprintf(cookie, "Set-Cookie: %s=%s;"
                    "%s,%02d-%s-%d %d:%d:%d GMT;"
                    "path=%s;\r\n",
                    key,val,
                    str_days(date->tm_wday),
                        date->tm_mday,
                             str_months(date->tm_mon),
                                date->tm_year+1900,
                                    date->tm_hour,
                                      date->tm_min,
                                         date->tm_sec,
                                          _pctx->_url_path);
    _cookies = cookie;
}

inline int64_t  simple_hash(const char* s)
{
    int64_t  hash = 0;
    int64_t c;
    while((c = (int64_t)*s++)) {
        hash = ((hash << 5) + hash) ^ c;
    }
    return hash;
}

int  SqMod::work(CtxMod* pctx, CAN_RW rw)
{
    char   docu[512];
    char   docu_hash[512];
    struct stat forig;

    size_t chars = sprintf(docu,"%s%s%s", pctx->_dir_home,
                                          pctx->_url_path,
                                          pctx->_url_doc);

    _pctx = pctx;
    if(stat(docu, &forig)!=0) { //no file
        _pctx->socket_write(HTTP_ERR(docu), nHTTP_ERR);
        return _ERR(1);
    }

    int64_t  docu_hname = simple_hash(docu);
    sprintf(docu_hash,"%ssbin/_%020lld.pnut", _pctx->_dir_cache, docu_hname);
    struct stat fcahced={0};
    stat(docu_hash, &fcahced);
    if(forig.st_mtime != fcahced.st_mtime)
    {
        char   composed[FILENAME_MAX];

        sprintf(composed,"%stmp/_%08X%08X%s", _pctx->_dir_cache,
                                this, pthread_self(), _pctx->_url_doc);
        FILE* pf = fopen(composed,"wb");
        if(pf){
            int err = _include(docu, pf, forig.st_size);
            fclose(pf);
            if(0==err)
                return _runScript(composed, docu_hash, forig.st_mtime);
            unlink(composed);
        }
        _pctx->socket_write(HTTP_ERR(composed), nHTTP_ERR);
        return _ERR(1);

    }
    return _runScript(0, docu_hash, forig.st_mtime);
}


int  SqMod::_runScript(const char* docu,
                        const char* docu_hash,
                        time_t script_time)
{
    _penv->acquire(); //on this thread
    try {
        if(docu) { //compile it in docu_hash
            struct utimbuf  utb = {script_time, script_time};
            MyScript s = _penv->CompileScript(docu);
            s.Run();
            s.WriteCompiledFile(docu_hash);
            utime(docu_hash, &utb);
            return _DONE;
        } else {
            MyScript s = _penv->CompileScript(docu_hash);
            s.Run();
            return _DONE;
        }
    }
    catch(Sqrat::Exception ex)
    {
        swrite(ex.Message().c_str());
        unlink(docu_hash);
    }
    return _ERR(1);
}

bool   SqMod::isSingleton()
{
    return true;
}

char* SqMod::_newFileCntn(const char* fname, size_t sz)
{
    FILE* pfd = fopen(fname,"rb");
    if(!pfd){
        _pctx->socket_write(HTTP_ERR(fname), nHTTP_ERR);
        return 0;
    }
    char* p = new char[sz+1];
    size_t l = fread(p, 1, sz, pfd);
    p[l] = 0;
    fclose(pfd); pfd=0;
    return p;
}

int  SqMod::_include( const char* fin, FILE* pfo, size_t ninsz)
{
    const char* ext = str_getfile_ext(fin);
    if(!str_cmp(ext+1,"hnut"))
    {
        char* p =_newFileCntn(fin, ninsz);
        if(!p) return _ERR(errno);
        char* walk = p;
           while(*walk){
            if(*walk=='\"')*walk='\'';
            walk++;
        }
        walk = p;

        const char *tok;
        char tk  = '%';  // <% %>
        while((tok = str_up2chr(&walk, tk)))
        {
            if(*walk=='{')
            {
                ++walk;
                fprintf(pfo,"echo (@\"%s\");\n",tok);
            }
            else if(*walk=='}')
            {
                ++walk;
                fprintf(pfo,"%s",tok);
            }
            else if(*walk)
            {
                fprintf(pfo,"echo (@\"%%s\");\n",tok);
            }
            else{
                fprintf(pfo,"echo (@\"%s\");\n",tok);
                break;
            }
        }
        delete[] p;
        return 0;
    }

    if(!str_cmp(ext+1,"nut"))
    {
        char* p =_newFileCntn(fin, ninsz);
        if(!p) return _ERR(errno);
        fwrite(p, ninsz, 1, pfo);
        delete []p;
        return 0;
    }

    if(!str_cmp(ext+1,"tnut"))
    {
        char* p =_newFileCntn(fin, ninsz);
        if(!p) return _ERR(errno);
        char* walk = p,*tok, *token;
        char ctok = '@';
        bool processed =false;
        while((token = str_up2chr(&walk, ctok)))
        {
            if(*token==0 && *walk){
                token = str_up2chr(&walk, ';');
                ctok=';';
            }
            if(ctok=='@'){
                fprintf(pfo,"\necho(@\"%s\");", token);
                ctok =';';
            }
            else
            {
                if(token[0]=='='){
                    fprintf(pfo, "\necho(%s);\n ", token+1);
                }
                else if(token[0]=='#'){
                    const char* file = token+1;
                    char docu[PATH_MAX];
                    if(*file=='/')//absolute path
                    {
                        strcpy(docu,file);
                    }
                    else{
                        sprintf(docu,"%s%s%s", _pctx->_dir_home,_pctx->_url_path,file);
                    }
                    struct stat ts;
                    if(stat(docu, &ts)){
                        _pctx->socket_write(HTTP_ERR(docu), nHTTP_ERR);
                        return _ERR(1);
                    }
                    if(_include(docu, pfo, ts.st_size)){
                        return _ERR(2);
                    }
                }
                else{
                    fprintf(pfo,"\n%s;",token);
                }
                ctok ='@';
            }
            if(0==*walk)
                break;
        }
        delete []p;
        return 0;
    }
    _pctx->socket_write(HTTP_ERR(fin), nHTTP_ERR);
    return _ERR(1);
}

bool   SqMod::hasFds()const
{
    return false;
};
int    SqMod::fdOp (FD_OPS op, fd_set& rd, fd_set& wr)
{
    return 0;
};


const KeyValx* SqMod::_getHdr(const char* key)const
{
    const KeyValx* ph = _pctx->_hdrs;

    for(size_t k = 0 ; ph[k].key; k++) {
        if(!str_cmp(key+1, ph[k].key+1)) {
                return &ph[k];
        }
    }
    return 0;
}


