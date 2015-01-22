/**
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
#
*/

#ifndef UDPSOCKS_H
#define UDPSOCKS_H

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

static char PI_QUERY[32]={0};

class Dialog;
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
    static int get_local_ifaces(std::vector<std::string>& ifaces)
    {

        char    tmp[128];
        struct  ifaddrs* ifAddrStruct = 0;
        void    *tmpAddrPtr = 0;

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
                ifaces.push_back(tmp);
                std::cout <<"Local IP:" << tmp << "\n";
            }
            ifAddrStruct = ifAddrStruct->ifa_next;
        }
        freeifaddrs(ifAddrStruct);
        return ifaces.size(); //returns 'ip,interface'
    }


    int discover(int& progress, Dialog* pd);


    const std::set<std::string>& rem_ips()const
    {
        return _remips;
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


#endif // UDPSOCKS_H
