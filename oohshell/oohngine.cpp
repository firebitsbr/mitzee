

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

/**
    std::cout << "oohshell -s<d> *:PORT          : start server on all interfaces on port, -d run as daemon \n";
    std::cout << "oohshell -s<d> X.Y.Z.K:PORT <-d>  : start server on X.Y.Z.K interface and port, -d run as daemon \n";
    std::cout << "oohshell -c X.Y.Z.K:PORT       : connects shell client to IP:PORT \n";
    std::cout << "oohshell -cp fname X.Y.Z.K:PORT:fname    : transfer file download \n";
    std::cout << "oohshell -cp X.Y.Z.K:PORT:fname fname    : transfer file download \n";
    std::cout << "oohshell stop                 : stops the server instance \n";
*/

oohngine::oohngine(int nargs, char* vargs[]):_cmd_mode(false),_ip("*"),_port(0),_pg(0),_prun(0)
{
    char            loco[256];
    char            tmp[256];

    if(!strcmp(vargs[1],"-s") || !strcmp(vargs[1],"-c") ||  !strcmp(vargs[1],"-sd"))
    {
        ::strcpy(loco,vargs[2]);
        if(loco[0]!=':' && strchr(loco,':'))
        {
            const char* piface = ::strtok(loco,":");
            if(piface[0] && piface[0]!='*')
            {
                _ip=piface;
            }
            _port = ::atoi(::strtok(0,":"));

            if(strncmp(vargs[1],"-c",2))
            {
                _prun=&oohngine::_server;
            }
            else
            {
                if(!_ip.empty())
                    _prun=&oohngine::_client;
                else
                    _port=0; //trigger ok() as to return false
            }
        }
        return;
    }

    // cp fname@X.Y.Z.K:PORT fname
    // cp fname fname@X.Y.Z.K:PORT
    if(!strcmp(vargs[1],"-cp"))
    {
        if(nargs != 4)return;
        const char* pextok1 = strchr(vargs[2],'@');
        const char* pextok2 = strchr(vargs[3],'@');
        if(pextok1==0 && pextok2==0)return;

        if(pextok1)
        {
            ::strcpy(loco,vargs[2]);
            _pg='g';
            _fl=vargs[3];
        }
        else
        {
            ::strcpy(loco,vargs[3]);
            _pg='p';
            _fl=vargs[2];
        }
        _fr=strtok(loco,"@");
        if(_fr.empty())return;
        ::strcpy(tmp,::strtok(0,"@"));
        if(tmp[0]==0)return;
        _ip=::strtok(tmp,":");
        if(_ip.empty())return;
        _port=::atoi(::strtok(0,":"));
        if(_port)
        {
            _prun=&oohngine::_filetp;
        }
    }
}

oohngine::~oohngine()
{
    //dtor
}


int  oohngine::run()
{
    return (this->*_prun)();
}


static const char* prompt(tcp_cli_sock& c)
{
    static const char  off_line[] = "\r\noffline% ";
    static const char  on_line[] = "\r\non-line$ ";
    return c.is_really_connected() ? on_line : off_line;
}


void oohngine::_execute_cmd()
{
    std::istringstream  iss(_command);
    std::string         token;

    if(!getline(iss, token, ' '))
    {
        std::cout << "no such command: " << _command;
        return;
    }

}

/* a'la google function size...*/
int oohngine::_client()
{
    if(_port==0)return -1;

    struct termios  ttyold,ttynew; //supress local echo
    tcp_cli_sock    c;
    int             cinfd=0;
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

    int  code = c.try_connect(_ip.c_str(),_port);
    if(code > 0)
    {
        sleep(1);
        c.sendall("\r",1);
    }

    while(Inst.alive())
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

                int maxcod = min(8,bytes);
                uint64_t kcode = 0;
                for(int i=0; i<maxcod; i++)
                {
                    kcode |= (buff[i] << (i*8)); //grab f# keys as well
                }
                if(kcode == ooh_alt_c)
                {
                    _cmd_mode=!_cmd_mode;
                    _command.clear();
                    std::cout << prompt(c);
                }

                if(c.isopen() && code>0 && _cmd_mode==false)
                {
                    int by = c.sendall(buff,bytes);
                    if(0!=by)
                    {
                        c.destroy();
                        std::cout << prompt(c);
                    }
                }
                else
                {
                    _local_cmds(buff, bytes, kcode,c);
                }
                FD_CLR(cinfd, &rd);
            }

            if(c.isopen() && FD_ISSET(c.socket(), &rd))
            {
                if(code==0)
                {
                    FD_CLR(c.socket(), &rd);
                    c.destroy();
                    std::cout << prompt(c);
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
                    std::cout  << prompt(c);
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
    Inst.kill();
    return 0;
}


int oohngine::_server()
{
    int             k = 30;
    tcp_srv_sock    s, sftp;

    if(_port==0)return -1;

    while (k-->0 && Inst.alive())
    {
        if(s.create(_port, SO_REUSEADDR, _ip=="*" ? 0 : _ip.c_str() )<=0 ||
           sftp.create((_port+1), SO_REUSEADDR, _ip=="*" ? 0 : _ip.c_str())<=0)
        {
            sftp.destroy();
            s.destroy();
            std::cout <<"listen create socket: " <<strerror(errno) <<" wait a bit or restart later \n";

            sleep(3);
            continue;
        }
        else
            break;
    }

    std::cout << "server listen on:" << _port << " and on port "<< (_port+1) <<"\n";
    ::fcntl(s.socket(), F_SETFD, FD_CLOEXEC);
    ::fcntl(sftp.socket(), F_SETFD, FD_CLOEXEC);

    s.set_blocking(0);
    sftp.set_blocking(0);

    s.listen(32);
    sftp.listen(32);

    cout << "listen ON:\n" << _port << " and " << (_port+1) <<"\n";
    fd_set  rd;
    int     ndfs;
    timeval tv = {0,16384};
    while(Inst.alive())
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
                    std::cout << ".remote shell accepted \n";
                    int sfd = c.detach();
#ifndef USE_THREAD
                    pid_t pc = Proco.forkit();//_PIDS

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
                        Proco.exitproc(0, "clientcontext");
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
                    std::cout << ".... accept file transfer conection \n";
                    ftcontext *pt = new ftcontext(c);
                    c.detach();
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



int oohngine::_filetp()
{
    tcp_cli_sock    c;

    int  code = c.try_connect(_ip.c_str(),_port+1);
    if(code == 0)
    {
        std::cout << "cannot conect to: " << _ip << ":" <<_port+1<<"\n";
        return 0;
    }
    ftcontext   ft(c);
    c.detach();
    return ft.thread_main(_pg, _fl, _fr);
}


void oohngine::_local_cmds(char* buff, int bytes, int kcode, tcp_cli_sock&  c)
{
    int code;
    switch(kcode)
    {
    case ooh_esc:
        std::cout << prompt(c);
        break;
    case ooh_p:
        break;
    case ooh_q:
        std::cout << "bye\r\n";
        Inst.kill(0,"c");
        break;
    case ooh_o:
        //std::cout << "connecting to:" << piface << ":" << port;
        code = c.try_connect(_ip.c_str(),_port);
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
        std::cout << prompt(c);
        break;
    case ooh_h:
        std::cout << "esc print status\r\n";
        std::cout << "^q quit\r\n";
        std::cout << "^o connect\r\n";
        std::cout << "^h help\r\n";
        std::cout << prompt(c);
    default:
        if(_cmd_mode)
        {
            for(int ch=0;ch<bytes;++ch)
            {
                if(buff[ch]=='\r' || buff[ch]=='\n')
                {
                    if(!_command.empty())
                    {
                        _execute_cmd();
                        _command.clear();
                        std::cout << prompt(c);
                    }
                    else
                        std::cout << prompt(c);
                }
                else
                {
                    char two[2]={0};
                    two[0]=buff[ch];
                    _command.append(two);
                    std::cout << two;
                }
            }
        }
        else
            std::cout << prompt(c);
        break;
    }
    fflush(stdout);
}
