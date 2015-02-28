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

#ifndef PROC_INST
#define PROC_INST

#include <signal.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <strutils.h>

typedef  void (*CBSIG)(int);

class procinst
{
public:

    procinst( int sigsend, int sigsign, CBSIG foo, int to=2):_alive(true),
                                        _timout(to),
                                        _owner(false),
                                        _sigend(sigsend),
                                        _sigign(sigsign),
                                        _foo(foo)
    {
        //std::cout << "procinst " << getpid() <<","<< getppid() << " STARTING \r\n";
    }
    ~procinst()
    {
        ::unlink(_quitfile.c_str());
        if(_owner)
            ::unlink(_lockfile.c_str());
        _alive=false;
        //std::cout << "procinst " << getppid() <<" ---> "<< getpid() << " EXITING \r\n";
    }

    int kill()
    {
        _alive=false;
//        printf ("stopping...\n");
        std::string sys="touch "; sys+=_quitfile;
        system(sys.c_str());
        sleep(_timout);
        return 0;
    }
    bool owner()const{return _owner;}

    bool alive(){
        if(::access(_quitfile.c_str(),0)==0)
        {
            _alive = false;
        }
        return _alive;
    }
    int instance(int nargs, char * vargs[], const char* nameadd,  bool single, bool d)
    {
        char locname[256] = {0};
        const char* pappname = ::rstrchr(vargs[0],'/');

        if(pappname==0)
            pappname=vargs[0];
        else
            pappname++;

        ::sprintf(locname, "%s%s", pappname, nameadd);

        _lockfile = getuid() == 0 ? "/var/run/" : "/tmp/";
        _lockfile += locname;
        _quitfile = "/tmp/"; _quitfile+=locname; _quitfile+=".stop";

        if(single)
        {
            if(single_instance_dmn(nargs,vargs)!=0)
            {
                return -1;
            }
            if(d)
                daemon(1,0);
        }
        int sig;
        for(int i=0;i<32;i++)
        {
            sig = 1<<i;
            if((sig & _sigend) == sig)
                ::signal(sig,  _foo);
        }

        for(int i=0;i<32;i++)
        {
            sig = 1<<i;
            if((sig & _sigign) == sig)
                ::signal(sig,  SIG_IGN);
        }
        return 0;
    }
protected:
    int is_running()
    {
    #ifdef DEBUG
        return 0;
    #endif
        int pdf = open(_lockfile.c_str(), O_CREAT | O_RDWR, 0666);
        int rc = ::flock(pdf, LOCK_EX | LOCK_NB);

        if(rc == 0)
        {
            //std::cout << "Locking file" << _lockfile << "\n";
            _owner=true; // is it required !?!
            return 0;
        }
        return  1;

    }

    int single_instance_dmn(int nargs, char* vargs[])
    {
        bool running = is_running();
        if(nargs!=2) {
            if(running) {
                printf("process already running\n");
                return -1;
            }
        }

        if(nargs>=2) {
            if(!strcmp(vargs[1],"stop")) {
                if(running) {
                    printf ("stopping...\n");
                    std::string sys="touch "; sys+=_quitfile;
                    system(sys.c_str());
                }
                return -1;
            }

            if(running) {
                printf("process already running\n");
                return -1;
            }
        }
        return 0;
    }


public:
    bool    _alive;
    int     _timout;
    std::string  _lockfile;
    std::string  _quitfile;
    bool    _owner;
    int     _sigend;
    int     _sigign;
    CBSIG    _foo;
};

#endif // PROC_INST
