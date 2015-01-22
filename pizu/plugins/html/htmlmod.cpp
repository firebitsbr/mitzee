/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/
#include <assert.h>
#include <stdio.h>
#include <string>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utime.h>
#include <pthread.h>
#include <limits.h>
#include <strutils.h>
#include <consts.h>
#include "htmlmod.h"
#include <iostream>
static size_t CHUNK_BUFF = 4096;


static const char HTTP_200[] = "HTTP/1.1 200 OK\r\n"
                               "Connection: close\r\n"
                               "Content-Type: text/html; charset=utf-8\r\n"
                               "Server: mariux/1.0\r\n\r\n";
static const size_t nHTTP_200 = strlen(HTTP_200);

static const char HTTP_404[] = "HTTP/1.1 404 Not Found\r\n"
                               "Connection: close\r\n"
                               "Content-Type: text/html; charset=utf-8\r\n"
                               "Server: mariux/1.0\r\n\r\n"
                               "<H2>404 Not Found!</H2>\r\n";
static const size_t nHTTP_404 = strlen(HTTP_404);



HtmlModule::HtmlModule(const char* cook):IModule(cook),_pf(0),
                                            _left_bytes(0),
                                            _initial(0),
                                            _pctx(0),
                                            _pfn_method(0)
{
}

HtmlModule::~HtmlModule()
{
    assert(_pf==0);
    if(_pf)
        fclose(_pf);
}

bool HtmlModule::construct(CtxMod* pmod)
{
    assert(_pf==0);
    if(_pf)fclose(_pf);
    _pf       = 0;
    _left_bytes = 0;
    _initial  = 0;

    switch(pmod->_emethod)
    {
        case eGET:
            _pfn_method = &HtmlModule::get_work;
            break;
        case ePOST:
            _pfn_method = &HtmlModule::post_work;
            break;
        case eHEAD:
            _pfn_method = &HtmlModule::head_work;
            break;
        case ePUT:
            _pfn_method = &HtmlModule::put_work;
            break;
        case eDEL:
        case eOPTIONS:
        case eTRACE:
        case eCONNECT:
            return false;
    }
    return true;
}

void HtmlModule::destroy()
{
    _pfn_method = 0;
    if(_pf)
        fclose(_pf);
    _pf = 0;

    if(_left_bytes) {
        std::cout << "Module " << this << "LEFT OVER: " << _left_bytes <<
                  " out of: " << _initial << " /  " << (_initial-_left_bytes) << "\n";
    }
}

int HtmlModule::work(CtxMod* pctx, CAN_RW rw)
{
    _pctx = (CtxMod*)pctx;
    if(_pfn_method)
        return (this->*_pfn_method)(pctx, rw);

    pctx->socket_write(HTTP_404, nHTTP_404);
    return _ERR(1);
}

int HtmlModule::get_work(CtxMod* pctx, CAN_RW rw)
{
    if(0 == _pf) {
        char        loco[512];
        const char* home = _pctx->_dir_home; //// var/www/
        const char* dir = _pctx->_url_path;  //// from request localhost:/[dir]/
        const char* fle = _pctx->_url_doc;   //// from request localhost:/dir/[fle.ext]


        size_t leng = sprintf(loco, "%s%s%s", home, dir, fle);
        if(loco[leng-1]=='/') {
            return _reply_directory_listing(loco, home, dir);
        }
        if(0 == stat(loco, &_stat)) {
            return _reply_file_content(loco);
        } else {
            _pctx->socket_write(HTTP_404, nHTTP_404);
        }

        return _DONE;
    }
    assert(_pf != 0);//the file we serving
    return this->_keep_streaming_on();
}


int HtmlModule::_reply_directory_listing(const char* path, const char* home,
        const char* uridir)
{
    unsigned char   isFile = 0x8;
    //unsigned char   isDir = 0x4;
    struct  dirent  *dp;
    DIR             *pdir;
    char            chunk[256], smode[16];
    struct stat     stt;
    bool            bol = false;
    char            fullf[256];
    const char bgk[2][8] = {"F0F0F0", "EAEAEA"};

    if ((pdir = opendir(path)) == NULL) {
        _pctx->socket_write(HTTP_404, nHTTP_404);
        return _DONE;
    }

    _pctx->socket_write(HTTP_200, strlen(HTTP_200));
    const char* p = "<div align='center'><font size='1' face='Courier New'><table border=0 width='60%'><tr><td>file</td><td>Size</td><td>mode</td></tr>";
    _pctx->socket_write(p,strlen(p));
    while ((dp = readdir(pdir)) != NULL) {
        if(dp->d_name[0]=='.')
            continue;

        if(dp->d_type == isFile) {
            sprintf(fullf,"%s%s",path,dp->d_name);
            memset(&stt,0,sizeof(stt));
            stat(fullf, &stt);
            const char* bn = basename(dp->d_name);
            str_int2mode(smode, stt.st_mode);

            sprintf(chunk,"<tr bgcolor='%s'><td><a href='%s%s'>%s</a></td><td>%zu</td><td>%s</td></tr>\n",
                    bgk[!bol],
                    uridir,
                    bn,
                    bn,
                    (u_int64_t)(stt.st_size),
                    smode);

        } else {
            const char* bn = basename(dp->d_name);
            sprintf(fullf,"%s%s",path,dp->d_name);
            // strip up to home
            memset(&stt,0,sizeof(stt));
            stat(fullf, &stt);
            str_int2mode(smode, stt.st_mode);
            sprintf(chunk,"<tr bgcolor='%s'><td><a href='%s%s/'><b>%s/</b></a></td><td>%dKb</td><td>%s</td></tr>\n",
                    bgk[!bol],
                    uridir,
                    bn,
                    bn,
                    4,
                    smode);

        }
        _pctx->socket_write(chunk, strlen(chunk));
    }//while readdir
    closedir(pdir);
    p = "</table></font></div>";
    _pctx->socket_write(p, strlen(p));

    return _DONE;
}

// socket_write len bytes from the opened file to the client.
int HtmlModule::_reply_file_content(const char* file, bool justheader)
{
    const char* ext = str_getfile_ext(file,"*");
    return _reply_raw_content(file,ext, justheader);
}

int HtmlModule::_reply_raw_content(const char* file, const char* ext, bool justheader)
{
    _pf = _ttfopen(file,"rb");
    if(_pf) {
        fcntl(fileno(_pf), F_SETFD, FD_CLOEXEC);
        _left_bytes = _sendHeader(_pf, ext, _stat);
        if(justheader){
            return _DONE;
        }
        _initial = _left_bytes;

        //printf("[%p]<- swriteing %s of : %lld bytes\n",_pf, file, (lld)_left_bytes);
        return _keep_streaming_on();
    }
    _pctx->socket_write(HTTP_404, nHTTP_404);
    return _ERR(1);
}

int HtmlModule::_keep_streaming_on()
{
    char buf[CHUNK_BUFF];
    if( _left_bytes>0 ) {
        size_t rb = CHUNK_BUFF;
        if(_left_bytes < CHUNK_BUFF)
            rb =_left_bytes;
        size_t read = fread(buf, 1, rb, _pf);
        if(read > 0) {
            // printf("[%p]<- leftover %lld bytes\n",_pf, (lld)_left_bytes);
            if(0!=_pctx->socket_write(buf, (int)read)) {

                printf("%p remote closed -> we close. outstanding bytes: (%zu / %zu)\n",this, read, _left_bytes);
                fclose(_pf);
                _pf=0;
                std::cout << __FILE__ << __LINE__ << "\n";
                return _ERR(1);
            }
            _left_bytes -= (int64_t)read;
            //usleep(0x1FFF);
            if(_left_bytes > 0) {
                return _CONTINUE;
            }
        }
    }
//    printf("%p wrote all bytes %llu  let over %llu\n",this, _initial, _left_bytes);
    fclose(_pf);
    _pf = 0;
    return _DONE;
}

int64_t HtmlModule::_sendHeader(FILE* fp, const char* ext, const struct stat& stt)
{
    u_int64_t      allbytes = stt.st_size, a,b;
    char        header[640];
    size_t      sofar = 0;
    u_int64_t      bytes = allbytes;

    const char* ifr = _getHeader("IF-RANGE");
    if(!ifr) {
        bytes = extractRanges(_getHeader("RANGE"), allbytes, a,b);
        if(bytes != allbytes) //there are ranges
            sofar += sprintf(header+sofar,"HTTP/1.1 206 Partial Content\r\n");
        else
            sofar += sprintf(header+sofar,"HTTP/1.1 200\r\n");
    } else {
        sofar += sprintf(header+sofar,"HTTP/1.1 200\r\n");
    }
    sofar += strftime(header+sofar, sizeof(header)-sofar-1,
                      "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", localtime(&_pctx->_start_time));
    sofar += strftime(header+sofar, sizeof(header)-sofar-1,
                      "Last-Modified: %a, %d %b %Y %H:%M:%S %Z\r\n", localtime(&stt.st_mtime));
    sofar += sprintf(header+sofar,
                     "Etag: \"%lx.%lx\"\r\n",(unsigned long) stt.st_mtime,
                     (unsigned long) stt.st_size);

    if(ext)
        sofar+=sprintf(header+sofar, "Content-Type: %s\r\n", extractMime(ext));
    sofar+=sprintf(header+sofar, "Content-Length: %zu\r\n",bytes);
    sofar+=sprintf(header+sofar, "Cache-Control: no-cache\r\n");


    const char* pcc = _getHeader("CONNECTION");
    sofar+=sprintf(header+sofar,"Connection: %s\r\n", pcc ? pcc : "close");
    sofar+=sprintf(header+sofar, "Accept-Ranges: bytes\r\n");
    if(ifr == 0 && bytes != allbytes)
        sofar+=sprintf(header+sofar, "Content-Range: bytes %zu-%zu/%zu\r\n",
                       a, a+bytes, stt.st_size);

    sofar+=sprintf(header+sofar, "\r\n");
    if(ext && ext[0]=='@')
    {
        return bytes; //do no send header.
    }
    _pctx->socket_write(header, sofar);
    return  bytes;
}

const char* HtmlModule::_getHeader(const char* key)const
{
    const KeyValx* ph = _pctx->_hdrs;

    for(size_t k = 0 ; ph[k].key; k++) {
         if(!str_cmp(key+1, ph[k].key+1)) {
                return ph[k].val;
         }
    }
    return 0;
}

bool   HtmlModule::is_singleton(){return false;};


int HtmlModule::head_work(CtxMod* pctx, CAN_RW rw)
{
    char        loco[PATH_MAX];

    //const char* index = _pctx->_doc_index;
    const char* home = _pctx->_dir_home; //// var/www/
    const char* dir = _pctx->_url_path;  //// from request localhost:/[dir]/
    const char* fle = _pctx->_url_doc;   //// from request localhost:/dir/[fle.ext]

    size_t leng = sprintf(loco, "%s%s%s", home, dir, fle);
    if(loco[leng-1]=='/') {
        _pctx->socket_write(HTTP_404, nHTTP_404);
    }else{
        if(0 == stat(loco, &_stat)) {
            _reply_file_content(loco, true);
        } else {
            _pctx->socket_write(HTTP_404, nHTTP_404);
        }
    }
    return _DONE;
}

int HtmlModule::put_work(CtxMod* pctx, CAN_RW rw)
{
    // ??? todo
    return _ERR(1);
}

// we can get a large file here up to max filesize
int HtmlModule::post_work(CtxMod* pctx, CAN_RW rw)
{
    assert(pctx->_tempfile);

    struct stat fst;
    fstat(fileno(pctx->_tempfile), &fst);
    std::cout << "temp file is: " << fst.st_size << "\n";
    // we save temp file
    return get_work(pctx, rw);
}



 bool   HtmlModule::uses_fds()const{return false;};
 int    HtmlModule::fd_action (FD_OPS op, fd_set& rd, fd_set& wr){return 0;};
