/*
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - getic.net - N/A
#           FOR HOME USE ONLY. For corporate  please contact me
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


#include <stdlib.h>
#include <errno.h>
#ifdef WINDOWS
#include <widndows.h>
#include <socket.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <resolv.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <linux/if.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <netdb.h>
#endif
#include <string.h>
#include <string>
#include <vector>
#include <set>
#include <iostream>


#define SRV_PORT  19211
#define CLI_PORT  19212
#define PACKET_LEN  32

static char PI_QUERY[32]= {0};

static bool __alive=true;
class Udper
{
    std::string _iface;
    std::string _ip;

public:
    Udper(std::string& iface):_sock(0),_rock(0)
    {
        char loco[128];
        ::strcpy(loco,iface.c_str());
        if(strchr(loco,','))
        {
            _ip=::strtok(loco,",");
            _iface=::strtok(0,",");
        }
    }
    ~Udper()
    {
        _destroy();
    }

    static bool get_local_ifaces(std::set<std::string>& ifaces)
    {
        bool    newb = false;
        char    tmp[128];
        struct  ifaddrs* ifAddrStruct = 0;
        void    *tmpAddrPtr = 0;

        ifaces.clear();
        getifaddrs(&ifAddrStruct);
        while (ifAddrStruct != 0)
        {
            if (ifAddrStruct->ifa_addr->sa_family==AF_INET &&
                    strncmp(ifAddrStruct->ifa_name, "lo", 2)!=0)
            {
                memset(tmp,0,sizeof(tmp));
                tmpAddrPtr = (void*)&((sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
                inet_ntop(AF_INET, tmpAddrPtr, tmp, (sizeof(tmp)-1));
                strcat(tmp,",");
                strcat(tmp,ifAddrStruct->ifa_name);
                ifaces.insert(tmp);
            }
            ifAddrStruct = ifAddrStruct->ifa_next;
        }
        freeifaddrs(ifAddrStruct);
        return true;
    }

    int listen_and_respond()
    {
        if(_sock==0)
        {
            if(_create(_sock))
            {
                _reuse();
                if(!_bind_to_addr(_sock, _srv_addr,INADDR_ANY, SRV_PORT))
                    return -1;
            }
            else
                return -1;
            //std::cout << "watching for net on port:" <<    SRV_PORT << "\n";
        }
        int s = _is_signaled(_sock);
        if(s>0)
        {
            char                buff[128];
            struct sockaddr_in  remote;
            socklen_t           len = sizeof(remote);
            char                hostname[256]= {0};
            char                out[256]= {0};

            gethostname(hostname, 255);

            sprintf(out,"%s: %s", hostname, _ip.c_str());
            int n=::recvfrom(_sock,buff,sizeof(buff)-1,0,(struct sockaddr *)&remote,&len);
            if(n>0)
            {
                buff[n] = 0;
                //std::cout << "received: " << buff << "\n";
                if(!strcmp(buff,PI_QUERY))
                {
                    remote.sin_port = htons(CLI_PORT);
                    n=::sendto(_sock,out,::strlen(out),0,(struct sockaddr *)&remote,len);
                    if(n==(int)_ip.length())
                    {
                        // std::cout<<"ip:" << _ip << " was sent back \n";
                    }
                }
            }
        }
        else if(s<0)
            return -1;
        return 0;
    }

    int discover();


    const std::set<std::string>& rem_ips()const
    {
        return _remips;
    }
    static void re_interface(std::vector<Udper*>& udps, std::set<std::string>&  ifs)
    {
        static struct stat prev={0};
        struct stat cur={0};

        ::stat("/var/log/syslog", &cur);
        if(cur.st_mtime != prev.st_mtime)
        {
            Udper::get_local_ifaces(ifs);
            std::vector<Udper*>::iterator u = udps.begin();
            for(; u!=udps.end(); ++u)
            {
                delete *u;
            }
            udps.clear();
            std::set<std::string>::iterator i = ifs.begin();
            for(; i!=ifs.end(); ++i)
            {
                std::string is = *i;
                //std::cout << "Adding udp:" << is << "\n";
                Udper* pu = new Udper(is);
                udps.push_back(pu);
            }
            prev.st_mtime=cur.st_mtime;
            sleep(1);
        }
    }

private:

    bool _broad()
    {
        int optval=1;
        socklen_t slen = sizeof(optval);
        return -1!=setsockopt(_sock, SOL_SOCKET, SO_BROADCAST, &optval, slen);
    }

    void _reuse()
    {
        int optval=1;
        socklen_t slen = sizeof(optval);
        setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , slen);
    }
    void _bind_to_iface()
    {
        struct ifreq interface;
        memset(&interface, 0, sizeof(interface));
        ::strncpy(interface.ifr_ifrn.ifrn_name, _iface.c_str(), IFNAMSIZ);
        if (-1 == setsockopt(_sock,
                             SOL_SOCKET,
                             SO_BINDTODEVICE,
                             &interface,
                             sizeof(interface)))

        {
            perror("bind 2 dev");
        }
    }

    bool _bind_to_addr(int s, struct sockaddr_in& skaddr, int addr = INADDR_ANY, int port=0)
    {
        skaddr.sin_family = AF_INET;
        skaddr.sin_addr.s_addr = htonl(addr);
        skaddr.sin_port = htons(port);
        if (bind(s, (struct sockaddr *) &skaddr, sizeof(skaddr))<0)
        {
            perror("bind");
            return false;
        }
        return true;
    }

    bool _create(int& s)
    {
        s = socket(AF_INET, SOCK_DGRAM, 0);
        return  s>0;
    }


    void _destroy()
    {
        if(_sock>0)
        {
            ::shutdown(_sock, 0x3);
            ::close(_sock);
        }
        _sock=0;
        if(_rock>0)
        {
            ::shutdown(_rock, 0x3);
            ::close(_rock);
        }
        _rock=0;
    }

    int _is_signaled(int s)
    {
        timeval  tv   = {0, 1000};
        fd_set   rd_set;

        FD_ZERO(&rd_set);
        FD_SET(s, &rd_set);
        int nfds = (int)s+1;
        int sel = ::select(nfds, &rd_set, 0, 0, &tv);
        if(sel<0)
        {
            perror ("select");
            return -1;
        }
        return sel > 0 && FD_ISSET(s, &rd_set);

    }


private:
    int          _sock;
    int          _rock;

    std::set<std::string>   _remips;
    sockaddr_in             _srv_addr;
};


int main(int argc, char *argv[])
{
    std::vector<Udper*>  udps;
    std::set<std::string>  ifs;

    if(argc <3)
    {
        std::cout << "Usage:    'pidisc d signaure' to discover\n          'pidisc l signature' to run on RPI\n";
        return -1;
    }
    ::strcpy(PI_QUERY,argv[2]);


    if(*argv[1]=='l')   //listen
    {
        int hmin=3000;
        while(__alive)
        {
            if(hmin++ % 3000==0)
            {
                Udper::re_interface(udps,ifs);
            }
            std::vector<Udper*>::iterator u = udps.begin();
            for(; u!=udps.end(); ++u)
            {
                Udper* pu = (*u);

                pu->listen_and_respond();
            }
            usleep(10000);
        }

        std::vector<Udper*>::iterator u = udps.begin();
        for(; u!=udps.end(); ++u)
        {
            delete *u;
        }
    }
    else if(*argv[1]=='d')     //discover
    {
        Udper::get_local_ifaces(ifs);

        std::set<std::string>::iterator i = ifs.begin();
        for(; i!=ifs.end(); ++i)
        {
            std::string is = *i;
            Udper* pu = new Udper(is);
            udps.push_back(pu);
        }

        std::set<Udper*>    done;
        std::vector<Udper*>::iterator u ;
        for(int k=0; k<4; ++k)
        {
            u = udps.begin();
            for(; u!=udps.end(); ++u)
            {
                Udper* pu = (*u);
                int rk = pu->discover();
                if(rk>0 && done.find(pu) == done.end())
                {
                    done.insert(pu);
                }
            }
        }

        std::cout << "\n";
        int k=1;
        std::set<Udper*>::iterator si = done.begin();
        for(; si!=done.end(); si++)
        {
            const std::set<std::string>& infos = (*si)->rem_ips();
            std::set<std::string>::const_iterator info=infos.begin();
            for(; info!=infos.end(); info++)
            {
                std::cout <<  k <<": " << *info << "\n";
                ++k;
            }
        }
        std::cout << "\n";
        u = udps.begin();
        for(; u!=udps.end(); ++u)
        {
            delete *u;
        }
    }
    else
    {
        std::cout << "Invalid arguments\n";
        std::cout << "'pidisc d signaure' to discover\b 'pidisc l signature &' to run on RPI";
    }
    //std::cout << "no interfaces\n";
    return 0;
}


int Udper::discover()
{
    if(_sock==0)
    {
        if(_create(_sock))
        {
            if(!_broad()){
                std::cout  << "cannot create broad:" << errno << "\n";
                return -1;
            }
        }
        else{
            std::cout << "cannot create l socket:" << errno << "\n";
            return -1;
        }
        std::cout << "try port:" <<    SRV_PORT <<" on "<< _iface <<"\n";
    }
    if(_rock==0)
    {
        if(_create(_rock))
        {
            if(!_bind_to_addr(_rock, _srv_addr,INADDR_ANY, CLI_PORT))
            {
                std::cout << "cannot bind:" << errno << "\n";
                return -1;
            }
        }
        else{
            std::cout << "cannot create r socket:" << errno << "\n";
            return -1;
        }
    }

    // broadcast on _sock
    struct sockaddr_in skaddr;

    skaddr.sin_family = AF_INET;
    skaddr.sin_addr.s_addr = INADDR_BROADCAST;
    skaddr.sin_port = htons(SRV_PORT);


    ::sendto(_sock,PI_QUERY,::strlen(PI_QUERY),0,(struct sockaddr*) &skaddr,sizeof(skaddr));
    for(int k=0; k<512; ++k)
    {
        if(k%8==0)
            ::sendto(_sock,PI_QUERY,::strlen(PI_QUERY),0,(struct sockaddr*) &skaddr,sizeof(skaddr));

        //_wait on _rock
        int n = _is_signaled(_rock);
        if(n==-1)
        {
            break;
        }
        if(n>0)
        {
            char                buff[128];
            struct sockaddr_in  remote;
            socklen_t           len = sizeof(remote);
            n=::recvfrom(_rock,buff,sizeof(buff)-1,0,(struct sockaddr *)&remote,&len);
            if(n>0)
            {
                buff[n] = 0;
                if(_remips.find(buff)==_remips.end())
                {
                    _remips.insert(buff);
                }
            }
        }
        usleep(1024);
        //++progress;
        //pd->ui->progressBar->setValue(progress);
    }

    return _remips.size();
}

