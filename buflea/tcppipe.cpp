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

#include "tcppipe.h"
#include "configprx.h"
#include "context.h"

//-----------------------------------------------------------------------------
int TcpPipe::sendall(const u_int8_t* buff, int length, int tout)
{
    if(_ssl)
    {
        int   shot, sent=0,trys=10,err;

        if(length>GCFG->_pool.buffsize)
        {
            tout += 1000 * (length / GCFG->_pool.buffsize);
        }

        while(length>0)
        {
            shot = SSL_write(_ssl, buff + sent, length);
            if(shot <= 0)
            {
                GLOGE( "SSL sendall" << sslNerror(_ssl));
                destroy();
                break;
            }
            sent+=shot;
            length-=shot;
        }
        return length;              //return 0 if all was send
    }
    return tcp_cli_sock::sendall((unsigned char*)buff, length, tout);
}

//-----------------------------------------------------------------------------
int TcpPipe::receive(u_int8_t* buff, int length)
{
    int rd,err;
    if(_ssl)
    {
        rd = SSL_read(_ssl, buff, length);
        if(rd <= 0)
        {
            GLOGE( "SSL read" << sslNerror(_ssl) <<" : "<< errno);
            rd=0;
            destroy();

        }
        return rd;
    }
    rd = tcp_cli_sock::receive(buff, length, 0, 0);
    if(rd>0) buff[rd]=0;
    return rd; //return 0 connclosed, -1 no bytes, >0 bytes received
}

bool TcpPipe::setconnected(const Ctx* pc)
{
    tcp_cli_sock::setconnected();
    if(pc->_pcsl_ctx)
    {
        assert(_ssl==0);
        this->set_blocking(1);
        _ssl = SSL_new(pc->_pcsl_ctx);
        if(_ssl)
        {
            SSL_set_fd(_ssl, socket());
        }
        if(-1==SSL_connect(_ssl))
        {
            GLOGE( "SSL connect" << sslNerror(_ssl));
            destroy();
            return false;
        }
    }
    _incoming=true;
    return true;
}


bool TcpPipe::ssl_accept(const Ctx* pc)
{
    assert(_ssl==0);

    this->set_blocking(1);
    _ssl = SSL_new(pc->_pssl_ctx);
    if(_ssl)
    {
        SSL_set_fd(_ssl, socket());
        if(SSL_accept(_ssl)<=0)
        {
            GLOGE( "SSL accept error: " << sslNerror(_ssl)); //will faild
            destroy();
            return false;
        }
    }
    return true;
}



