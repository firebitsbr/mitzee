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


#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "os.h"
#include <modiface.h>
#include "config.h"
#include <string>
#include <map>

using namespace std;

class Context;
struct Req {
    Req():_done(false),_post_file(0) {
        clear();
    }
    ~Req()
    {
        if(_post_file)
            ::fclose(_post_file);
    }
    enum E_METHOD {eGET,ePUT,eHEADER};
    enum {MAX_REQUEST=8912, MAX_HDRS=128,};
    void clear() {::memset(this,0,sizeof(*this));}
    bool parse(const Context*, size_t bytes, size_t& by_ext);
    void commit_header(const Conf::Vhost* ph);
    int get_content_data(kchar* buff, size_t len);
//private:
    void _parse( char* header, size_t hdr_len);
    kchar* get_hdr(kchar* key)const;
    const KeyValx&  get_meth_kv()const {return _hdrs[0];}

    bool        _done;
    //parser
    char*       _get_line;
    size_t      _hdr_index;

    HASH_METH   _emethod;
    char        _ip[32];
    KeyValx     _hdrs[MAX_HDRS];
    KeyValx     _get[MAX_HDRS];
    KeyValx     _cookie[MAX_HDRS];

    //pre formated
    char       _uri_dir[256];
    char       _uri_doc[512];

    // what we got from content after the  \r\n\r\n we pass to the handler
    // so it can continue processing
    char*      _body;
    size_t     _body_len;

    // big buffer where the pointer point
    char       _request[MAX_REQUEST];

    // post temp file
    int64_t    _post_length;
    FILE      *_post_file;
};

struct Hash
{
    Hash();

    static int GET;
    static int POST;
    static int HEAD;
    static int PUT;
    static int DEL;
    static int OPTIONS;
    static int TRACE;
    static int CONNECT;

    static int hash(kchar* o){
        int rv = 0;
        while(*o++){
            rv += *o;
        }
        return rv;
    };
};


#endif // HTTPREQUEST_H
