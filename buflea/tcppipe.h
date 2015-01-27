/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - coinscode.com - N/A
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


#ifndef TCPPIPE_H
#define TCPPIPE_H

#include <iostream>
#include <sock.h>
#include "main.h"
#include "strutils.h"
#include "threadpool.h"
#include "sslcrypt.h"

//-----------------------------------------------------------------------------
class Ctx;
class TcpPipe : public tcp_cli_sock
{
public:

    TcpPipe():tcp_cli_sock(),_ssl(0),_incoming(false),_gothdr(false){
    }
    virtual ~TcpPipe(){
        destroy();
    }
    //just the socket
    TcpPipe(const TcpPipe& o):tcp_cli_sock(o),_ssl(0){
        //_ssl = o._ssl
    }
    void destroy(){
        if(_ssl){
//            int sd = SSL_get_fd(_ssl);
//            ::close(sd);
            //SSL_shutdown(_ssl);
            SSL_set_fd(_ssl, 0);
            SSL_free(_ssl);
            _ssl = 0;
        }
        tcp_cli_sock::destroy();
        _incoming=false;
    }
    int sendall(const u_int8_t* buff, int length, int tout=8912);
    int sendall(const std::string& s, int tout=8912){return sendall((u_int8_t*)s.c_str(), (int)s.length(), tout);}
    int receive(u_int8_t* buff, int length);
    int raw_connect(u_int32_t uip4, int port){
        int rv =  tcp_cli_sock::raw_connect(uip4, port);
        return rv;
    }
    bool setconnected(const Ctx* pc);
    bool ssl_accept(const Ctx* pc);
    void set_ctx(const Ctx* pc){_pc=pc;}


private:
    SSL     *_ssl;
    bool    _incoming;
    bool    _gothdr;
    const   Ctx* _pc;
public:
    SADDR_46 _raddr;
};

#endif // TCPPIPE_H
