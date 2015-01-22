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

#include "listeners.h"
#include "ctxthread.h"
#include <modiface.h>
#include "context.h"

#define DTD 2;

int  Context::socket_write(kchar* str)
{
    return socket_write(str, strlen(str));
}

int  Context::socket_write(kchar* buff, size_t len)
{
    int sz = _sock.sendall(buff, len, 4000);

    if(sz!=0)
    {
        _sock.destroy();
        return -1;
    }
    if(sz>0)++_sendrecs[0];
    _pt ? _pt->checkin(0,len):(void)0;
    return sz;
}

int  Context::socket_read(char* buff, size_t len)
{
    int sz = _sock.receive(buff, len);
    if(sz>0)++_sendrecs[0];
    _pt ? _pt->checkin(sz,0):(void)0;
    return sz;
}


void    Context::log_string(kchar*, ...)
{

}
extern Listeners* __pl; //ugly

void    Context::get_report(std::ostringstream& ss)
{
    __pl->get_report(ss);
}

