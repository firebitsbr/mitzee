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

#ifndef _PERSONAL

#include <assert.h>
#include <iostream>
#include <sstream>
#include <sock.h>
#include <strutils.h>
#include "main.h"
#include "config.h"
#include "context.h"
#include <consts.h>
#include "threadpool.h"
#include "ctxthread.h"
#include "listeners.h"
#include "contexthttp.h"
#include "dnsthread.h"


/*
    SOCKS HTTP PROXY PROTOCOL
*/



//-----------------------------------------------------------------------------
CtxHttp::CtxHttp(const ConfPrx::Ports* pconf, tcp_xxx_sock& s):
    Ctx(pconf, s),_hdr_has_open(false),_tflush(128),_loops(0)
{
    _tc= 'H';
    _mode= P_HTTP;
    Ctx::_init_check_cb((PFCLL)&CtxHttp::_parse_header);
}

//-----------------------------------------------------------------------------
CtxHttp::~CtxHttp()
{
    //dtor
}
//-----------------------------------------------------------------------------
int  CtxHttp::_get_hdr()
{
    _rec_some();
    int code = _hdr.parse();
    if(0==code)
    {
        return 0; //get more
    }
    return 1; //header complette
}

CALLR  CtxHttp::_parse_header()
{
    if(_get_hdr()==0)
        return R_CONTINUE;

    assert(!_hdr_has_open);
    //const bool con = _rock.check_connection();
    if(_hdr._nhost)
    {
        char        host[512]= {0};
        char        referer[512]= {0};
        char        port[4] = "80";


        ::strncpy(host, _hdr.get_host().c_str(), 511);
        if(_hdr._nreferer && _hdr._nreferer_end)
            ::strncpy(referer, _hdr.get_referer().c_str(), 511);

        char* pp = strchr(host,':');
        if(pp)
        {
            *pp++=0;
        }
        else
        {
            pp = port;
        }

        _raddr = __db->dnsgetip(host);
        _raddr.set_port(::atoi(pp));

        LOGT("http-hdr: ->" << host << " " << IP2STR(_raddr));
        LOGH("C: hdr ["<<_hdr.bytes() << "]\n[" << _hdr.buf() << "]\n");

        if(_raddr.ip4()==0)
        {
            throw Mex(CANNOT_CONNECT,__FILE__,__LINE__);
        }
        if(_working )
        {
            if(_rip == _raddr)
            {
                LOGH("http-hdr ->:"<< IP2STR(_rip) );
                return _r_send_header();
            } // else rip!=ap
            return _overwrite_connection(_raddr);
        }
        assert(!_rock.isopen());

        if(_is_access_blocked(_raddr, host, referer))
        {
            LOGW("access is blocked to: " << IP2STR(_raddr));
            return R_CONTINUE;
        }

        _set_rhost(_raddr, host, referer);
        _rip     = _raddr;
        return _rock_connect(_rock);
    }
    else
    {
        //we could have a raw ssl connection
        throw Mex(CANNOT_PARSE_HTTP,__FILE__,__LINE__);
    }
    if(_working)
        return _r_send_header();

    throw Mex(CANNOT_PARSE_HTTP,__FILE__,__LINE__);
}

CALLR  CtxHttp::_empty_rock()
{
    if(--_tflush>0 )
    {
        if(_get_from_rock())
        {
            _tflush = 128;
        }
        usleep(64);
        return R_CONTINUE;       //got someting, keep receiveing on this connection
    }
    _working = false;
    _negok = false;
    _hdrsent = false;
    _destroy_rock();
    _set_rhost(_rip);
    return _rock_connect(_rock);
}


//-----------------------------------------------------------------------------
CALLR  CtxHttp::_r_send_header()
{
    if(_hdr.bytes())
    {
        if(_hdr.has_open == true)
        {
            _hdr_has_open = true;
            LOGT(" HTTPS / 'CONNECT' found");
            //
            // reply connected Proxy connection established
            //
            _sock.sendall((const u_int8_t*)HTTP_200,  strlen(HTTP_200), SS_TOUT);
        }
        else if(_hdr.bytes() != 0) // no open, the proxy is accessed as a web server
        {
            _hdr.prep_doc();
            _rock.sendall(_hdr.buf(), _hdr.bytes(), SS_TOUT);
            LOGT(" SENT HDR TO RADDR: ["<<_hdr.bytes() <<"]\n[" << _hdr.buf() <<"]\n");
        }
        _clear_header();
    }
    if(_working)
        return R_CONTINUE;
    return Ctx::_r_send_header();
}

//-----------------------------------------------------------------------------
int  CtxHttp::_s_send_reply(u_int8_t code, const char* info)
{
    Ctx::_s_send_reply( code, info);

    if(SUCCESS==code)
        return 1;

    char msg[800];
    sprintf(msg,"%s <html><h1>proxy error: %s (%s)</h1></html>", HTTP_400,
                                                                 socks_err(code), info ? info : "*");
    _sock.sendall((const u_int8_t*)msg, strlen(msg), SS_TOUT);
    return 1;
}

//-----------------------------------------------------------------------------
CALLR  CtxHttp::_transfer()
{
    if(_hdr_has_open)
    {
        return Ctx::_transfer();
    }
    return _sr_http_read_write();
}

//-----------------------------------------------------------------------------
CALLR  CtxHttp::_sr_http_read_write()
{
    if(_sock.set() & 1)
    {
        if(0==_parse_header())
        {
            return R_CONTINUE;
        }
    }
    return _get_from_rock();
}

CALLR  CtxHttp::_get_from_rock()
{
    if(!_rock.isopen())
        return R_CONTINUE; //keep wiating the pending connect

    int         rsz;
    u_int8_t*   buff = _pt->buffer(rsz);

    if(_rock.set() & 0x00000001)
    {
        rsz = _rock.receive(buff, rsz);
        if(rsz > 0)
        {
            _stats._temp_bytes[BysStat::eIN]+=rsz;
            _sock.sendall(buff, rsz,  SS_TOUT);
            _stats._temp_bytes[BysStat::eOUT]+=rsz;
        }
    }
    return rsz==0?R_KILL:R_CONTINUE;;
}

void  CtxHttp::send_exception(const char* desc)
{
    if(_sock.isopen() && _sock.is_really_connected())
    {
        _sock.sendall((const u_int8_t*)HTTP_200, strlen(HTTP_200),SS_TOUT);
        _sock.send(desc, strlen(desc));
    }
}

bool CtxHttp::_new_request(const u_int8_t* buff, int bytes)
{
	if( !::memcmp(buff,"CONNECT", 7) ||
        !::memcmp(buff,"GET", 3) ||
        !::memcmp(buff,"POST", 4) ||
        !::memcmp(buff,"HEAD", 4))
	{
		_clear_header();
		_hdr.append((const char*)buff, bytes);
		_pcall = (PFCLL)&CtxHttp::_parse_header;
		return true;
	}
	return false;
}

#if 1
CALLR  CtxHttp::_overwrite_connection(const SADDR_46& ap)
{
    LOGT(IP2STR(_cliip) << " -|o- " << IP2STR(_rip));
    _rip    = ap;
    _tflush = 128;
    _get_from_rock();
    _pcall=(PFCLL)&CtxHttp::_empty_rock;
    return R_CONTINUE;
}

#else

#endif


#endif //_PERSONAL
