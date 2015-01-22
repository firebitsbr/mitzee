/**
# Copyright (C) 2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - coinscode.com - N/A
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
#include <unistd.h>
#include "strutils.h"
#include "config.h"
#include "watcher.h"
#include "theapp.h"

extern bool __alive;

Watcher::Watcher():_u_dnsidx(0),_errors(0),_seq(0)
{
}

Watcher::~Watcher()
{
    signal_to_stop();
    stop_thread();
}

void Watcher::thread_main()
{
    time_t now = 0;
    _reconnow = false;

    while(!is_stopped() && __alive)
    {
        if(time(0) - now > 15 || _reconnow) //every 15 seconds
        {
            if(!PCFG->_srv._prx_addr.empty())
                _keep_alive();
            PCFG->check_log_size();
            now = time(0);
            _reconnow=false;
        }
        if(!_reconnow)
            sleep(1);
    }
}

#if 0
static std::string exec(char* cmd)
{
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    int enough=32;
    while(!feof(pipe) && --enough>0)
    {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}
#endif

bool  Watcher::is_cached(const Message& m)
{
    return false;
}

bool Watcher::cache( Message& m)
{
    return false;
}


bool Watcher::is_dn_for_prx(const char* dname, DnsRecord* rec)
{
    if(dname==0 || *dname==0) return 0;

    std::map<std::string,SADDR_46>::const_iterator rule = PCFG->rules.find(dname);
    if(rule != PCFG->rules.end())
    {
        //    GLOGD("domain: " << dname << " replaced with: " << rule->second.c_str()  << "/" << IP2STR(rule->second));
        rec->ip = htonl(rule->second.ip4());
//        std::cout << "MASQ: " << dname << " -> " << IP2STR(htonl(rec->ip)) << "\n";
        return true;
    }
    return false;

}


bool Watcher::https_notify_proxy( Message& m, int c, const SADDR_46& uip)
{
    if(PCFG->_srv._prx_addr.empty())
    {
        GLOGE("???? cannot notify proxy. address not configured");
        return false; //no notifications
    }
    if(_try_connect(false)==false)
    {
        GLOGE("Connection to proxy is not openned yet");
        return false;
    }
    bool            b = false;
    if(c == 0) //client message
    {
        for(auto & it : m._responses)
        {
            const Message::Answer& a  = it;
            auto dn = PCFG->rules.find(a.name);

            if(dn != PCFG->rules.end())
            {
                DnsCommon   dns;

                dns.header   = 'D';
                dns.sequence = ++_seq;
                dns.now      = time(0);
                dns.client   = uip.sin_addr.s_addr;
                dns.prxip    = PCFG->_srv._prx_addr.ip4();
                dns.domainip = a.origip.s_addr;
                ::strcpy(dns.hostname,a.name);

                GLOGI("[D]->[P]" << "C: " << IP2STR(htonl(dns.client)) << " IP:" << IP2STR(htonl(dns.domainip)) << "->" << IP2STR(dns.prxip));
                if(_prxsock.send((char*)&dns, sizeof(dns))!=sizeof(dns))
                {
                    GLOGE("Error usending to proxy\n");
                }
                b = true;           //one name is OK
                break;
            }
        }
    }
    else
    {
        for(auto & it : m._requests)
        {
            const Message::Question& q  = it;
            auto dn = PCFG->rules.find(q.name);

            if(dn != PCFG->rules.end())
            {
                DnsCommon   dns;

                dns.header   = 'D';
                dns.sequence = ++_seq;
                dns.now      = time(0);
                dns.client   = uip.sin_addr.s_addr;
                dns.prxip    = PCFG->_srv._prx_addr.ip4();
                dns.domainip = 0;
                ::strcpy(dns.hostname, q.name);

                GLOGI("[D]->[P]" << "C: " << IP2STR(htonl(dns.client)) << " IP:" << IP2STR(htonl(dns.domainip)) << "->" << IP2STR(dns.prxip));
                if(_prxsock.send((char*)&dns, sizeof(dns))!=sizeof(dns))
                {
                    GLOGE("Error usending to proxy\n");
                }
                b = true;           //one name is OK
                break;
            }
        }
    }
    return b;
}

bool     Watcher::is_dns_addr(const SA_46& inaddr)
{
    SADDR_46 sadr(inaddr);

    std::string ip = sadr.c_str();
    std::vector<string>::iterator sit = PCFG->_srv.nextdnss.begin();

    for(; sit != PCFG->_srv.nextdnss.end(); ++sit)
    {
        if(*sit == ip)
        {
            return true;
        }
    }
    return false;
}

void   Watcher::_keep_alive()
{
    if(_try_connect(true))
    {
        AutoLock        __a(&_m);
        int bytes = _prxsock.sendall("########",8); // keep connection
        if(bytes!=0)
        {
            _prxsock.destroy();
            GLOGE(": "<< "destroyng connect " << PCFG->_srv._prx_addr.c_str() << ":" <<  PCFG->_srv._prx_addr.port() << "\n");
            _reconnow=true;
        }
    }
    //GLOGI("pinging proxy sock");
}



bool   Watcher::_try_connect(bool doit)
{
    AutoLock        __a(&_m);

    if(!_prxsock.isopen())
    {
        if(doit)
        {
            _prxsock.destroy();
            if(_prxsock.raw_connect(PCFG->_srv._prx_addr, 4)==-1)
            {
                GLOGE(": "<< "cannot connect " << PCFG->_srv._prx_addr.c_str() << ":" <<  PCFG->_srv._prx_addr.port() << "\n");
                return false;
            }
            GLOGI("Connection established to proxy notification port");
            return true;
        }
        return false;
    }
    return true;
}
