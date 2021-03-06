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

#ifndef TTYPTMDEV_H
#define TTYPTMDEV_H

#include <os.h>
#include <sock.h>
#include <iostream>
#include "ttyptmdev.h"
#include "main.h"

class clicontext : public OsThread
{
public:
    clicontext(int  sfd);
    virtual ~clicontext();
    void thread_main();
    void _post_thread_foo()
    {
        delete this;
    }
    int start_thread(){
#ifdef USE_THREAD
        return OsThread::start_thread();
#else
        //call direct, we fork a nre process.
        thread_main();
#endif
        return 0;
    }
    int forkproc();
private:

    bool _proc_ttyout(char* shot, int bytes, const std::string& prompt, const char* eol);

private:
    tcp_cli_sock    _c;
    ttyio           _t;
    pid_t           _pid;
};

#endif // TTYPTMDEV_H
