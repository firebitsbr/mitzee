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
#include <sock.h>
#include "strutils.h"
#include "config.h"
#include "watcher.h"
#include "theapp.h"

extern bool __alive;

Watcher::Watcher():_u_dnsidx(0),_errors(0),_seq(0),_lastactivity(0)
{
}

Watcher::~Watcher()
{
    signal_to_stop();
    stop_thread();
}

void Watcher::thread_main()
{
    time_t last = 0;
    _reconnow = false;

    _register_subscriber();

    while(!is_stopped() && __alive)
    {
        time_t now = time(0);

        if(now - last > 10 && now-_lastactivity<120)
        {
            if(!PCFG->_srv._prx_addr.empty())
            {
                _keep_alive();
            }
            PCFG->check_log_size();
            last = now;
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
        rec->ip = htonl(rule->second.ip4());
        return true;
    }


	for(const auto& w : PCFG->rules)
	{
		if(::strstr(dname, w.first.c_str()))
		{
			rec->ip = htonl(w.second.ip4());
			return true;
		}
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

    bool b = false;
    if(PCFG->_srv.onresponse) //response message
    {
        for(auto & it : m._responses)
        {
            const Message::Answer& a  = it;

            GLOGI("searching: " << a.name);
            auto dn = PCFG->rules.find(a.name);

            if(dn != PCFG->rules.end())
            {
                DnsCommon   dns;

                dns.header   = 'D';
                dns.sequence = ++_seq;
                dns.now      = time(0);
                dns.client   = uip.ip4();
                dns.prxip    = PCFG->_srv._prx_addr.ip4();
                dns.domainip = htonl(a.real_ip_host.s_addr);
                dns.sizee    = sizeof(dns);
                ::strcpy(dns.hostname,a.name);

                GLOGI("Q:DNS: C:"<<IP2STR(htonl(dns.client))<<"/"<<dns.client<<" ->redir1[" << IP2STR(dns.domainip) <<"/" << dns.hostname<< "] --->PRX:"<< PCFG->_srv._prx_addr  <<" = " <<dns.sizee );

                if(_try_connect(true)==false)
                {
                    GLOGE("Connection to proxy is not openned yet");
                    break;
                }

                AutoLock   __a(&_m);
                if(_prxsock.send((char*)&dns, sizeof(dns))!=sizeof(dns))
                {
                    _prxsock.destroy();
                    GLOGE("Error sending to proxy\n");
                }
                _lastactivity=time(0);
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
                dns.client   = htonl(uip.sin_addr.s_addr);
                dns.prxip    = PCFG->_srv._prx_addr.ip4();
                dns.domainip = _localaddr.ip4();
                dns.sizee    = sizeof(dns);
                ::strcpy(dns.hostname, q.name);

                GLOGI("R:DNS: C:"<<IP2STR(dns.client)<<"/"<<dns.client<<" ->redir2[" << IP2STR(dns.domainip) << "] ---->PRX:" <<PCFG->_srv._prx_addr <<" = " <<dns.sizee );

                if(_try_connect(true)==false)
                {
                    GLOGE("Connection to proxy is not openned yet");
                    break;
                }

                AutoLock  __a(&_m);
                if(_prxsock.send((char*)&dns, sizeof(dns))!=sizeof(dns))
                {
                    _prxsock.destroy();
                    GLOGE("Error usending to proxy\n");
                }
                _lastactivity=time(0);
                b = true;           //one name is OK
                break;
            }
            else
            {
				GLOGI(q.name << "not found in rules");
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
    AutoLock        __a(&_m);

    if(_prxsock.is_really_connected())
    {
        int bytes = _prxsock.sendall("#,",2); // keep connection
        if(bytes!=0)
        {
            GLOGE(": "<< "destroing connect " << PCFG->_srv._prx_addr.c_str() << ":" <<  PCFG->_srv._prx_addr.port() << "\n");
            _try_connect(0);
        }
        return;
    }
    _try_connect(0);

}

bool   Watcher::_try_connect(bool doit)
{
    _prxsock.destroy();
        bio_unblock(&_prxsock,1);
    if(_prxsock.raw_connect(PCFG->_srv._prx_addr, 8)==0)
    {
        GLOGE(": "<< "cannot connect " << PCFG->_srv._prx_addr.c_str() << ":" <<  PCFG->_srv._prx_addr.port() << "\n");
        return false;
    }

    if( _prxsock.is_really_connected())
    {
        _lastactivity=time(0);
        return true;
    }
    return false;
}

void   Watcher::_register_subscriber()
{
	std::vector<std::string>    localips;

	if(!PCFG->_srv.localip.empty())
	{
		_localaddr = SADDR_46(PCFG->_srv.localip.c_str());
		localips.push_back(PCFG->_srv.localip);
	}
	else
	{

		std::string                 token;
		std::string                 localaddr = sock::GetLocalIP();
		std::istringstream          iss(localaddr);
		SADDR_46                    local;

		while(getline(iss, token, ','))
		{
			localips.push_back(token);
			_localaddr = SADDR_46(token.c_str());
		}
	}


    if(_try_connect(true)>0)
    {
        std::string query;
        for(auto ip : localips)
        {
            query+="S";
            query+=ip;
            query+=",";
        }
        _prxsock.sendall(query.c_str(), query.length());
        GLOGD(query);
    }

}


