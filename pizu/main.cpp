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


#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include "os.h"
#include "main.h"
#include "config.h"
#include "listeners.h"
#include "ctxqueue.h"
#include "context.h"
#include "threadpool.h"
#include "modules.h"
#include <strutils.h>
#include "sslcrypt.h"


// /usr/bin/valgrind --tool=memcheck  -v --leak-check=full  --track-origins=yes --show-reachable=yes /home/marius/C++/marius/bin/Debug/marius
using namespace std;

bool __alive = true;

int single_instance_dmn(int nargs, char* vargs[], const char* pname);


void ControlC (int i)
{
    __alive = false;
    printf("^C. please wait. terminating threads\n");
    sleep(3);
}

//---------------------------------------------------------------------------------------
// broken pipe
void ControlP (int i)
{
    printf("P-Signal %d \n", i);
}

Listeners* __pl; //ugly
ThreadPool*  __tp;

int main(int nargs, char * vargs[])
{
    if(!single_instance_dmn(nargs, vargs,"pizu")) {
        return 0;
    }
    system("sync");
    signal(SIGINT,  ControlC);
    signal(SIGABRT, ControlC);
    signal(SIGKILL, ControlC);
    signal(SIGTRAP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    Conf   f;
    f.load("pizu.conf");
    GCFG = &f;
    Modules m;
    __modules = &m; //as well

    if(!m.is_not_empty()) {
        std::cout << "Cannot load modules. At least libhtml_mod.so should be build. \n";
        return -1;
    }

    do {

        ThreadPool  tpa(false); //asyncronous. replies that flows
        Listeners   thel(&tpa);

        __pl = &thel;  // ugly
        __tp = &tpa;
        if(thel.start_thread()!=0)
            break;
        if(tpa.start_thread()!=0)
            break;

        while( thel.san() && __alive) {
            usleep(0xFFFF);
        }

    } while(__alive);

    __tp = 0;
    __pl = 0;

    printf("ALL OBJECTS GONE \n");
    return 0;
}

int is_running(const char* pname)
{
    /*
    char fname[128];

    sprintf(fname,"/tmp/%s.pid", pname);
    int pid_file = open(fname,  O_CREAT | O_RDWR, 0666);
    if(pid_file > 0) {
        return 0;
    }
    return 1;
    */
    return 0;
}

int single_instance_dmn(int nargs, char* vargs[], const char* pname)
{
    pid_t pid = 0;
    pid_t sid = 0;

    bool running = is_running(pname);
    if(nargs!=2) {
        if(running) {
            printf("process already running\n");
            return 0;
        }
        printf("usage: pizu start/stop\n");
    }

    if(nargs==2) {
        if(!strcmp(vargs[1],"stop"))
        {
            if(running) {
                printf ("stopping");
                system("pkill pizu");

            }
            return 0;
        }

        if(running) {
            printf("process already running");
            return 0;
        }

        if(!strcmp(vargs[1],"start")) {

            pid = fork();

            if (pid < 0) {
                printf("cannot daemonize!\n");
                exit(1);
            }

            if (pid > 0) {
                printf("pid of child process %d \n", pid);
                exit(0);
            }

            umask(0);
            sid = setsid();
            if(sid < 0) {
                printf("cannot set session sid !\n");
                exit(1);
            }
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }
    }
    return 1;
}
