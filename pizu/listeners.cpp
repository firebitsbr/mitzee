/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(udfjj39546284@gmail.com) - getic.net - N/A
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



#include <map>
#include <regex>
#include <iostream>
#include <sstream>
#include <tr1/regex>
#include <signal.h>
#include <consts.h>
#include <stdlib.h>
#include <fcntl.h>
#include "main.h"
#include "config.h"
#include "listeners.h"
#include "context.h"
#include "ctxqueue.h"
#include "threadpool.h"
#include "sslcrypt.h"

using namespace std;
using namespace std::tr1;


static kchar HTTP_401[] = "HTTP/1.1 401 Unauthorized\r\n"
                          "Connection: close\r\n"
                          "Content-Type: text/html; charset=utf-8\r\n"
                          "Server: mariux/1.0\r\n\r\n"
                          "<H2>401 Unauthorized!</H2>\r\n";
static const size_t nHTTP_401 = strlen(HTTP_401);

extern bool __alive;
Listeners::Listeners(ThreadPool* pa):_ss(0),_pa(pa),
    _pssl(0),_sanity(true)
{
    _pqa = pa->get_q();
}

Listeners::~Listeners()
{
    if(_pssl)
        delete _pssl;
    stop_thread();
}

bool Listeners::initSsl()
{
    if(0 == _pssl) {
        _pssl = new SslCrypt();
        if(!_pssl->init()) {
            delete _pssl;
            _pssl = 0;
            return false;
        }
    }
    return true;
}

bool CancelAutoConnect(void* pVoid, unsigned long time)
{
    UNUS(pVoid);
    return time < 5;
}

int  Listeners::start_thread()
{
    map<int, SrvSock*>        dups;
    map<string, Conf::Vhost>& rh = GCFG->_hosts;
    int                       check = 30;
    int                       pending = GCFG->_marius.pending;

    ::fix(pending, 32, 256);
again:
    _clear();
    dups.clear();
    if(!__alive) {
        return -1;
    }
    map<string, Conf::Vhost>::iterator it = rh.begin();
    for(; it!= rh.end(); it++) {
        Conf::Vhost& ph = (*it).second;

        map<int, SrvSock*>::iterator f = dups.find(ph.port);
        if(f != dups.end()) {
            (*f).second->_vhss[(*it).first]=&ph;
            continue;
        }
        SrvSock*  pss = new SrvSock(ph.port);
        pss->_vhss[(*it).first] = &ph; //on same port mutiple vhosts

        dups[ph.port] = pss;

        if(-1==pss->create(ph.port, SO_REUSEADDR, 0)) {
            printf("Port:\t\t%zd Error: %d. Trying: %d/30\n",ph.port,
                   (int)pss->error(), 30-check);
            sleep(4);
            if(check-->0) {
                delete pss;
                goto again;
            }
            delete pss;
            return -1;
        } else {
            fcntl(pss->socket(), F_SETFD, FD_CLOEXEC);
            pss->set_blocking(0);
            if(pss->listen(pending)==0) {
                _ss.push_back(pss);
            } else {
                printf("Server Listen:\t\t Port:%zd error(%d)\n", ph.port, pss->error());
                delete pss;
                return -1;
            }
            printf("\n");
        }
    }

    //deal with ssl
    vector<SrvSock*>::iterator it2 = _ss.begin();
    for(; it2<_ss.end(); it2++) {
        printf("Server Listen:\t\t Port:%zd\n", (*it2)->_port);

        map<string, Conf::Vhost*>::iterator vhss = (*it2)->_vhss.begin();
        for(; vhss != (*it2)->_vhss.end(); ++vhss) {
            printf("\t\t\t -host_conf:\t%s:%zd(%d) SSL=",vhss->second->host.c_str(),
                   vhss->second->port, vhss->second->iptabled);
            if((*vhss).second->ssl) {
                printf("on");
                if(initSsl()) {
                    (*it2)->setSsl(true);
                    printf("- SSL OK");
                } else {
                    printf("- SSL ERROR");
                }
            } else {
                printf("off");
            }
            printf("\n");
        }//for vhs
    }//for listeners
    return OsThread::start_thread();
}

void Listeners::_clear()
{
    vector<SrvSock*>::iterator it = _ss.begin();
    for(; it<_ss.end(); it++) {
        (*it)->destroy();
        delete *it;
        usleep(0xFFFF);
    }
    _ss.clear();
}

void Listeners::stop_thread()
{
    //FT();
    AutoLock a(&_m);

    OsThread::signal_to_stop();
    //printf("%s\n",__PRETTY_FUNCTION__);
    usleep(0x1FFFF);
    vector<SrvSock*>::iterator it = _ss.begin();
    for(; it<_ss.end(); it++) {
        (*it)->destroy();
        delete *it;
        usleep(0x1FFFF);
    }
    OsThread::stop_thread();
}

//
// we dont use eool/pool couse won wont listen more than 1000 incomming connections.
//

void Listeners::thread_main()
{
    fd_set  rd;
    int     ndfs;// = _count + 1;
    timeval tv {0,16384};
    time_t  tnow = time(NULL);
    int     conspesec = 0;

    printf("RUNNING\n");
    while(!_bstop) {
        ndfs = 0;
        FD_ZERO(&rd);
        vector<SrvSock*>::iterator it = _ss.begin();
        for (; it != _ss.end(); it++) {
            if(!(*it)->isopen())goto DONE;
            FD_SET((*it)->socket(), &rd);
            ndfs = max((*it)->socket(), ndfs);
        }
        tv.tv_usec = 0xFFF;//must be reinitialised here http://linux.die.net/man/2/select
        usleep(0x3F); // octad system -> 2000 conects/sec
        int is = ::select(ndfs+1, &rd, 0, 0, &tv);
        if(is ==-1) {
            if(errno == EINTR) {
                usleep(0x1F);
                continue;
            }
            perror("select");
            _sanity=false;
            break;
        }
        if(is == 0) {
            usleep(0xFF);
            continue;
        }
        it = _ss.begin();
        for(; it!=_ss.end(); it++) {
            if(!FD_ISSET((*it)->socket(), &rd)) continue;

            Context* pc = new Context((*it));//hold on to get the config
            if(pc == 0) {
                perror("new");
                __alive=false;
                break;
            }
            if((*it)->accept((tcp_cli_sock&)pc->sck())>0) {
                usleep(0x1F);
                //stats
                ++(*it)->_acons;
                if(time(NULL)- tnow > 1) { //calc cons/sec
                    (*it)->_cons = (*it)->_acons;
                    (*it)->_acons = 0;
                    tnow = time(NULL);
                }
                if(_allowed(pc)) {

                    if(GCFG->_marius.dedicated == (int)(*it)->port())
                    {
                        _new_thread(pc,conspesec);
                    }
                    else
                    {
                        _put_inqueue(pc, conspesec);
                    }
                }
                else
                {
                    //Context->send(HTTP_401, nHTTP_401);
                    delete pc;
                }
            } else {
                printf("ACCEPT FAILED \n");
                delete pc;
            }
            FD_CLR((*it)->socket(), &rd);

        }//for
    }//while alive
DONE:
    _sanity=false;
}


void Listeners::_new_thread(Context* pc, int ignore)
{
    pc->set_connected(_pssl ? _pssl->ctx() : 0);
    if(!_pa->add_context(pc))
    {
        printf("QUEUEA OVERFLOW. %zd CONNN REJECTED \n", _pqa->size());
        delete pc;
    }

}


void Listeners::_put_inqueue(Context* pc, int every_sec)
{
    pc->set_connected(_pssl ? _pssl->ctx() : 0);

    int r = _pqa->push(pc); //One thd per ctx
    if((0 == every_sec) && r==0 && _pa->add_thread(true))
        return;

    if(-1 == r) { //queu overflow
        printf("QUEUEA OVERFLOW. %zd CONNN REJECTED \n", _pqa->size());
        delete pc;
    }
    /*

    int r = _pa->enqueue(pc);
    if((0 == every_sec) && r==0 && _pa->add_thread(true))
        return;

    if(-1 == r) { //queu overflow
        printf("QUEUEA OVERFLOW. CONNN REJECTED \n");
        delete pc;
    }
    */

}

bool Listeners::_allowed(Context* pc)
{
#if 0
    std::regex          rex;
    std::string         rip(pc->sck().ssock_addrip());
    bool                rv = true;
    std::set<string>*   pad[2];

    if(GCFG->_marius.order == 1) {
        pad[0] = &GCFG->_marius.allow;
        pad[1] = &GCFG->_marius.deny;
        rv = true;
    } else {
        pad[1] = &GCFG->_marius.deny;
        pad[2] = &GCFG->_marius.allow;
        rv = false;
    }
    for(int k=0; k<2; k++) {
        set<string>::iterator it = pad[k]->begin();
        for(; it != pad[k]->end(); it++) {
            rex = *it;
            if(std::regex_match(rip.begin(), rip.end(), rex)) {
                return rv;
            }
        }
        rv=!rv;
    }
    return false;
#endif
    return true;
}

//search for virtual host
const Conf::Vhost* SrvSock::_vHoost(kchar* name)const
{
    char    lname[256];
    char    local[256];
    int     port = 80;

    sprintf(lname, "%s", name ? name : "*");
    char* pd = _ttstrchr(lname,':');
    if(pd) {
        *pd=0;
        if(*(pd+1)) {
            port = ::_ttatoi(pd+1);
            if(0 == port)
                port = 80;
        }
    }


    // lname=host port=#
    if(isdigit(lname[0]) || !strcmp(lname,"localhost"))
    {
        ::strcpy(local,"*");
    }
    else
    {
        ::strcpy(local,lname);
    }

    map<string, Conf::Vhost*>::const_iterator it = _vhss.find(local);

    if(it != _vhss.end() && ((*it).second->port==port || (*it).second->iptabled==port))
    {
        return (*it).second;
    }

    // no hosts names, get the default
    ::strcpy(local,"*");
    it = _vhss.find(local);
    if(it != _vhss.end() && ((*it).second->port==port || (*it).second->iptabled==port))
    {
        return (*it).second;
    }

    throw Mex(501,__FILE__,__LINE__);
    return 0;
}


void    Listeners::get_report(std::ostringstream& s)
{
    AutoLock a(&_m);

    vector<SrvSock*>::iterator it = _ss.begin();

    s << "Listenrs count:........" << _ss.size() << "<br>\n";
    for(; it<_ss.end(); it++) {
        s << "-port:........" << "" << "<br>\n";
        s << "-host:........" << "" << "<br>\n";
        s << "-cons/sec:...." << (*it)->_cons << "<br>\n";
    }
    s << "Async Queue size:............" << _pqa->size() << "<br>\n";

    _pa->get_report(s);

}

