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
#include <procinst.h>
#include <iostream>
#include <sstream>

#define USE_THREAD

extern procinst              Inst;

class procwrapper
{
public:
    procwrapper()
    {

        _thispid=getpid();
        _thisppid=getppid();
    }

    ~procwrapper()
    {
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
        }
        else
        {
            _thispid=getpid();
            _thisppid=getppid();
        }
        return p;
    }

    void killit(pid_t pid)
    {
        int rv;
        wait(&rv);
    }

    void exitproc(int code, const char* msg)
    {
        exit(code);
    }

    pid_t             _thispid;
    pid_t             _thisppid;
};


class custerr
{
    std::stringstream   _stream;
public:
    custerr(){}
    custerr(const custerr& e)
    {
        _stream << e._stream.str();
    }
    std::stringstream& stream(){return _stream;}
    const char* str(){
        return _stream.str().c_str();
    }
};

#define THROW(x) do{custerr erro;\
    erro.stream() << __FUNCTION__ << ":"<< strerror(errno) << "(" << errno << ")/" << x << "\n";   \
    throw erro;}while(0);


extern procwrapper Proco;


#endif //_MAIN_H
