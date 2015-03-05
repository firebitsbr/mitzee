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


ftcontext::ftcontext(tcp_cli_sock&  sfd):_pg(0),_err(0),_pf(0),_len(0),_remain(-1),_transf(0),_isclient(false)
{
    _c=sfd;
    _c.set_blocking(0);
    std::cout << "FILETP { \n";
}

ftcontext::~ftcontext()
{
    if(_pf) ::fclose(_pf);
    _c.destroy();
    std::cout << "FILETP } \n";
}

int ftcontext::thread_main(char pg, const string& f1,  const string& f2)
{
    _localf = f1;
    _remf = f2;
    _pg = pg;
    _isclient=true;

    try{
        _thread_main();
    }catch(custerr& err)
    {
        std::cout << err.str() <<"\n";
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
            _pf=fopen(_localf.c_str(),"rb");
            if(!_pf)
            {
                THROW("cannot open " << _localf);
            }
            ::fseek (_pf, 0, SEEK_END);   // non-portable
            _len=ftell (_pf);
            ::fseek (_pf, 0, SEEK_SET);   // non-portable
            _remain=_len;

            std::cout << _localf <<" opened for read: " << _len <<" bytes\n";
        }

        ::sprintf(loco,"%ld", _len);
        putget="p:";
        putget+=_remf;
        putget+=":";
        putget+=loco;
        putget+="*";
    }
    else if(over=='g')
    {
        if(_pf==0)
        {
            if(::access(_localf.c_str(),0)==0)
            {
                ::std::string newname=_localf+"~old";
                ::unlink(newname.c_str());
                ::rename(_localf.c_str(), newname.c_str());
                THROW("file: " << _localf << " exist, so it was renamed to " << newname);
                ::unlink(_localf.c_str());
            }
            _pf=fopen(_localf.c_str(),"wb");
            if(!_pf)
            {
                THROW("cannot open wb:" << _remf);
            }
        }
        putget="g:";
        putget+=_remf;
        putget+=":0*";

        std::cout << _localf <<" opened for write: " << _len <<" bytes\n";
    }
    std::cout << "send header: " << putget<< "\n";
    _c.sendall(putget.c_str(), putget.length());

}

int ftcontext::_read_header(char* buff, int bytes)
{
    char    loco[255];
    int     hdrlen = 0;
    char*   eohdr = str_up2chr(buff, '*', hdrlen);

    std::cout << __FUNCTION__<<" received header[" << eohdr << "]\n";

    ::strncpy(loco, eohdr, sizeof(loco)-1);
    const char* mustp = ::strtok(loco,":");
    _pg      = mustp[0];
    char* rf = ::strtok(0,":");
    _remf      = rf;
    _len     = ::atol(::strtok(0,":"));
    _remain = _len;
    if(_pg=='p' && _len==0)
    {
        THROW("invliad file length in header");
    }
    std::cout << __FUNCTION__ <<_pg << _remf <<"/"<< _len << " bytes\n";
    return hdrlen;
}

void ftcontext::thread_main()
{
    try{
        _thread_main();
        std::cout << "transfer finished \r\n";
    }catch(custerr& err)
    {
        std::cout << err.str() <<"\n";
    }
}

const char* ftcontext::_sarrow()
{
    static int lindex=0;
    ::strcpy(_arrow,"       ");
    _arrow[lindex]=_carr;
    lindex++;
    if( lindex>=sizeof(_arrow)-2 )lindex=0;
    return _arrow;
}


void ftcontext::_thread_main()
{

    char        loco[4096];
    int         ndfs;
    timeval     tv = {0,16384};
    bool        hdrsent=false;

    _now = time(0);
    if(_isclient && _c.is_really_connected() > 0)
    {
        hdrsent=true;
        _send_header();
    }

    while(Inst.alive() && _remain>0)
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
            if(time(0)-_now > 16) //16 seconds without progress break
                THROW("time out.");
            ::usleep(1000);
            continue;
        }
        if(FD_ISSET(_c.socket(), &_rd) &&
            _c.isconnecting() &&
            _isclient &&
            !hdrsent)
        {
            _c.setconnected();
            if(hdrsent==false)
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
    if(_pg=='p')
    {
        if(FD_ISSET(_c.socket(), &_wr))
        {
            int bytes = ::fread(loco,1,cap,_pf);
            if(bytes>0)
            {
                int bytesw = _c.sendall(loco,bytes);
                if(bytesw!=0)
                {
                    THROW("socket sendall:" << _c.error());
                }
                _transf+=bytes;
                _remain-=bytes;
                _now = time(0);
            }
            if(bytes != cap || feof(_pf))
            {
                if(bytes==0 && _remain)
                    THROW("failed to read from file");
            }
            _carr='>';
            int perc = (_transf*100/_len);
            std:: cout << _localf << "[" << _len << "]"<<_sarrow()<< _c.getsocketaddr_str(_temp) <<":"<<_remf <<"["<<_transf<<"]....."<<perc<<"%\r";
            fflush(stdout);
        }
    }
    else if(_pg=='g')
    {
        if(FD_ISSET(_c.socket(), &_rd))
        {
            //client get
            const char* pbuff=loco;
            int         bytes = _c.receive(loco,cap);

            if(0==bytes){
                THROW("receive error. connection closed");
            }
            loco[bytes]=0;
            if(_len==0)
            {
                int  hdrlen=_read_header(loco,bytes);
                _pg='g';
                if(0==hdrlen)
                {
                    THROW("cannt read  file transfer header ");
                }
                bytes-=hdrlen;
                pbuff+=hdrlen;
            }
            int bytesw = ::fwrite(pbuff,1,bytes,_pf);
            if(bytesw != bytes)
            {
                THROW("cannt write to file ");
            }
            _remain -= bytesw;
            _transf+=bytesw;
            _now = time(0);
            int perc = (_transf*100/_len);
            _carr='<';
            std:: cout << _localf << "[" << _transf << "]"<<_sarrow()<< _c.getsocketaddr_str(_temp) <<":"<<_remf <<"["<<_len<<"]....."<<perc<<"%\r";
             fflush(stdout);
        }
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
                THROW("cannot reeive. connection closed ");
            }
            if(_len==0 && bytes>0)
            {
                int hdrlen = _read_header(loco, bytes);
                if(_pg=='p')
                {
                    if(::access(_localf.c_str(),0)==0)
                    {
                        ::std::string newname=_localf+"~old";
                        ::unlink(newname.c_str());
                        ::rename(_localf.c_str(), newname.c_str());
                        THROW("file: " << _localf << " exist, so it was renamed to " << newname);
                        ::unlink(_localf.c_str());
                    }
                    std::cout << " opening for write " << _remf <<"\n";
                    _pf=::fopen(_remf.c_str(),"wb");
                    if(_pf==0)
                    {
                         THROW("cannot open file to write into ");
                    }
                }
                else
                {
                     assert(_pf==0);
                    _pf=::fopen(_remf.c_str(),"rb");
                    if(_pf==0)
                    {
                         THROW("cannot open file for read ");
                    }
                    ::fseek (_pf, 0, SEEK_END);
                    _len=ftell (_pf);
                    ::fseek (_pf, 0, SEEK_SET);
                    _remain=_len;
                    std::cout << " opening for read " << _remf <<" of " <<_len<<" bytes\n";
                    if(_len==0)
                    {
                        THROW(_remf << " has 0 bytes. aborting" );
                    }
                    _send_header('p');
                }
                bytes-=hdrlen;
                ploco+=hdrlen;
                _now = time(0);
            }
        }
        else if(_pg=='p')
        {
            bytes = _c.receive(loco,cap);
            if(0==bytes){
                THROW("receive error. connection is closed");
            }
        }
        if(0==_pf)
        {
            THROW("destination file was not opened !!!");
        }
        if(_pg=='p' && bytes)
        {
            int written = ::fwrite(ploco,1,bytes,_pf);
            if(written != bytes)
            {
                THROW("cannot write to file ");
            }
            _transf+=written;
            _remain-=written;
            _now = time(0);
        }
    }
    if(FD_ISSET(_c.socket(), &_wr))
    {
        if(_pg=='g')
        {

            if(0==_pf)
            {
                THROW("source file was not opened !!!");
            }
            int bytes = ::fread(loco,1, cap, _pf);
            if(bytes>0)
            {
                int all = _c.sendall(loco,bytes);
                if(all!=0)
                {
                    THROW("cannot send all bytes to socket ");
                }
                else
                {
                    _transf+=bytes;
                    _remain-=bytes;
                    _now = time(0);
                }
            }
            else if(_remain)
            {
                THROW("cannot read all bytes from file ");
            }
        }
    }

}








