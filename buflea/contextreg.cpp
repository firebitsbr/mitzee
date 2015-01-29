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
#include <matypes.h>
#include "threadpool.h"
#include "ctxthread.h"
#include "listeners.h"
#include "contextreg.h"
#include "dnsthread.h"


/*
    PRIVATE SERVER AUTH PROTOCOL
*/

/*
    PHP authentication page comes on this request
*/
//-----------------------------------------------------------------------------
CtsReg::CtsReg(const ConfPrx::Ports* pconf, tcp_xxx_sock& s):Ctx(pconf, s)
{
    _tc= 'A';
    _mode=P_CONTORL; //dns ssh
    LOGI("REG connect from:" << IP2STR(_cliip));
    Ctx::_init_check_cb((PFCLL)&CtsReg::_pend);
}

//-----------------------------------------------------------------------------
CtsReg::~CtsReg()
{
    //dtor
}

void CtsReg::send_exception(const char* desc)
{

}

int CtsReg::_s_send_reply(u_int8_t code, const char* info)
{
    Ctx::_s_send_reply( code, info);
    return 0;
};

int  CtsReg::_close()
{
    destroy();
    return 0;
}

CALLR  CtsReg::_pend()
{
    int  lr = _rec_some();
    size_t  nfs = _hdr.bytes();
    if(lr > 5 || nfs > 5) //&& lr==0/*have bytes and remove closed*/)
    {
        if(!_postprocess())
        {
            return R_KILL;
        }
    }
    _hdr.clear();
    return R_CONTINUE;
}

bool  CtsReg::_postprocess()
{
    // GET /R?PUB:x.x.x.x,PRIV:y.y.y.y HTTP 1.1
    //
    // add tto payers
    GLOGI(_hdr.buf());
    _sock.sendall((const uint8_t*)"SOCKET CONNECT OK",17,  SS_TOUT);
    return false;
}


bool CtsReg::_new_request(const u_int8_t* buff, int bytes)
{
    return false;
}