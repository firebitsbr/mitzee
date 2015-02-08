/**
# Copyright (C) 2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - coinscode.com - N/A
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

#include "udpsock.h"
#include "config.h"

udpsock::udpsock()
{
    //ctor
}

udpsock::~udpsock()
{
    //dtor
}

bool udpsock::ensure()
{
    if(!isopen())
    {
        const char* eth_iface = PCFG->_srv.addr.empty() ? 0 : PCFG->_srv.addr.c_str();

        destroy();
        if(create(PCFG->_srv.port, 0, eth_iface)>0 && bind()>0)
        {
            set_blocking(0);
            GLOGI("UDP waiting on::" <<PCFG->_srv.addr <<":"<<PCFG->_srv.port)
            return true;
        }
        sleep(1);
        destroy();
        return false;
    }
    return true;
}


int udpsock::pool()
{
    struct timeval  tv;
    int             ret;
    fd_set          r,w;
    const SOCKET    nsock = socket();
    const int       ndfs = nsock + 1;

    FD_SET(nsock, &r);
    FD_SET(nsock, &w);
    tv.tv_sec = 0;
    tv.tv_usec = 0x1FF;


    ret = select(ndfs, &r, &w, 0, &tv);
    if(ret<0)
    {
        destroy();
        return 0;
    }
    if(ret > 0 && FD_ISSET(nsock, &r))
    {
        return 1;
    }
    return 0;//
}

int udpsock::rec_all(uint8_t * buff, int bytes, SADDR_46&  fromaddr)
{

    SADDR_46 a;
    int sz = receive((unsigned char*)buff, bytes, a);
    if(sz>0)
    {
        fromaddr=a;  //to properly populate extended (buggy)
        return sz;
    }
    return sz; //0 con closed, -1 continue
}






