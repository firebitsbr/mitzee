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

#ifndef _WATCHER_H_
#define _WATCHER_H_

//-------------------------------------------------------------------------------
#include <string>
#include <set>
#include <map>
#include <vector>
#include <sock.h>
#include "message.h"
#include "config.h"

//-------------------------------------------------------------------------------
struct DnsRecord
{
    u_int32_t   ip;
    char        sip[16];
};


//-------------------------------------------------------------------------------
struct Context
{
    SADDR_46    _cliip;
    u_int32_t   _time;
    u_int16_t   _seq;
    u_int16_t   _newseq;
    string      _dnsip;
    string      _msig;
    int         _errors;
};


class Watcher : public OsThread
{
public:
    Watcher();
    virtual ~Watcher();
    virtual void thread_main();

    bool    is_dn_for_prx(const char* dname, DnsRecord*);
    bool    https_notify_proxy( Message& m, int c ,const SADDR_46& uip);
    bool    cache(Message& m);
    bool    is_cached(const Message& m);
    bool    is_dns_addr(const SA_46& inaddr);
    const char* get_dns_ip()const {
    return PCFG->_srv.nextdnss[_u_dnsidx].c_str();
    }
    const char* get_next_dns_ip(){
        size_t ss = PCFG->_srv.nextdnss.size();
        if(++_u_dnsidx > ss)
            _u_dnsidx = 0;
        return get_dns_ip();
    }
    size_t get_dnss_count()const{return PCFG->_srv.nextdnss.size();};

private:
    void    _keep_alive();
    bool   _try_connect(bool doit);
    void   _register_subscriber();
private:

    SADDR_46                    _localaddr;
    std::map<SADDR_46,time_t>   _users;
    std::set<SADDR_46>          _blocked;
    std::set<std::string>       _for_prx;
    std::set<std::string>       _https_dn;
    size_t                      _u_dnsidx;
    mutex                       _m;
    tcp_cli_sock                _prxsock;
    int                         _errors;
    uint32_t                    _seq;
    bool                        _reconnow;
    time_t                      _lastactivity;
};

extern Watcher* __pw;



#endif // RESOLVER_H
