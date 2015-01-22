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
#include <iostream>
#include <sstream>
#include <sock.h>
#include "main.h"
#include "config.h"
#include "context.h"
#include "threadpool.h"
#include "ctxthread.h"
#include "listeners.h"
#include "modules.h"
#include <strutils.h>
#include <consts.h>

static __thread char    HTTP_MSG[512];

static kchar HTTP_XXX[] = "HTTP/1.1 %s\r\n"
                               "Connection: close\r\n"
                               "Content-Type: text/html; charset=utf-8\r\n"
                               "Server: mariux/1.0\r\n\r\n"
                               "<H2>%s</H2><div align='right'>mariux-1.0</div>";

kchar * HTTP_ERR(kchar* hdr, kchar* msg)
{
    sprintf(HTTP_MSG, HTTP_XXX, hdr, msg);
    return HTTP_MSG;
}

static int        __ctx;
static int        __ctxc;


Context::Context(const SrvSock *pl):CtxMod(0),
    _plisteener(pl),
    _pt(0),
    _pvh(0),
    _p_so_handler(0),
    _mod_fds(), //module has fd's
    _rp_so(0),
    _pspin_foo(&Context::init_spin),
    _rec_index(0),
    _hdr_bytes(0),
    _done(_CONTINUE),
    _connected(true),
    _was_configd(false)

{
    _unicid     = __ctx++;
    ++__ctxc;
    _last_time  = _start_time;
    _msecs_up   = time(0);
    _hdr.clear();
    cout << "++++{   _psslCtx: "<< __ctxc <<"\n";
    if(__ctxc>5)
    {
        cout <<"break;";
    }
}

Context::~Context()
{
    clear();
    if(_rp_so) {
        if(0 != _p_so_handler) {
            _p_so_handler->destroy();
            kchar* cook = _p_so_handler->__cookie();
            _rp_so->_so_release(_p_so_handler, cook);
        }
    }
    cout << "----}   _psslCtx:" << __ctxc <<"\n";
    --__ctxc;
}

void Context::set_connected(SSL_CTX* pslctx)
{
    if(pslctx && _plisteener->hasSsl()) {
        _sock.attach_ssl(pslctx);
    }
}

int   Context::init_spin(CAN_RW rw)
{
    assert(_pt);
    _pspin_foo = &Context::_spin_rec_hdr;
    return _CONTINUE;
}

int   Context::spin(CAN_RW rw)
{
    try {
        _done = (this->*_pspin_foo)(rw);
    } catch(Mex& e) {
        __tp->log("%s %s", e.desc(), Mex::http_err(e.code()));
        if(_sock.isopen()) {
            kchar* p = HTTP_ERR(Mex::http_err(e.code()),e.desc());
            socket_write(p, strlen(p));
            _done = _ERR(1);
        }
    }
    return _done;
}

int Context::_spin_rec_hdr(CAN_RW can)
{
    if(can & CAN_READ) {
        size_t bytes = _rec_some(_hdr._request + _hdr_bytes,
                                Req::MAX_REQUEST - _hdr_bytes);
        _hdr_bytes += bytes;
        if(bytes && _hdr.parse(this, _hdr_bytes, _rec_index)) {
            kchar*  h =_hdr.get_hdr("HOST");

            _pvh = _plisteener->_vHoost(h);
            if(0 == _pvh) {
                // ERROR NO HOST
                throw Mex(400,__FILE__,__LINE__);
            }
            _hdr.commit_header(_pvh);

            if(_hdr._body_len) {
                int err = _hdr.get_content_data( _hdr._body, _hdr._body_len);
                _tempfile = _hdr._post_file;
                _templen  = _hdr._body_len;//_post_length;
                if(err < 0) {
                    throw Mex(400,__FILE__,__LINE__);
                }
                if( err > 0) {
                    _pspin_foo = &Context::_spin_rec_post;
                    return _CONTINUE;
                }
            }
            _config_by_header();
            _pspin_foo = &Context::_spin_module;
        }
        return _CONTINUE;
    }
    return _CONTINUE;
}

bool Context::_config_by_header()
{
    if(_was_configd)
        return true;
    _was_configd=true;

    //data for module. pointers into actual data

    _dir_cache   = GCFG->_marius.cache.c_str();
    _doc_index   = _pvh->index.c_str();
    _dir_home    = _pvh->home.c_str();             // var/www           from config
    _emethod     = _hdr._emethod;
    _url_doc     = _hdr._uri_doc;              // path/THIS  from GET
    _url_path    = _hdr._uri_dir;              // THIS/index.html from GET
    _cli_ip      = _hdr._ip;
    _hdrs        = _hdr._hdrs;
    _get         = _hdr._get;
    _cookies     = _hdr._cookie;
    ::strncpy(_settings, GCFG->_marius.plugflags.c_str(), sizeof(_settings-1));

    kchar* by_ext = ::str_getfile_ext(_url_doc ,"*");
    _rp_so = __modules->get_module(by_ext);
    if(0 == _rp_so) {
        throw Mex(400,__FILE__,__LINE__);
    }
    _p_so_handler = _rp_so->_so_get(by_ext);

    if(0 == _p_so_handler) {
        throw Mex(400,__FILE__,__LINE__);
    }
    if(!_p_so_handler->construct(this)) {
        _rp_so->_so_release(_p_so_handler, by_ext);
        _p_so_handler = 0;
        throw Mex(400,__FILE__,__LINE__);
    }
    return true;
}

int Context::_spin_rec_post(CAN_RW can)
{
    if(can & CAN_READ) {
        char buff[1024];

        size_t bytes = _rec_some(buff, 1024);
        if(0 == bytes) {
            return _CONTINUE;
        }
        int err = _hdr.get_content_data(buff, bytes);
        if(-1 == err) { //when 0 post finished
            throw Mex(400,__FILE__,__LINE__);/*bad request*/
        }

        if( err > 0) {
            return _CONTINUE;
        }
        _config_by_header();
        _pspin_foo = &Context::_spin_module;
        return _spin_module(can);
    }
    return _CONTINUE;
}

int Context::_spin_module(CAN_RW rw)
{
    return _p_so_handler->work(this, rw);
}

size_t   Context::_rec_some(char* buff, size_t sz)
{
    assert(sz > 0);
    long bytes = socket_read(buff, sz);
    if(bytes == 0) {        // remote closed connection
        if(_hdr_bytes)
            __tp->log("Socket read failure: header:",_hdr._request);
        else
            __tp->log("Socket read failure: header: (null)");
        _sock.destroy();
        throw Mex(-_sock.error(),__FILE__,__LINE__);
    } else if(bytes==-1) {
        bytes = 0;          // no bytes received
    }
    return  bytes;
}



int Context::set_fd (ClientThread* pt, fd_set& rd, fd_set& wr)
{
    _pt = pt;
    int rv = _sock.socket();
    FD_SET(rv, &rd);
    FD_SET(rv, &wr);

    if(_mod_fds && _p_so_handler) {
        rv = max (rv, (int)_p_so_handler->fd_action(eFD_SET, rd, wr));
    }
    return rv;
}

size_t  Context::is_fd_set(fd_set& rd, fd_set& wr)
{
    int    s   = _sock.socket();
    size_t can = (FD_ISSET(s, &rd) ? CAN_READ : CAN_ZERO);
    can |= (FD_ISSET(s, &wr) ? CAN_WRITE : CAN_ZERO);
    if(_mod_fds && _p_so_handler) {
        can |= (size_t)_p_so_handler->fd_action(eFD_IS, rd, wr);
    }
    return can;
}

int Context::clear_fd(fd_set& rd, fd_set& wr, bool time2check, time_t span)
{
    if(time2check) {
        if(!was_active())
            return _DONE;
    }

    if(_sock.isopen()) {
        FD_CLR(_sock.socket(), &rd);
        FD_CLR(_sock.socket(), &wr);
        if(_mod_fds && _p_so_handler) {
            _done = (int)_p_so_handler->fd_action(eFD_CLEAR, rd, wr);
        }
    } else {
        _done = _DONE;
    }
    return _done;
}

void Context::fill_ip(char* p)const
{
    strcpy(p, IP2STR(_sock.getsocketaddr()));//p);
}
