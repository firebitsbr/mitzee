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

#include "theapp.h"

#define      BEGIN_SEQ                100
Watcher*     theapp::__pw           = 0;
u_int32_t    theapp::__dns_tout     = 5; // 5 seconds
int          theapp::__sessionprx   = 600; // 600 seconds
int          theapp::__gerrors      = 0;
static int   DNS_DEFPORT            = 53;
extern bool __alive;


void theapp::ControlC (int i)
{
    __alive = false;
    printf("^C. please wait. terminating threads\n");
    sleep(3);
    system ("pkill -kill mariuz");
}

static std::string dummy;
static int idummy=-1;
theapp::theapp():_db(100,100, dummy,dummy,dummy,dummy,dummy,dummy,idummy),_gseq(BEGIN_SEQ)
{
    signal(SIGINT,  theapp::ControlC);
    signal(SIGABRT, theapp::ControlC);
    signal(SIGKILL, theapp::ControlC);
    signal(SIGTRAP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    __pw=&_tp;
}

theapp::~theapp()
{
    //dtor
}


int theapp::run()
{

    time_t          now=0, tick;
    time_t          ses=0;
    Message         m;

                              // delete unrespoded queries
    _tp.start_thread();
    sleep(2);

    while(__alive)
    {
        tick = time(0);
        if(!_buzy_socket.ensure())
            continue;

        if(_buzy_socket.pool()==1)
        {
            _io(m);
        }
        else
        {
            if(tick - now > __dns_tout)
            {
                now = tick;
                _flush(now);                                  // delete unrespoded queries
            }
            usleep(0xFFF);
        }

    }
    return 0;
}

void theapp::_io(Message& m)
{
    SADDR_46    fromaddr;

    m.clear();
    int sz = _buzy_socket.rec_all((unsigned char*)m._buff, BSZ, fromaddr);
    if(sz > 0)
    {
        m.parse(sz);

        if(__pw->is_dns_addr(fromaddr))
        {
            _from_dns(fromaddr, m);
        }
        else
        {
            if(m.is_cahced())
            {
                _cli_send_fromcache(fromaddr, m);
            }
            else
            {
                _from_client(fromaddr, m);
            }
        }
    }
}


//-------------------------------------------------------------------------------
void theapp::_flush( time_t now)
{
    bool swapped=false;

    auto exp_ctx = _ctexes.begin();
    for(; exp_ctx != _ctexes.end(); )
    {
        Context& ctx = (*exp_ctx).second;
        if(now - ctx._time > __dns_tout)
        {
            _ctexes.erase(exp_ctx++);
            if(swapped==false)
            {
                swapped=true;
                __pw->get_next_dns_ip();
            }
            GLOGD("deleting record. DNS timoeut");
        }
        else
        {
            ++exp_ctx;
        }
    }
}


void theapp::_cli_send_fromcache(const SADDR_46& cliaddr, Message& m)
{
    std::string  requestsig = m._mid.str();
    u_int16_t    hdrid =  m._h.id;

    m.clear();
    m.load(requestsig, cliaddr);
    m.set_id(hdrid);

    _tp.https_notify_proxy(m, 0, cliaddr); ///??????

    if(m._sz != _buzy_socket.send(m._buff, m._sz, cliaddr))
    {
        GLOGE("cannot send to client:" << IP2STR(cliaddr) << "err:" <<_buzy_socket.error());
        return;
    }
    GLOGD(cliaddr.c_str() << ":" << cliaddr.port() << ", [" << hdrid << "]<----[DNS-CACHE]");
}

void theapp::_from_dns(const SADDR_46& dnsaddr, Message& m)
{
    SADDR_46                                to_addr;
    u_int16_t                               seq = m._h.id;
    auto  exp_ctx = _ctexes.find(seq);

    if(exp_ctx == _ctexes.end())
    {
        GLOGD("sequence << seq << not found from:" << IP2STR(dnsaddr));
        return ;
    }
    const Context& ctx = (*exp_ctx).second;

    m.set_id(ctx._seq);
    to_addr = ctx._cliip;

    if(PCFG->rules.size() && m.replace_domains())
    {
        _tp.https_notify_proxy(m, 1, to_addr);
    }

    if(m._sz != _buzy_socket.send(m._buff, m._sz, to_addr))
    {
        GLOGE("cannot send to client:" << IP2STR(to_addr) << "err:" <<_buzy_socket.error());
    }
    else
    {
        GLOGD(to_addr.c_str() << ":" << to_addr.port() << ", [" << ctx._seq << "]<----[DNS]<---" << dnsaddr.c_str() << ", [" <<seq<<", ("<<m._sz<<")]");
        if(!PCFG->_srv.cache.empty())
        {
            m.cache(ctx._msig, to_addr);
        }
        _ctexes.erase(exp_ctx);
    }
}

//-------------------------------------------------------------------------------
void theapp::_from_client(const SADDR_46& inaddr, Message& m)
{
    const char* dns_ip = __pw->get_dns_ip();
    Context     ctx;

    ctx._seq    = m._h.id;
    ctx._newseq =  _gseq;
    ctx._errors = 0;
    ctx._time   = time(0);
    ctx._cliip  = inaddr;
    ctx._msig   = m._mid.str();
    m.set_id(_gseq);


    int errors = __pw->get_dnss_count(); // how many master dns entries we have
    while(--errors>=0)
    {
        ctx._dnsip = dns_ip;

        if(m._sz != _buzy_socket.send(m._buff, m._sz, DNS_DEFPORT, dns_ip))
        {
            GLOGE("cannot send to dns:" << dns_ip << "err:" << _buzy_socket.error());
            dns_ip = __pw->get_next_dns_ip();
            ++ctx._errors;
        }
        else
        {
            GLOGD(ctx._cliip.c_str()<<":"<<ctx._cliip.port() << ",["<<ctx._seq << "]--->[DNS]---> " <<  dns_ip << ":53, [" << ctx._newseq <<", ("<< m._sz <<")]");
            _ctexes[_gseq] = ctx;
            break;
        }
    }
    if(++_gseq>=65534)
        _gseq = BEGIN_SEQ;
}

