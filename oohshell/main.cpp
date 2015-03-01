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
    __appinstance.kill();
}


procwrapper __pwrap;
procinst __appinstance( SIGABRT|SIGTERM|SIGKILL|SIGTRAP,  SIGPIPE, control_c);


/**
    openprompt -c127.0.0.1:4444
    openprompt -s*:4444 <-d> or  oohshell -s *:4444  or oohshell -s
    openprompt -s192.168.1.100:4444
    openprompt -c192.168.1.100:4444
    openprompt -stop
*/

int server(const char* param);
int client(const char* param);


int main(int n, char* v[])
{
    if(n==1)
    {
        std::cout << v[0] << " " << "-sIFACE|*:PORT, -cIP:PORT  <-d>\n";
        return 1;
    }
    bool d=false;
    bool cc=false;
    char  addon[3]= {0};
    std::string params;

    // zombie reaper
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR)
    {
        perror(0);
        exit(1);
    }

    for(int a=1; a<n; a++)
    {
        if(!::strcmp(v[a],"-d"))
        {
            d=true;
            continue;
        }

        if(!::strcmp(v[a],"stop"))
        {
            return __appinstance.kill();
        }

        if(v[a][1]=='c' && ::strlen(v[a])>3)
        {
            params=v[a];
            addon[0]='c'; //add on to the name of the process to avoid  instance  naming clash when server or client
            cc=true;
        }
        else if(v[a][1]=='s' && ::strlen(v[a])>3)
        {
            addon[0]='s'; //add on to the name of the process to avoid  instance  naming clash when server or client
            params=v[a];
        }
    }
    if(!params.empty() && params.find(":")>0)
    {
        __appinstance.instance(n, v, addon, cc==false, d);
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
    std::cout << "client\r\n";
    struct termios  ttyold,ttynew; //supress local echo
    tcp_cli_sock    c;
    char            loco[128];
    int             cinfd=0;

    ::strcpy(loco,param+2);
    const char* piface = ::strtok(loco,":");
    int port = ::atoi(::strtok(0,":"));
    int code = c.try_connect(piface,port);
    if(code!=0)
    {
        int     ndfs;
        fd_set  rd;
        timeval tv = {0,16384};
        char buff[512];

        ::tcgetattr(0, &ttyold);
        ::memcpy(&ttynew,&ttyold,sizeof(ttyold));

        ttynew.c_iflag = 0;
        ttynew.c_oflag = 0;
        // ttynew.c_lflag &= ~ICANON;
        //  ttynew.c_lflag &= ~ECHO;


        ttynew.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                            | INLCR | IGNCR | ICRNL | IXON);
        ttynew.c_oflag &= ~OPOST;
        ttynew.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        ttynew.c_cflag &= ~(CSIZE | PARENB);
        ttynew.c_cflag |= CS8|ECHONL;


        //ttynew.c_lflag |=INLCR;

        ttynew.c_cc[VMIN] = 1;
        ttynew.c_cc[VTIME] = 1;
        ::tcsetattr(0, TCSANOW, &ttynew);

        sleep(1);



        while(__appinstance.alive())
        {
            FD_ZERO(&rd);
            FD_SET(c.socket(), &rd);
            FD_SET(cinfd, &rd);
            ndfs =c.socket()+1;
            tv.tv_sec = 0;
            tv.tv_usec = 0x1FFFF;
            int is = ::select(ndfs, &rd, 0, 0, &tv);
            if(is==-1)
                break;
            if(is>0)
            {

                if(FD_ISSET(cinfd, &rd))
                {
                    int bytes = read(cinfd,buff,sizeof(buff)-1);
                    if(bytes>0)
                        buff[bytes]=0;
                    int by = c.sendall(buff,bytes);
                    if(buff[0]==0x3)
                        break;
                    if(0!=by)
                        break;
                    FD_CLR(cinfd, &rd);
                }

                if(FD_ISSET(c.socket(), &rd))
                {
                    int bytes = c.receive(buff,512);
                    if(bytes==0)
                    {
                        if(code==0)/*from connect, is connected*/
                            break;
                        code=0;
                    }
                    if(bytes>0)
                    {

                        buff[bytes]=0;
                        if(buff[0]=='\n')
                            printf("\r");
                        printf(buff);
                        fflush(stdout);
                    }
                    FD_CLR(c.socket(), &rd);
                }
            }
        }
        c.destroy();
        ::tcsetattr(0, TCSANOW, &ttyold);
    }
    else
        std::cout << "cannot connect:" << errno <<"\n";
    __appinstance.kill();
    return 0;
}


int server(const char* param)
{
    std::cout << "server\r\n";
    int             k = 30;
    tcp_srv_sock    s;
    const char*     iface = 0;
    char            loco[128];

    ::strcpy(loco,param+1);
    const char* piface = ::strtok(loco,":");
    if(piface[0]!='*')iface=piface;
    int port = ::atoi(::strtok(0,":"));

    while (k-->0 && __appinstance.alive())
    {
        if(-1==s.create(port, SO_REUSEADDR, iface))
        {
            std::cout <<"listen create socket: " <<strerror(errno) <<" wait a bit or restart later \n";
            port++;
            sleep(3);
            continue;
        }
        else
            break;
    }
    std::cout << "server listen on:" << port << "\n";
    ::fcntl(s.socket(), F_SETFD, FD_CLOEXEC);
    s.set_blocking(0);
    if(s.listen(32)==0)
    {
        cout << "listen ON:\n" << port << "\n";
        fd_set  rd;
        int     ndfs;
        timeval tv = {0,16384};
        while(__appinstance.alive())
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
                        pid_t pc = __pwrap.forkit();//_PIDS

                        if(pc==0)
                        {
                            s.destroy();
                            std::cout << getppid() << "->" << getppid() << " we are in new process\n";
#else
                        std::cout << "USE THREAD:  start thread \n";
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
                            std::cout << "process:" << getpid() << " exits return 0\n";
                            //return 0;
                            __pwrap.exitproc(0, "<<<<<<<<<<clientcontext");
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
