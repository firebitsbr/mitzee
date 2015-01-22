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
#include "contextpass.h"
#include "dnsthread.h"


/*
    SOCKS TRANSPARENT PROXY PROTOCOL, ALLWAYS CONNECT TO HTML HEADER
    THEN RECEIVES AND OVERWRITES THE HOSTNAME ALL OVER WITH PROXY IP
    TO KEEP GET  THE CONTENT
*/


//-----------------------------------------------------------------------------
CtxPasstru::CtxPasstru(const ConfPrx::Ports* pconf, tcp_xxx_sock& s):
    Ctx(pconf, s)
{
    _tc= 'P';
    _mode= P_PASSTRU;
    _pcall = (PFCLL)&CtxPasstru::_overwrite_addr;
}

//-----------------------------------------------------------------------------
CtxPasstru::~CtxPasstru()
{
    //dtor
}

//-----------------------------------------------------------------------------
// wait for  all the header
CALLR  CtxPasstru::_overwrite_addr()
{
    if(R_KILL==_overwrite_hosts())return R_KILL;
    return _rock_connect(_rock);
}


CALLR  CtxPasstru::_r_send_header()
{
    _pcall=(PFCLL)&CtxPasstru::_transfer;
    return R_CONTINUE;
}

CALLR CtxPasstru::_transfer()
{
    return Ctx::_transfer();
}


CALLR  CtxPasstru::_overwrite_hosts(const char* host)
{
    if(_pconf->toaddr.empty())
    {
        LOGD("The context does not have config redirs. closing");
        return R_KILL;
    }
    _set_rhost(_pconf->toaddr, 0, 0);
    return R_CONTINUE;
}
