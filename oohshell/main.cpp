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



#include <iostream>
#include <os.h>
#include <sock.h>
#include <signal.h>
#include <consts.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include "ttyptmdev.h"
#include "clicontext.h"
#include "ftcontext.h"
#include "oohngine.h"
#include "main.h"


int client();
int server();


void control_c(int k)
{
    Inst.kill(0,0);
}


procwrapper Proco;
procinst Inst( SIGABRT|SIGTERM|SIGKILL|SIGTRAP,  SIGPIPE, control_c);


/**
    openprompt -c127.0.0.1:4444
    openprompt -s*:4444 <-d> or  oohshell -s *:4444  or oohshell -s
    openprompt -s192.168.1.100:4444
    openprompt -c192.168.1.100:4444
    openprompt -stop
*/


void _help()
{
    std::cout << "oohshell -s<d> *:PORT          : start server on all interfaces on port, -d run as daemon \n";
    std::cout << "oohshell -s<d> X.Y.Z.K:PORT <-d>  : start server on X.Y.Z.K interface and port, -d run as daemon \n";
    std::cout << "oohshell -c X.Y.Z.K:PORT       : connects shell client to IP:PORT \n";
    std::cout << "oohshell -cp fname@X.Y.Z.K:PORT  fname    : transfer file download \n";
    std::cout << "oohshell -cp fname fname@X.Y.Z.K:PORT     : transfer file upload \n";
    std::cout << "oohshell stop                 : stops the server instance \n";
}


int main(int n, char* v[])
{
    int err;

    Inst.reap_zobies();

    if(!::strcmp(v[1],"stop"))
        return Inst.kill(v[0], "s");

    Inst.instance(n, v, v[1], true, (0==::strcmp(v[1],"-sd"))); //limit to one instance

    oohngine engine(n,v);
    if(engine.ok())
        err=engine.run();
    else
        _help();
    return 0;
}



