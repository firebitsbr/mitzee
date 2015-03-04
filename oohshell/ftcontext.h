/**
# Copyright (C) 2007-2015 s(mariuschincisan@gmail.com) - coinscode.com - N/A
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

#ifndef FTPTMDEV_H
#define FTPTMDEV_H

#include <os.h>
#include <sock.h>
#include <iostream>
#include "ttyptmdev.h"
#include "main.h"

class ftcontext : public OsThread
{
public:
    ftcontext(int  sfd);
    virtual ~ftcontext();
    void thread_main();
    int  thread_main(char pg, const string& f1,  const string& f2);

    void _post_thread_foo()
    {
        delete this;
    }
    int start_thread(){
        return OsThread::start_thread();
    }

private:
    void _send_header(char over=0);
    int _read_header(char* buff, int bytes);

    void _client_pg(char* loco, int cap);
    void _server_pg(char* loco, int cap);
    void _thread_main();

private:
    tcp_cli_sock    _c;
    std::string     _f1;
    std::string     _f2;
    char            _pg;
    int             _err;
    FILE*           _pf;
    uint64_t        _len;
    uint64_t        _remain;
    uint64_t        _transf;
    bool            _isclient;
    char            _temp[128];
    fd_set           _rd;
    fd_set           _wr;
};

#endif // TTYPTMDEV_H
