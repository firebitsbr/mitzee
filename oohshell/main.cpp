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
#include "argspars.h"
#include "main.h"


int client();
int server();


void control_c(int k)
{
    _I.kill();
}

pid_t    _PID = 0;

processses _PS;
procinst _I( SIGABRT|SIGTERM|SIGKILL|SIGTRAP,  SIGPIPE, control_c);


/**
    openprompt -c127.0.0.1:4444
    openprompt -s*:4444 <-d> or  oohshell -s *:4444  or oohshell -s
    openprompt -s192.168.1.100:4444
    openprompt -c192.168.1.100:4444
    openprompt -stop
*/

int server(const char* param);
int client(const char* param);

void kill_child(int)
{
    pid_t pid = 0;
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
	{
        usleep(0xFFFF);
	}
	return;
}


int main(int n, char* v[])
{
    if(n==1)
    {
        std::cout << v[0] << " " << "-sIFACE|*:PORT, -cIP:PORT  <-d>\n";
        return 1;
    }
    bool d=false;
    bool cc=false;
    std::string params;

    if(0==_PID)
    {
        std::cout << "PID = " << _PID << "\n";
        _PID=getpid();
    }

    signal(SIGALRM,(void (*)(int))kill_child);


    for(int a=1;a<n;a++)
    {
        if(!::strcmp(v[a],"-d"))
        {
            d=true;
            continue;
        }

        if(!::strcmp(v[a],"stop"))
        {
            return _I.kill();
        }

        if(v[a][1]=='c' && ::strlen(v[a])>3)
        {
            params=v[a];
            cc=true;
        }
        else if(v[a][1]=='s' && ::strlen(v[a])>3)
        {
            params=v[a];
        }
    }
    if(!params.empty() && params.find(":")>0)
    {
        _I.instance( n, v, cc==false, d);
        if(cc)
            return client(params.c_str());
        return server(params.c_str());
    }
    std::cout << v[0] << " " << "-sIFACE|*:PORT, -cIP:PORT  <-d>\n";
    return 0;
}


inline int kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}


int client(const char* param)
{
    cout << "not tested , use telnet instead...\n";

    tcp_cli_sock    c;
    char            loco[128];

    ::strcpy(loco,param+1);
    const char* piface = ::strtok(loco,":");
    int port = ::atoi(::strtok(0,":"));

    if(c.try_connect(piface,port)>=0)
    {
        int     ndfs;
        fd_set  rd;
        timeval tv = {0,16384};
		while(_I.alive())
		{
            FD_ZERO(&rd);
            FD_SET(c.socket(), &rd);
            ndfs =c.socket()+1;
            tv.tv_sec = 0;
            tv.tv_usec = 0xFFF;
            int is = ::select(ndfs, &rd, 0, 0, &tv);
            if(is==-1)
                break;
            if(is>0 && FD_ISSET(c.socket(), &rd))
            {
                char buff[512];
                int bytes = c.receive(buff,512);
                if(bytes==0)
                    break;
                if(bytes>0)
                {
                    buff[bytes]=0;
                    std::cout << buff;
                }
                FD_CLR(c.socket(), &rd);
            }
            if(kbhit())
            {
                char chi[2] = {0};
                chi[0] = (char)getchar();
                int by = c.sendall(chi,1);
                if(0!=by)
                    break;
            }
        }
        c.destroy();
    }
    _I.kill();
    sleep(3);
    return 0;
}


int server(const char* param)
{
    int             k = 30;
    tcp_srv_sock    s;
    const char*     iface = 0;
    char            loco[128];

    ::strcpy(loco,param+1);
    const char* piface = ::strtok(loco,":");
    if(piface[0]!='*')iface=piface;
    int port = ::atoi(::strtok(0,":"));

    while (k-->0 && _I.alive())
    {
        if(-1==s.create(port, SO_REUSEADDR, iface))
        {
            std::cout <<"listen create socket: " <<strerror(errno) <<" wait a bit or restart later \n";
            sleep(3);
            continue;
        }else
            break;
	}
	std::cout << "OK, listerning now\n";
	::fcntl(s.socket(), F_SETFD, FD_CLOEXEC);
	s.set_blocking(0);
    if(s.listen(32)==0)
	{
        cout << "listen ON:\n" << port << "\n";
        fd_set  rd;
        int     ndfs;
        timeval tv = {0,16384};
		while(_I.alive())
		{
            FD_ZERO(&rd);
            FD_SET(s.socket(), &rd);
            ndfs =s.socket()+1;
            tv.tv_sec = 0;
            tv.tv_usec = 0xFFF;
            int is = ::select(ndfs, &rd, 0, 0, &tv);
            if(is==-1)
                break;
            if(is>0)
            {
                if(FD_ISSET(s.socket(), &rd))
                {
                    FD_CLR(s.socket(), &rd);
                    tcp_cli_sock c;
                    if(s.accept(c)>0)
                    {
                        c.set_blocking(0);
                        std::cout << ".... accept conection \n";
                        int sfd = c.detach();
    #ifndef USE_THREAD
                        pid_t pc = _PS.forkit();//_PIDS

                        if(pc==0)
                        {
                            s.destroy();
                            std::cout << getppid() << "->" << getppid() << " we are in new process\n";
    #else
                            std::cout << " start thread \n";
    #endif

                            clicontext *pt = new clicontext(sfd);
                            if(pt )
                            {
                                if(pt->forkproc()==0)
                                {
                                    pt->start_thread();
                                }
    #ifndef USE_THREAD
                                pt->_post_thread_foo();
                            }
                            _PS.exitproc(0, "clientcontext");
    #endif
                        }
                    }
                    else
                    {
                        std::cout <<"accept error. close the telnet terminal. there might be a hanging socket \n";
                    }
                }
            }
            usleep(0xFF);
        }
	}
	s.destroy();
	std::cout <<"MAIN(" << getpid() << ")  EXITS\r\n";
    return 0;
}
