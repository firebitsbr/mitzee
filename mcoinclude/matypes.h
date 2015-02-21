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


#ifndef MATYPES_H
#define MATYPES_H

#include <stddef.h>
#include <os.h>
#include <sock.h>

struct DnsCommon
{
    char        header;     //'D
    int32_t     sizee;
    int32_t     sequence;   // ++
    int32_t     now;        // time
    u_int32_t   client;     // client ip on DNS (this can be internal IP if dns runs on personal net)
    u_int32_t   clientpub;  // client ip on PROXY
    u_int32_t   prxip;      // prx that is new
    u_int32_t   domainip;   // ip that was replaced
    char        hostname[128];
} __attribute__((packed));


#define _ERR(x)   (-x)

#define  _CONTINUE 1
#define _DONE  0



#endif //
