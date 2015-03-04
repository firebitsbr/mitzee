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
#include "argspars.h"
#include "main.h"


int client();
int server();


void control_c(int k)
{
    __appinstance.kill(0,0);
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
int client_ftransf(const char* param, const char* f1, const char* f2, char gp);


typedef enum _oohcmds
{
    ooh_esc = 27,
    ooh_til = 96,
    ooh_q = 17,
    ooh_c = 3,
    ooh_d = 4,
    ooh_m = 13,
    ooh_j = 10,
    ooh_x = 24,
    ooh_z = 26,
    ooh_o = 15,
    ooh_h = 8,
    ooh_p = 16,
    ooh_ent = 13,
} oohcmds;


void _help()
{
    std::cout << "oohshell -s*:PORT <-d>        : start server on all interfaces on port, -d run as daemon \n";
    std::cout << "oohshell -sX.Y.Z.K:PORT <-d>  : start server on X.Y.Z.K interface and port, -d run as daemon \n";
    std::cout << "oohshell -cX.Y.Z.K:PORT       : connects shell client to IP:PORT \n";
    std::cout << "oohshell -gX.Y.Z.K:PORT:fname-remote fname-local    : transfer file \n";
    std::cout << "oohshell -pX.Y.Z.K:PORT:fname-local fname-remote     : transfer file \n";
    std::cout << "oohshell stop                 : stops the server instance \n";
}


int main(int n, char* v[])
{
    if(n==1)
    {
        _help();
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
            return __appinstance.kill(v[0], "s");
        }

        if(v[a][0]=='-' && v[a][1]=='c' && ::strlen(v[a])>3)
        {
            params=v[a];
            addon[0]='c';
            cc=true;
        }
        else if(v[a][0]=='-' &&  v[a][1]=='s' && ::strlen(v[a])>3)
        {
            addon[0]='s';
            params=v[a];
        }
        else if(v[a][0]=='-' &&  v[a][1]=='g' && ::strlen(v[a])>3)
        {
            addon[0]='g';
            params=v[a];
        }
        else if(v[a][0]=='-' &&  v[a][1]=='p' && ::strlen(v[a])>3)
        {
            addon[0]='p';
            params=v[a];
        }
    }
    int err =0;
    if(!params.empty() && params.find(":")>0)
    {
        __appinstance.instance(n, v, addon, cc==false, d);
        if(addon[0]=='c')
            err = client(params.c_str());
        else if(addon[0]=='s')
            err = server(params.c_str());
        else if((addon[0]=='p' ||addon[0]=='g') && n==4 )
            err = client_ftransf(params.c_str(), v[2], v[3], addon[0]);
    }
    if(err)
    {
        _help();
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
    bool            online=false;

    ::strcpy(loco,param+2);
    if(*loco==':' || !strchr(loco,':'))return 1;
    const char* piface = ::strtok(loco,":");
    if(0==piface)return 1;
    int port = ::atoi(::strtok(0,":"));
    int         ndfs;
    fd_set      rd;
    timeval     tv = {0,16384};
    char        buff[512];

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

    int  code = c.try_connect(piface,port);
    if(code > 0)
    {
        sleep(1);
        c.sendall("\r",1);
    }

    while(__appinstance.alive())
    {
        FD_ZERO(&rd);
        FD_SET(c.socket(), &rd);
        FD_SET(cinfd, &rd);
        if(c.socket()>0)
            ndfs = c.socket()+1;
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

                if(c.isopen() && code>0)
                {
                    int by = c.sendall(buff,bytes);
                    if(0!=by)
                    {
                        c.destroy();
                        std::cout << "\noohshell$ : ";
                    }
                }
                else
                {
                    int maxcod = min(8,bytes);
                    uint64_t kcode = 0;
                    for(int i=0; i<maxcod; i++)
                    {
                        kcode |= (buff[i] << (i*8)); //grab f# keys as well
                    }
                    // printf("%d\n",kcode);
                    switch(kcode)
                    {
                    case ooh_esc:
                        std::cout << "disconnected\r\n";
                        std::cout << "\r\noohshell$ : ";
                        break;
                    case ooh_p:
                        break;
                    case ooh_q:
                        std::cout << "bye\r\n";
                        __appinstance.kill(0,"c");
                        break;
                    case ooh_o:
                        //std::cout << "connecting to:" << piface << ":" << port;
                        code = c.try_connect(piface,port);
                        if(code==0)
                        {
                            c.destroy();
                        }
                        else if(code>0)
                        {
                            sleep(1);
                            c.sendall("\r",1);
                            break;
                        }
                        std::cout << "\r\noohshell$ : ";
                        break;
                    case ooh_h:
                        std::cout << "esc print status\r\n";
                        std::cout << "^q quit\r\n";
                        std::cout << "^o connect\r\n";
                        std::cout << "^h help\r\n";
                        std::cout << "\r\noohshell$ : ";
                    default:
                        std::cout << "\r\noohshell$ : ";
                        break;
                    }
                    fflush(stdout);
                }
                FD_CLR(cinfd, &rd);
            }

            if(c.isopen() && FD_ISSET(c.socket(), &rd))
            {
                if(code==0)
                {
                    FD_CLR(c.socket(), &rd);
                    c.destroy();
                    std::cout << "\r\noohshell$ :";
                    fflush(stdout);
                    continue;
                }
                else if(code==-1)
                {
                    FD_CLR(c.socket(), &rd);
                    code=c.socket();
                    sleep(1);
                    c.sendall("\r",1);
                    continue; //connected
                }
                int bytes = c.receive(buff,512);
                if(bytes==0)
                {
                    c.destroy();
                    std::cout << "\r\noohshell$ :";
                    fflush(stdout);
                    code=0 ;
                }
                if(bytes>0)
                {

                    buff[bytes]=0;
                    if(buff[0]=='\n')
                        printf("%s","\r");
                    printf("%s",buff);
                    fflush(stdout);
                }
                FD_CLR(c.socket(), &rd);
            }
        }
        ::usleep(0xFFF);
    }
    c.destroy();
    ::tcsetattr(0, TCSANOW, &ttyold);
    __appinstance.kill();
    return 0;
}


int server(const char* param)
{
    std::cout << "server\r\n";
    int             k = 30;
    tcp_srv_sock    s, sftp;
    const char*     iface = 0;
    char            loco[128];

    ::strcpy(loco,param+2);
    if(*loco==':' || !strchr(loco,':'))return 1;
    const char* piface = ::strtok(loco,":");
    if(0==piface)return 1;

    if(piface[0] && piface[0]!='*')
        iface=piface;
    int port = ::atoi(::strtok(0,":"));

    while (k-->0 && __appinstance.alive())
    {
        if(s.create(port, SO_REUSEADDR, iface)<=0 ||
           sftp.create((port+1), SO_REUSEADDR, iface)<=0)
        {
            sftp.destroy();
            s.destroy();
            std::cout <<"listen create socket: " <<strerror(errno) <<" wait a bit or restart later \n";
            port++;
            sleep(3);
            continue;
        }
        else
            break;
    }

    std::cout << "server listen on:" << port << " and on port "<< (port+1) <<"\n";
    ::fcntl(s.socket(), F_SETFD, FD_CLOEXEC);
    ::fcntl(sftp.socket(), F_SETFD, FD_CLOEXEC);

    s.set_blocking(0);
    sftp.set_blocking(0);

    s.listen(32);
    sftp.listen(32);

    cout << "listen ON:\n" << port << " and " << (port+1) <<"\n";
    fd_set  rd;
    int     ndfs;
    timeval tv = {0,16384};
    while(__appinstance.alive())
    {
        FD_ZERO(&rd);
        FD_SET(s.socket(), &rd);
        FD_SET(sftp.socket(), &rd);
        ndfs = max(s.socket(), sftp.socket())+1;

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
            if(FD_ISSET(sftp.socket(), &rd))
            {
                FD_CLR(sftp.socket(), &rd);
                tcp_cli_sock c;
                if(sftp.accept(c)>0)
                {
                    c.set_blocking(0);
                    std::cout << ".... accept ftp conection \n";
                    int sfd = c.detach();

                    ftcontext *pt = new ftcontext(sfd);
                    pt->start_thread();

                }
                else
                {
                    std::cout <<"accept error. close client might be a hanging socket \n";
                }
            }
        }
        usleep(0xFFF);
    }

    sftp.destroy();
    s.destroy();
    std::cout <<"MAIN(" << getpid() << ")  EXITS\r\n";
    return 0;
}



int client_ftransf(const char* param, const char* f1, const char* f2, char fp)
{
    tcp_cli_sock    c;
    char            loco[128];

    ::strcpy(loco,param+2);
    std::cout << "ft client\r\n";
    if(*loco==':' || !strchr(loco,':'))return 1;
    const char* piface = ::strtok(loco,":");
    if(0==piface)return 1;
    int port = ::atoi(::strtok(0,":"))+1;

    int  code = c.try_connect(piface,port);
    if(code == 0)
    {
        std::cout << "cannot conect to: " << piface << ":" <<port<<" \r\m";
        return 0;
    }
    ftcontext   ft(c.detach());
    return ft.thread_main(fp, f1, f2);
}
