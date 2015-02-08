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

#ifndef THEAPP_H
#define THEAPP_H

#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <iostream>
#include <sock.h>
#include <map>
#include <strutils.h>
#include "udpsock.h"
#include "watcher.h"
#include <minidb.h>
#include <sslcrypt.h>


class theapp
{
public:
    static Watcher*     __pw;
    static u_int32_t    __dns_tout;
    static int          __gerrors;
    static int          __sessionprx;
public:
    theapp();
    virtual ~theapp();
    static void ControlC (int i);
    int run();

protected:
    void _flush(time_t now);
    void _io(Message& m);
    void _from_dns(const SADDR_46& inaddr, Message& m);
    void _from_client(const SADDR_46& inaddr, Message& m);
    void _cli_send_fromcache(const SADDR_46& inaddr, Message& m);


private:
    std::map<u_int16_t, Context>  _ctexes;
    udpsock                       _buzy_socket;
    Watcher                       _tp;
    DbAccess                      _db;
    u_int16_t                     _gseq;

};

#endif // THEAPP_H
