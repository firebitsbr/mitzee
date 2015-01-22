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


#ifndef CONTEXT_H
#define CONTEXT_H

#include <string>
#include <map>
#include <vector>
#include <sock.h>
#include "main.h"
#include "httprequest.h"
#include "config.h"
#include "modules.h"
#include <modiface.h>
#include "sslcrypt.h"
//-----------------------------------------------------------------------------
class ClientThread;
class SrvSock;
class Context;
class SslCrypt;
typedef int (Context::*PFN_HANDLE)(CAN_RW);

//-----------------------------------------------------------------------------
class TcpPipe : public tcp_cli_sock
{
public:
    TcpPipe():tcp_cli_sock(),_ssl(0){}
    virtual ~TcpPipe(){}
    void destroy(){
        if(_ssl){
            SSL_set_fd(_ssl, 0);
            SSL_free(_ssl);
        }
        tcp_cli_sock::destroy();
    }

    int sendall(kchar* buff, int length, int tout=1000){
        if(_ssl){
            int   shot, sent=0,trys=10;
            while(length>0){
                shot = SSL_write(_ssl, buff + sent, length);
                if(shot <= 0){
                    if(errno == EAGAIN && trys-->0){
                        usleep(1000);
                        continue;
                    }
                    tcp_cli_sock::destroy();
                    SSL_set_fd(_ssl, 0);
                    break;
                }
                sent+=shot;
                length-=shot;
            }
            return length;
        }
        return tcp_cli_sock::sendall((unsigned char*)buff, length, tout);
    }

    int receive(char* buff, int length, int port=0, kchar* ip=0){
        if(_ssl){
            int rd= SSL_read(_ssl, buff, length);
            if(rd <=0){
                cout << sslNerror();
                tcp_cli_sock::destroy();
                SSL_set_fd(_ssl, 0);
            }
            return rd;
        }
        return tcp_cli_sock::receive(buff, length, port, ip);
    }
    void attach_ssl(SSL_CTX* sslctx){
        _ssl = SSL_new(sslctx);
        if(_ssl){
            SSL_set_fd(_ssl, socket());
            SSL_accept(_ssl);
        }
    };
private:
    SSL     *_ssl;
};

//-----------------------------------------------------------------------------
class Context : public CtxMod
{
public:
    friend class CtxMod;
    Context();
    Context(const SrvSock* parent);
    virtual ~Context();
    virtual int  socket_write(kchar*);
    virtual int  socket_write(kchar*, size_t len);
    virtual int  socket_read(char* ,size_t len);
    virtual void get_report(std::ostringstream& s);
    virtual void log_string(kchar*, ...);
    int     set_fd (ClientThread* pt, fd_set& rd, fd_set& wr);
    size_t  is_fd_set (fd_set& rd, fd_set& wr);
    int   clear_fd(fd_set& rd, fd_set& wr, bool time2check, time_t secs);
    const Conf::Vhost* host_conf(){return _pvh;}
    int  spin(CAN_RW rw);
    int  init_spin(CAN_RW rw);
    TcpPipe&  sck() { return _sock;}
    void    clear() { _sock.destroy();
        if(_p_so_handler)
            _p_so_handler->destroy();
    }
    const Req& info_req()const {return _hdr;}
    bool was_active(){bool br =  (_sendrecs[0]!=_sendrecs[1]);
                      _sendrecs[0]=_sendrecs[1]; return br;}
    ClientThread* thread()const{return _pt;}
    void  fill_ip(char*)const;
    void set_connected(SSL_CTX*);
private:
    size_t _rec_some(char* buff, size_t sz);
    bool   _config_by_header();

private:
    int  _spin_rec_hdr(CAN_RW rw);
    int  _spin_rec_post(CAN_RW can);
    int  _spin_module(CAN_RW  rw);

    const SrvSock      *_plisteener;
    ClientThread       *_pt;
    const Conf::Vhost  *_pvh;
    IModule*           _p_so_handler;
    bool               _mod_fds;
    const SoEntry*     _rp_so;
    PFN_HANDLE         _pspin_foo;
    size_t             _rec_index;
    size_t             _hdr_bytes;
    int                _done;
    bool               _connected;
    size_t             _msecs_up;
    time_t             _last_time;
    time_t             _start_time;
    size_t             _unicid;
    size_t             _sendrecs[2];
    TcpPipe            _sock;
    string             _request;
    Req                _hdr;
    bool               _was_configd;
};

#define ERR_START "<br><pre><font color='red'>"
#define ERR_END "</font></pre><br>"

#endif // CONTEXT_H
