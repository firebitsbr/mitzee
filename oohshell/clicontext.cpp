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
#include <signal.h>
#include "clicontext.h"
#include "main.h"

#define THIS    _pid << " / " << this << " , "

clicontext::clicontext(int  sfd)
{
    _c.attach(sfd);
    _c.set_blocking(0);
    std::cout << THIS << "clicontext(){\n";
}

clicontext::~clicontext()
{
    std::cout << THIS << "~clicontext()}\n";
    _c.destroy();
    _t.close();

}

void clicontext::thread_main()
{
    fd_set   rd;
    int     ndfs;
    timeval tv = {0,16384};
    char    shot[4096] = {0};
    int     bytes;
    bool    broken=false;
    string  user = getenv("USER");
    string  hostn = "local";
    bool    enabled=false;
    char    eol[3];
    const char* hn = getenv("HOSTNAME");
    if(hn)
        hostn=hn;
    else
    {
        char pid[32];
        ::sprintf(pid, "%d",_pid);
        hostn=pid;
    }
    string  prompt = "export PS1=";
    string pp = user; pp += "@"; pp += hostn;
    pp += getuid()==0 ? ":# " : ":$";
    prompt+=pp;
    _t.write((const char*)prompt.c_str(), prompt.length());
    int erros=8;

    _t.write("\r\n", 2);

    while(_I.alive() && !broken)
    {
        FD_ZERO(&rd);
        FD_SET(_c.socket(), &rd);
        FD_SET(_t.master(), &rd);
        ndfs =max(_c.socket(),_t.master())+1;
        tv.tv_sec = 0;
        tv.tv_usec = 0xFFF;
        int is = ::select(ndfs, &rd, 0, 0, &tv);
        if(is ==-1)
        {
            if(erros-->0)
            {
                std::cout <<THIS  <<" cannot select: " << _t.master() <<", " <<errno<<"\n";
                usleep(10000);
                continue;
            }
            std::cout <<THIS  <<" cannot select aborted: " << _t.master() <<", " <<errno<<"\n";
            break;
        }
        erros=8;
        if(is>0)
        {
            shot[0]=0;
            bytes=0;
            if(FD_ISSET(_c.socket(), &rd))
            {

                bytes = _c.receive(shot,sizeof(shot)-1);
                if(bytes==0)
                {
                    cout << "\nSOCKS IS CLOSED\n";
                    broken=true;
                    break;
                }
                if(bytes>0)
                {
                    enabled=true;
                    shot[bytes]=0;

                    if(shot[0]=='\r' && shot[1]=='\n')
                        ::strcpy(eol,"\r\n");
                    else if(shot[0]=='\r')
                        ::strcpy(eol,"\r");
                    else if(shot[0]=='\n')
                        ::strcpy(eol,"\n");
                    else
                        eol[0]=0;

                    if(!strncmp(shot,"exit",4))
                    {
                        _t.write("exit\r\n",6);
                        broken=true;
                        break;
                    }

                    //cout << "\nSHELL <---" << shot << "<--- TCP\n";
                    if(bytes==1)
                    {
                        switch (shot[0])
                        {
                            case 4:
                                _t.write("exit\r\n",6);
                                broken=true;
                                break;
                            case 3:
                                _t.write("exit\r\n",6);
                                 broken=true;
                                break;
                            default:
                                _t.write(shot,bytes);
                                break;
                        }
                    }
                    else
                        _t.write(shot,bytes);
                }
            }
            if(FD_ISSET(_t.master(), &rd))
            {
                bytes = _t.read(shot,sizeof(shot)-1);
                if(bytes==0)
                {
                    cout << THIS << "\nSHELL IS CLOSED\n";
                    broken=true;
                    break;
                }
                if(bytes>0 && enabled)
                {
                    shot[bytes]=0;
                    broken=_proc_ttyout(shot, bytes, pp, eol);
                }
            }
            if(broken)
                break;
            FD_CLR(_c.socket(), &rd);
            FD_CLR(_t.master(), &rd);
        }
    }
    usleep(10000);
}

bool clicontext::_proc_ttyout(char* shot, int bytes, const std::string& prompt, const char* eolc)
{
    bool        promptsend=false;
    char        eol[3] = {0};

    if(strstr(shot,"\r\n"))
        ::strcpy(eol,"\r\n");
    else if(strchr(shot,'\r'))
        ::strcpy(eol,"\r");
    else if(strchr(shot,'\n'))
        ::strcpy(eol,"\n");

    if(*eol==0)
    {
        bytes = _c.sendall(shot,bytes);
        if(bytes!=0)
        {
            cout << THIS << " SHELL IS CLOSED\n";
            return true;
        }
        return false;
    }

    const char* isprompt = ::strstr(shot, prompt.c_str());
    std::string pcrlf = prompt; pcrlf+=eol;

    char* noprompt = str_trimo_cc(shot, pcrlf.c_str());
    if(*eolc && !strncmp(noprompt,eolc, ::strlen(eolc)))
    {
        ::strcpy(noprompt,noprompt+strlen(eolc));
    }
    bytes = _c.sendall(noprompt,::strlen(noprompt));
    if(isprompt)
    {
    //    _c.sendall(prompt.c_str(),prompt.length());
    }
    if(bytes!=0)
    {
        cout <<  THIS << " SHELL IS CLOSED\n";
        return true;
    }
    return false;
}


int clicontext::forkproc()
{
    _pid = _t.opendev();
    if(_pid>0)
    {
        std::cout << THIS << " forking shell {\n";
        return 0;
    }
    if(_pid==0)
    {
        std::cout << getppid() << "->" << getppid() << " we are in new shell process\n";
    }
    return -1;
}


