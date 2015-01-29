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
        int   shot, sent=0,trys=10;
        if(length>GCFG->_pool.buffsize)
        {
            tout += 1000 * (length / GCFG->_pool.buffsize);
        }

        while(length>0 && --trys>0)
        {
            shot = SSL_write(_ssl, buff + sent, length);
            if(shot>0)
            {
                sent+=shot;
                length-=shot;
                usleep(0xFF);
                continue;
            }
            if(shot==0)return length;

            int error = SSL_get_error(_ssl, 0);

            if(error==SSL_ERROR_WANT_WRITE||
                    error==SSL_ERROR_SSL||
                    errno==11)
            {
                usleep(0xFF);
                continue;
            }
            GLOGE( "SSL_write: Error: [" << sslNerror(_ssl) <<"]");
            break;
        }
        return length;
    }
    return tcp_cli_sock::sendall((unsigned char*)buff, length, tout);
}

//-----------------------------------------------------------------------------
int TcpPipe::receive(u_int8_t* buff, int length)
{
    int rd;
    if(_ssl)
    {
        rd = SSL_read(_ssl, buff, length);
        if(rd>=0) {buff[rd]=0; return rd;}  //either closed or bytes

        int error = SSL_get_error(_ssl, 0);

        if(error == SSL_ERROR_WANT_READ||
           error == SSL_ERROR_SSL ||
           errno == 11)
        {
            return -1; // keep getting
        }
        GLOGE( "SSL read Error: [" << sslNerror(_ssl) <<"]");
        return 0;
    }

    rd = tcp_cli_sock::receive(buff, length, 0, 0);
    if(rd>0) buff[rd]=0;
    return rd; //return 0 connclosed, -1 no bytes, >0 bytes received
}

int TcpPipe::ssl_connect(const Ctx* pc)
{
    assert(_ssl);

    if(-1==SSL_connect(_ssl))
    {
        int error = SSL_get_error(_ssl, 0);
        if  (error == SSL_ERROR_WANT_READ ||
             error == SSL_ERROR_WANT_WRITE ||
             error == SSL_ERROR_SSL ||
             errno == 11)
        {
            return 1; //keep going
        }
        return 0; //cannot connect
    }
    return 1;
}


bool TcpPipe::setconnected(const Ctx* pc)
{
    if(pc->_pcsl_ctx)
    {
        if(!_create_ssl(pc->_pcsl_ctx))
            return false;
    }
    tcp_cli_sock::setconnected();
    return true;
}

bool TcpPipe::setaccepted(const Ctx* pc)
{
    if(pc->_pssl_ctx)
    {
        if(!_create_ssl(pc->_pssl_ctx))
            return false;
    }
    return true;
}

bool TcpPipe::_create_ssl(SSL_CTX* pctx)
{
    assert(_ssl==0);
    _ssl = SSL_new(pctx);
    if(_ssl)
        SSL_set_fd(_ssl, socket());
    return _ssl!=0;
}



int TcpPipe::ssl_accept(const Ctx* pc)
{
    assert(_ssl);
    //set_blocking(1);
    if(SSL_accept(_ssl)<=0)
    {
        int error = SSL_get_error(_ssl, 0);
        if  (error == SSL_ERROR_WANT_READ ||
             error == SSL_ERROR_WANT_WRITE||
             error == SSL_ERROR_SSL ||
             errno == 11)
        {
            return -1; //keep going
        }
        GLOGE("SSL_Accept: " << sslNerror(_ssl));
        return 0; //cannot connect
    }
    return 1; //connected
}



