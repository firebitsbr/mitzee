/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(udfjj39546284@gmail.com) - getic.net - N/A
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/

#include <assert.h>
#include <sstream>
#include <libgen.h>
#include "main.h"
#include <strutils.h>
#include <stdlib.h>
#include <fcntl.h>
#include "threadpool.h"
#include "httprequest.h"
#include "context.h"


int Hash::GET;
int Hash::POST;
int Hash::HEAD;
int Hash::PUT;
int Hash::DEL;
int Hash::OPTIONS;
int Hash::TRACE;
int Hash::CONNECT;

Hash _hash;

Hash::Hash()
{
    GET=hash("GET");
    POST=hash("POST");
    HEAD=hash("HEAD");
    PUT=hash("PUT");
    DEL=hash("DELETE");
    OPTIONS=hash("OPTIONS");
    TRACE=hash("TRACE");
    CONNECT=hash("CONNECT");
}


bool Req::parse(const Context* pctx, size_t bytes, size_t& by_ext)
{
    if(bytes > 4) {            // because we start 4 bytes ahead
        char* pos = strstr((char*)(_request + by_ext),"\r\n\r\n");
        if(pos == 0) {
            _get_line = strstr(_request, "\r\n"); // has first line
            by_ext = bytes - 4;//not found. hold on 4 bytes before end
        } else {
            /// std::cout << _request;
            pctx->fill_ip(_ip);
            _parse(_request, pos-_request);
            _body = pos+4;
            _body_len = bytes - (pos-_request) - 4;
            ////std::cout << "\nAfter header bytes: " << _body_len << "\n";
            _done = true;
        }
    }
    return _done;
}


/** parse as we go. kind of ugly but efficient
*/
void Req::_parse( char* header, size_t hdr_len)
{
    #define CHECK(pwalkonHdr)    if(*pwalkonHdr == 0){throw Mex(400,__FILE__,__LINE__);}
    #define RETCK(pwalkonHdr)    if(*pwalkonHdr == 0){return;}
    size_t  pos = 0;
    // can have blanks at begining
    if(hdr_len)
        __tp->log(header);

    while(pos < hdr_len && (header[pos]==' ' || header[pos]=='\t'))++pos;
    char*   pwalkonHdr = &header[pos];

    KeyValx* ph = &_hdrs[_hdr_index];
    ph->key = str_up2chr(pwalkonHdr ,' ', &::toupper); CHECK(pwalkonHdr);
    ph->val = str_up2chr(pwalkonHdr, ' '); CHECK(ph->val);
    _emethod = (HASH_METH)Hash::hash(ph->key);
    // read arguments if any
    char* pargs;
    if((pargs = (char*)_ttstrchr(ph->val,'?')) != 0) {
        size_t gets = 0;

        *pargs++ = 0; // end with 0 all arguments
        KeyValx* pget = _get;
        do{
            pget->key=str_up2chr(pargs ,'=');
            pget->val=str_up2chr(pargs ,'&');
            ++pget;
        }while(*pargs && ++gets < (MAX_HDRS-1));
        pwalkonHdr = pargs+1;
    }
    RETCK(pwalkonHdr);

    ph = &_hdrs[++_hdr_index];
    ph->key = "VERSION";                        //fabricated entry
    ph->val = str_up2any(pwalkonHdr, "\r\n");
    RETCK(pwalkonHdr);
    //
    // extract all as we go
    //
    ph = &_hdrs[++_hdr_index];
    do
    {
        kchar* prekey = str_up2any(pwalkonHdr, ": ", &::toupper);
        //CHECK(pwalkonHdr);
        if(!str_cmp(prekey,"COOKIE")){
            size_t nCooks = 0;
            char*  cookiesLine = (char*)str_up2any(pwalkonHdr, "\r\n");

            KeyValx* pck = _cookie;
            do{
                pck->key = str_up2chr(cookiesLine, '=');
                CHECK(cookiesLine);
                pck->val = str_up2any(cookiesLine, "; ");
                ++pck;
            }while(*cookiesLine && nCooks < (MAX_HDRS-1));
            pwalkonHdr=cookiesLine + 2;
            continue;
        }
        ph->key = prekey;
        ph->val = str_up2any(pwalkonHdr, "\r\n");
        ++ph;
    }while(++_hdr_index < (MAX_HDRS-1) && *pwalkonHdr && *pwalkonHdr!='\r');

    if(_hdr_index==MAX_HDRS-1){
        __tp->log("too many headers");
        throw Mex(413, __FILE__,__LINE__);
    }
    ++_hdr_index;
    return;
}


void Req::commit_header(const Conf::Vhost* ph)
{
    char uri_fulldoc[512];

    size_t l = str_urldecode(uri_fulldoc, _hdrs[0].val, false);
    while(uri_fulldoc[l] != '/')--l;
    strcpy(_uri_doc, &uri_fulldoc[l+1]);
    strncpy(_uri_dir, uri_fulldoc, l+1);
    //fix
    if(_uri_dir[0]==_uri_doc[0] && _uri_doc[0] =='/') {
        _uri_doc[0]=0;
    }
    //
    // default if not
    //
    if(*_uri_doc==0) {
        struct stat fstat;
        char loco[PATH_MAX];

        std::istringstream iss(ph->index);
        std::string token;
        while(getline(iss, token, ',')) {
            ::sprintf(loco, "%s%s/%s", ph->home.c_str(), _uri_dir, token.c_str());
            if(0 == stat(loco, &fstat)) {
                strcpy(_uri_doc, token.c_str());
            }
        }
    }
}

int Req::get_content_data(kchar* buff, size_t len)
{
    if(_post_file == 0) {
        //kchar *ptype = get_hdr("CONTENT-TYPE");
        kchar *plen = get_hdr("CONTENT-LENGTH");
        if(0 == plen) {
            return 0;
        }

        _post_length = ::atoll(plen);
        if(0 == _post_length) {
            assert(_body_len == 0); // self check
            return 0;
        }

        kchar *pct = get_hdr("CONTENT-TYPE");
        if(0 == pct){
            _post_length = 0;
            return 0;
        }

        if(!strncmp(pct,"multipart/form-data;", 20) && _post_length > 600000)
        {
            throw Mex(413,__FILE__,__LINE__);
        }


        _post_file = ::tmpfile();
        ::fcntl(fileno(_post_file), F_SETFD, FD_CLOEXEC);

        if(0==_post_file) {
            __tp->log("cannot open post file");
            return -1;
        }
    }
    size_t written = fwrite(buff, 1, len,  _post_file);
    assert(written == len);
    _post_length -= written;
    if(0 == _post_length) {
        rewind(_post_file);
    }
    return  _post_length;
}


kchar* Req::get_hdr(kchar* key)const
{
    for(size_t k = 0 ; k< _hdr_index; k++) {
        if(_hdrs[k].key == 0)
            break;
        if(key[0] == _hdrs[k].key[0]) {
            if(!str_cmp(key, _hdrs[k].key)) {
                return _hdrs[k].val;
            }
        }
    }
    return 0;
}
