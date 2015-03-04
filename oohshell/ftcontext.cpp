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
#include "ftcontext.h"
#include "main.h"


ftcontext::ftcontext(int  sfd):_pg(0),_err(0),_pf(0),_len(0),_remain(-1),_transf(0),_isclient(false)
{
    _c.attach(sfd);
    _c.set_blocking(0);
    std::cout << "ftcontext { \n";
}

ftcontext::~ftcontext()
{
    if(_pf) ::fclose(_pf);
    _c.destroy();
    std::cout << "ftcontext } \n";
}

int ftcontext::thread_main(char pg, const string& f1,  const string& f2)
{
    _f1 = f1;
    _f2 = f2;
    _pg = pg;
    _isclient=true;

    try{
        _thread_main();
    }catch(int& err)
    {
        std::cout << "error while tranferring \n";
        return 1;
    }
    return 0;
}


void ftcontext::_send_header(char over)
{
    char        loco[512];
    std::string putget;

    if(over==0)
        over=_pg;

    if(over=='p')//local remote  p:abc.txt:4444*
    {
        if(_pf==0)
        {
            _pf=fopen(_f1.c_str(),"rb");
            if(!_pf)
            {
                throw(errno);
            }
            ::fseek (_pf, 0, SEEK_END);   // non-portable
            _len=ftell (_pf);
            ::fseek (_pf, 0, SEEK_SET);   // non-portable
            _remain=_len;
        }

        ::sprintf(loco,"%ld", _len);
        putget="p:";
        putget+=_f2;
        putget+=":";
        putget+=loco;
        putget+="*";
    }
    else //remote local  g:abc.txt:0**
    {
        if(_pf==0)
        {
            _pf=fopen(_f2.c_str(),"wb");
            if(!_pf)
            {
                throw(errno);
            }
        }
        putget="g:";
        putget+=_f1;
        putget+=":0*";
    }
    _c.sendall(putget.c_str(), putget.length());

}

int ftcontext::_read_header(char* buff, int bytes)
{
    char    loco[255];
    int     hdrlen = 0;
    char*   eohdr = str_up2chr(buff, '*', hdrlen);

//    std::cout << "received buffer[" << buff << "]\n";
//    std::cout << "received header[" << eohdr << "]\n";

    ::strncpy(loco, eohdr, sizeof(loco)-1);
    const char* mustp = ::strtok(loco,":");
    _pg      = mustp[0];
    char* rf = ::strtok(0,":");
    _f2      = rf;
    _len     = ::atol(::strtok(0,":"));
    return hdrlen;
}

void ftcontext::thread_main()
{
    try{
        _thread_main();
    }catch(int& err)
    {
        std::cout << "error while tranferring \n";
    }

}

void ftcontext::_thread_main()
{

    int         code;
    char        loco[4096];
    int         ndfs;
    timeval     tv = {0,16384};

    if(_isclient && _c.is_really_connected() > 0)
    {
        _send_header();
    }

    while(__appinstance.alive() && _remain>0)
    {
        FD_ZERO(&_rd);
        FD_ZERO(&_wr);
        FD_SET(_c.socket(), &_rd);
        FD_SET(_c.socket(), &_wr);

        ndfs = _c.socket()+1;
        tv.tv_sec = 0;
        tv.tv_usec = 0x1FFFF;
        int is = ::select(ndfs, &_rd, &_wr, 0, &tv);
        if(is==-1)  break;
        if(is==0)
        {
            ::usleep(1000);
            continue;
        }
        if(FD_ISSET(_c.socket(), &_rd) &&
            _c.isconnecting() &&
            _isclient)
        {
            _c.setconnected();
            _send_header();
            FD_CLR(_c.socket(), &_rd);
            FD_CLR(_c.socket(), &_wr);
            continue;
        }
        if(_isclient)//client
        {
            _client_pg(loco, sizeof(loco)-1);
        }
        else //server
        {
            _server_pg(loco, sizeof(loco)-1);
        }
        FD_CLR(_c.socket(), &_rd);
        FD_CLR(_c.socket(), &_wr);
        usleep(0xFF);
    }
}

void  ftcontext::_client_pg(char* loco, int cap)
{
    if(_pg=='p' && FD_ISSET(_c.socket(), &_wr))
    {
        int bytes = ::fread(loco,1,cap,_pf);
        if(bytes>0)
        {
            int bytesw = _c.sendall(loco,bytes);
            if(bytesw!=0)
            {
                throw(_c.error());
            }
            _transf+=bytes;
            _remain-=bytes;
        }
        if(bytes != sizeof(loco)-1 && feof(_pf))
        {
            if(bytes==0)
                throw(-1);

        }
        std:: cout << _f1 << "->" << _transf << " / " << _len  << " -> "<< _c.getsocketaddr_str(_temp) <<":"<<_f2 <<"\r";
         fflush(stdout);
        return;
    }

    if(FD_ISSET(_c.socket(), &_rd))
    {
        //client get
        const char* pbuff=loco;
        int         bytes = _c.receive(loco,cap);

        if(0==bytes){
            throw(-2);
        }
        loco[bytes]=0;
        if(_len==0)
        {
            int  hdrlen=_read_header(loco,bytes);
            if(0==hdrlen)
            {
                throw(3);
            }
            bytes-=hdrlen;
            pbuff+=hdrlen;
        }
        int bytesw = ::fwrite(pbuff,1,bytes,_pf);
        if(bytesw != bytes)
        {
            throw(-4);
        }
        _remain -= bytesw;
        _transf+=bytesw;
        std:: cout << _f1 << "<-" << _transf << " / " << _len  << " -> "<< _c.getsocketaddr_str(_temp) <<":"<<_f2 <<"\r";
         fflush(stdout);
    }

}

void ftcontext::_server_pg(char* loco, int cap)
{
    const char* ploco=loco;
    int bytes = 0;

    if(FD_ISSET(_c.socket(), &_rd))
    {
        if(_pg==0)
        {
            bytes = _c.receive(loco,cap);
            if(0==bytes){
                throw(-5);
            }
            if(_len==0 && bytes>0)
            {
                int hdrlen = _read_header(loco, bytes);
                if(_pg=='p')
                {
                    _pf=::fopen(_f2.c_str(),"wb");
                    if(_pf==0)
                    {
                         throw(errno);
                    }
                }
                else
                {
                     assert(_pf==0);
                    _pf=::fopen(_f2.c_str(),"rb");
                    if(_pf==0)
                    {
                         throw(errno);
                    }
                    ::fseek (_pf, 0, SEEK_END);   // non-portable
                    _len=ftell (_pf);
                    ::fseek (_pf, 0, SEEK_SET);   // non-portable
                    _remain=_len;
                    _send_header('p');
                }
                bytes-=hdrlen;
                ploco+=hdrlen;
            }
        }
        else
        {
            if(_pg=='p')
            {
                bytes = _c.receive(loco,cap);
                if(0==bytes){
                    throw(_c.error());
                }
            }
        }
        if(0==_pf)
            return;
        if(_pg=='p' && bytes)
        {
            int written = ::fwrite(ploco,1,bytes,_pf);
            if(written != bytes)
            {
                throw(errno);
            }
            _transf+=written;
            _remain-=written;
        }
    }
    else if(FD_ISSET(_c.socket(), &_wr))
    {
        if(0==_pf)
            return;
        int bytes = ::fread(loco,1, cap, _pf);
        if(bytes>0)
        {
            int all = _c.sendall(loco,bytes);
            if(all!=0)
            {
                throw(_c.error());
            }
            else
            {
                _transf+=bytes;
                _remain-=bytes;
            }
        }
    }

}








