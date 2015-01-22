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

#include <assert.h>
#include <algorithm>
#include "tinyclasses.h"
#include "ctxthread.h"
#include "threadpool.h"
#include "config.h"
#include "ctxqueue.h"
#include "context.h"
#include <strutils.h>

//====================================================================
ClientThread::ClientThread(ThreadPool* tp, int idx,
                           int dynamic):_tp(tp),
                                         _index(idx),
                                         _dynamic(dynamic),
                                         _locked(0),
                                         _flushed(false),
                                         _inbytes(0),
                                         _outbytes(0),
                                         _owned(false)

{
}

ClientThread::~ClientThread()
{

    cout << "T<---}\n";
    this->stop_thread();
}

void callClear(Context* p){
    p->clear();
}

void ClientThread::signal_to_stop()
{
    OsThread::signal_to_stop();

    AutoLock a(&_m);
    std::for_each(_pctxs.begin(), _pctxs.end(), callClear);
}

void ClientThread::stop_thread()
{
    OsThread::stop_thread();
}

bool ClientThread::_pre_thread_foo()
{
    _tp->inc();
    //preload  singletons modules if configured
    // therefore we keep them from destroy when lasy context releases them

    if(GCFG->_pool.cache_singletons){
        const SoMap& rm =__modules->get_modules();
        SoMapIt it = rm.begin();
        for(; it != rm.end(); it++){
            kchar* ks = (*it).first.c_str();
            SoEntry* pe = (*it).second;
            IModule* pm = pe->_so_get(ks);
            if(pm && pm->is_singleton()){
                _pmodules.push(pm);
            }
        }
    }
    return true;
}

int ClientThread::_get_from_q(CtxQueue* pq, size_t maxctxes)
{
     if(_pctxs.size() < maxctxes && pq->size())
     {
        Context* pc;
        if(pq->pop(&pc)) {
            if(0 == pc)
                return -1;
            _pctxs.push(pc);
            return 1;
        }
     }
     return 0;
}
//
// we dont use eool/pool couse won wont handle more than 1000 conections/thread.
//
void ClientThread::thread_main()
{
   // FT();
    time_t      tout = (time_t)GCFG->_pool.time_out + (rand() % 4/*add some variation in lifetime*/);
    time_t      st = time(0);
    size_t      maxctxes = _tp->capacity();
    CtxQueue*   pq = _tp->get_q();
    fd_set      rd,wr;
    int         is, ndfs;
    time_t      secbeat,secs=time(0);
    bool        sectick=false;
    timeval     tv {0,0xFFF};
    bool        pushed =false;

    ::fix(maxctxes, (size_t)1,(size_t) MAX_CTXES);
    FD_ZERO(&rd);
    FD_ZERO(&wr);

    while(!this->_bstop) {
        sectick = false;
        secbeat = time(0);
        if(secbeat - _lat > tout){

            secs  = secbeat-_lat;
            this->_commit_stats(secs);
            _lat = secbeat;
            sectick=true;
        }
        // look at it ???
        int ksz = 0;
        if(_tp->is_locked())
        {
            usleep(0x1FFF);
           // continue;
        }

        if(!_owned) //when 2 dont get from q
        {
          //  cout << "! owned\n";
            AutoLock a(&_m);
            int qn = _get_from_q(pq, maxctxes);
            if(qn == -1) break;
            ksz = _pctxs.size();
        }
        else
        {
         //   cout << "was locked\n";
            AutoLock a(&_m);
            ksz = _pctxs.size();
        }

        if(0 == ksz)
        {
            if(_owned)
            {
            //    cout << "releasing owned\n";
                _owned=false;
            }
            if(_dynamic && _lat - st > tout/*time_out for dynamic*/)
            {
                break;
            }
            usleep(0x1FFF);
            continue;
        }
        //
        // FD_SET
        //
        ndfs = 0;
        FD_ZERO(&rd);
        FD_ZERO(&wr);

        for(size_t k=0; k<ksz; k++) {
            ndfs = max(_pctxs[k]->set_fd(this, rd,wr), ndfs);
        }
        ++ndfs;

        //
        // FD_ISSET
        //
        tv.tv_sec = 0;
        tv.tv_usec = 0xFFF;
        if(ndfs)
            is = ::select(ndfs+1, &rd, &wr, 0, &tv);
        else
            is = 0;
        if(is < 0){
            if(errno == EINTR){
                usleep(0x1F);
                continue;
            }
            perror("select");
            break;
        }
        if(is > 0) {
            st = time(0);
            for(size_t k=0; k < ksz; k++) {

                assert(_pctxs[k]);
                size_t can = _pctxs[k]->is_fd_set(rd, wr);
                if(can)
                {
                    AutoLock a(&_m);
                    _locked = _pctxs[k]; //owns the sq engine
                        _pctxs[k]->spin((CAN_RW)can);
                    _locked = 0;

                    if(_flushed) {
                        _flushed = false;
                        break;
                    }
                }
            }//for spin
        }
        //
        // delte closed connection and finshed contexts
        //
        for (int k =0; k < ksz; ++k) {
            if(0 == _pctxs[k]) continue;
            if(_pctxs[k]->clear_fd(rd, wr, sectick, secs)!=_CONTINUE) {
                AutoLock a(&_m);
                delete _pctxs[k];
                _pctxs.remove(k);
                pushed=true;
            }
        }
        if(pushed)
        {
            AutoLock a(&_m);
             ksz = _pctxs.size();
             cout << "ctxes ="<< ksz<<" \n";
             pushed=false;
        }

        //
        // default loop sleep. keep low profile
        //
        usleep(0x1F);
    }//while THREAD SPIN

    //
    // clean up, we've exit
    //
    if(_pctxs.size()) {
        AutoLock a(&_m);
        for(size_t k=0; k < _pctxs.size(); k++) {
             delete _pctxs[k];//_cxleng;
        }
        _pctxs.clear();
    }
}

//
// clean thread context loaded libraries and values
//
void ClientThread::_post_thread_foo()
{
    for(size_t k=0; k < _pmodules.size(); k++)
    {
        kchar* by_ext = _pmodules[k]->__cookie();
        const SoEntry* pse = __modules->get_module(by_ext);
        pse->_so_release(_pmodules[k], by_ext);
    }
    _pmodules.clear();
    _tp->remove_thread(this);
}

//
// move all contexts to other threads
//
void     ClientThread::purge(time_t t, time_t tout)
{
    if(t - _lat > tout) {
        AutoLock a(&_m);
        if(_locked != 0) {

        do{
            int k = _pctxs.size();

            while(--k >= 0) {
                if(_pctxs[k] == _locked )continue;
                _tp->enqueue(_pctxs[k]);
                break;
            }
            _pctxs.remove(k);

        }while( _pctxs.size() > 1);
            _flushed = true;
        }
    }
}

void    ClientThread::checkin(size_t rec, size_t sent)
{
    _inbytes += rec;
    _outbytes += sent;
    _bpss[0] += rec;
    _bpsr[0] += sent;
}

void     ClientThread::_commit_stats(time_t diff)
{
    //if(0 == diff)diff=1; //division with 0
    //diff*=1024;
    _bpss[1] = _bpss[0]/diff;
    _bpsr[1] = _bpss[0]/diff;

    _bpss[0] = 0;//_bpss[0]/diff;
    _bpsr[0] = 0;//_bpss[0]/diff;
}

void     ClientThread::get_report(std::ostringstream& s)
{
    AutoLock a(&_m);


    s << "<table border=1 cellspacing='1' bgcolor='#CCCCCC' width=80%>";
    s << "<tr><th>queue</th><th>contexts</th><th>bytes in</th><th>bytes out</th><th>bps/in</th>";
    s << "<th>bps/out</th><th>time</th></tr>";
    s << "<tr>";
    s << "<th>" << _queue.size() << "</th>";
    s << "<th>" << _pctxs.size() << "</th>";
    s << "<th>" << _inbytes << "</th>";
    s << "<th>" << _outbytes << "</th>";
    s << "<th>" << _bpss[1] << "</th>";
    s << "<th>" << _bpsr[1] << "</th>";
    s << "<th> na </th>";
    s << "</tr>";

    s << "</table>";
}
//<div style='background-color:#FF7777;;border:0;width:7.2944297082228;' > 7% </div>
