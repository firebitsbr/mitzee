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

#ifndef _MAIN_H
#define _MAIN_H

#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <set>
#include <procinst.h>

#define USE_THREAD

extern procinst              _I;

class processses
{
public:
    processses()
    {

        _thispid=getpid();
        _thisppid=getppid();
        std::cout<<"processses{ " << _thisppid << " : " << _thispid << "\n";

    }

    ~processses()
    {
        std::cout<<"~processses}" << _thisppid << " : " << _thispid << "\n";
    }

    pid_t   forkit()
    {
        pid_t p = fork();
        if(p<0)
        {
            exit(0);
            return 0; //not reached
        }
        else if(p>0) //in parent
        {
            std::cout<<"PARENT PROC: " << _thispid << "----yields--->" << p << "\n";
        }
        else
        {
            _thispid=getpid();
            _thisppid=getppid();
            std::cout<<"CHILD PROC from: " << _thisppid << "----yiel--->" << _thispid << "\n";
        }
        return p;
    }

    void killit(pid_t pid)
    {
        /*
        kill(pid, SIGTERM);

        bool died = false;
        for (int loop; !died && loop < 5 ; ++loop)
        {
            int status;
            pid_t id;
            sleep(1);
            if (waitpid(pid, &status, WNOHANG) == pid) died = true;
            {
                if (!died) kill(pid, SIGKILL);
            }
        }
        */
        int rv;
        wait(&rv);
        std::cout<<" KILLIT WAIT( "<< pid << ")\n";
    }

    void exitproc(int code, const char* msg)
    {
        std::cout<<"EXITING PROCESS: "<< msg << ": " << _thispid << "\n";
        exit(code);
    }

    pid_t             _thispid;
    pid_t             _thisppid;
};


extern processses _PS;


#endif //_MAIN_H
