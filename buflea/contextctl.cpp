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
#include <matypes.h>
#include "threadpool.h"
#include "ctxthread.h"
#include "listeners.h"
#include "contextctl.h"
#include "dnsthread.h"


/*
    PRIVATE SERVER AUTH PROTOCOL
*/

/*
    PHP authentication page comes on this request
*/
//-----------------------------------------------------------------------------
CtxCtl::CtxCtl(const ConfPrx::Ports* pconf, tcp_xxx_sock& s):Ctx(pconf, s)
{
    _tc= 'A';
    _mode=P_CONTORL; //dns ssh
    LOGI("ACL connect from:" << IP2STR(_cliip));
    Ctx::_init_check_cb((PFCLL)&CtxCtl::_pend);
}

//-----------------------------------------------------------------------------
CtxCtl::~CtxCtl()
{
    //dtor
}

void CtxCtl::send_exception(const char* desc)
{

}

int CtxCtl::_s_send_reply(u_int8_t code, const char* info)
{
    Ctx::_s_send_reply( code, info);
    return 0;
};

int  CtxCtl::_close()
{
    destroy();
    return 0;
}

CALLR  CtxCtl::_pend()
{
    _rec_some();
    size_t  nfs = _hdr.bytes();
    if(nfs > 5) //&& lr==0/*have bytes and remove closed*/)
    {
        if(!_postprocess())
        {
            return R_KILL;
        }
    }
    _hdr.clear();
    return R_CONTINUE;
}

bool  CtxCtl::_postprocess()
{
    std::string  response="N/A";
    char* pb = (char*)_hdr.buf();
    if(pb[0])
    {
        response="OK";
        GLOGI("--> ACL " << pb );

        switch(pb[0])
        {
        case '#': //raw send 'L'
            return true;
            break;
        case 'L': //raw send 'L'
            __db->reload();
            break;
        case 'B': // raw SEND A/Rw.x.y.z.:ppp
        case 'X': // raw SEND A/Rw.x.y.z.:ppp
        case 'A': // raw SEND A/Rw.x.y.z.:ppp
        case 'R': //raw send 'R'
            __db->instertto(string(pb+1));
            break;
        case 'D': // Raw SEND   <__packed__ DnsCommon>
            {
                // dns server sends the IP that was obtained durring DNS request
                // we hold this in association with the IP that asked for that IP
                DnsCommon* pc = reinterpret_cast<DnsCommon*>(pb);
                __dnsssl->queue_host(_cliip, *pc);
                // add to session as well.
                std::string ip("A");
                ip+=IP2STR(htonl(pc->client));
                GLOGI("DNS for client:" << ip << " [ to connect to ] "  << IP2STR(pc->domainip));
                ip+=":0";
                __db->instertto(ip);
            }
            return true;
        case 'Q': // Raw SEND    U.x.y.z:PPP
        {
            const SADDR_46* pad = reinterpret_cast<const SADDR_46*>(pb+1);
            if(__db->is_client_allowed(*pad))
            {
                response="YES";
            }
            else
            {
                response = "NO";
            }
        }
        break;
        case 'T': // '/Tx.y.z.k.ppp' addr of next proxy. dynamically chnage the next proxy ip and port
            {
                std::string ip = pb+1;
                if(ip.find(".")!=string::npos || ip.find(":")!=string::npos)
                {
                    for(auto & it : GCFG->_listeners)
                    {
                        ConfPrx::Ports& p = (ConfPrx::Ports&)it;
                        if(!p.redirect.empty()) //was set
                        {
                            p.redirect = ip;
                            p.toaddr = fromstringip(ip);
                        }
                    }
                    response="OK";
                }
                else
                {
                    response="FAIL";
                }
            }
            break;
        case 'G': //http GET send   'http://proxyip:port/$#.#.#.#'  $=B, X, A, R
            if(!::strncmp(pb,"GET / HTTP", 10) || !::strncmp(pb,"GET /M", 6))
            {
                __tp->dump_metrics(_sock);
            }
            else //parse above docs  GET /$?param
            {
                char* eol =(char*)::strchr(pb,'\r');
                if(eol)
                {
                    *eol=0;

                    eol =(char*)::strchr(pb+5,' ');
                    if(eol)
                        *eol=0;

                    switch(pb[5])
                    {
                    case 'L':   // 'GET / HTTP'
                        __db->reload();
                        break;
                    case 'A': // 'GET /Ax.y.z.k.ppp' add/remove ip in session
                    case 'R': // 'GET /Rx.y.z.k.ppp' add/remove ip in session
                    case 'B': // 'GET /Bx.y.z.k.ppp' add/remove ip in banned
                    case 'X': // 'GET /Xx.y.z.k.ppp' add/remove ip in banned
                        __db->instertto(string(pb+5));
                        break;
                    case 'T': // 'GET /T?x.y.z.k:ppp' addr of next proxy
                        {
                            std::string ip = pb+7;
                            if(ip.find(".")!=string::npos || ip.find(":")!=string::npos)
                            {
                                for(auto & it : GCFG->_listeners)
                                {
                                    ConfPrx::Ports& p = (ConfPrx::Ports&)it;
                                    if(!p.redirect.empty()) //was set
                                    {
                                        p.redirect = ip;
                                        p.toaddr = fromstringip(ip);
                                    }
                                }
                                response="OK";
                            }
                            else
                            {
                                response="FAIL";
                            }
                        }
                        break;
                    case 'Q': // 'GET /Q?x.y.z.k:ppp'  check if is allowed or not
                    {
                        const SADDR_46 pad = fromstringip(pb+7);
                        if(__db->is_client_allowed(pad))
                        {
                            response="YES";
                        }
                        else
                        {
                            response = "NO";
                        }
                    }
                    break;
                    case 'D':  // set for transparent proxy the host where we connect
                    {
                        // ?D=C.L.I.EIP,H.O.S.TIP|hostname:HOSTPORT,
/*
                        int32_t     sequence;
                        time_t      now;
                        u_int32_t   client;
                        u_int32_t   hostip;
                        u_int16_t   port;
                        char        hostname[128];
                        u_int16_t   hostport;
*/
                    }
                    break;
                    default:
                        response="N/A";
                        break;
                    }
                }
                else
                {
                    response="N/A";
                }
            }
            break;
        default:
            response="N/A";
            break;
        }
    }
    _sock.send(( char*)response.c_str(),response.length());
    _clear_header();
    return false;
}


bool CtxCtl::_new_request(const u_int8_t* buff, int bytes)
{
    return false;
}

#endif // #ifdef _PERSONAL
