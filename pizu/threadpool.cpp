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

#include <iostream>
#include <sstream>
#include "main.h"
#include "threadpool.h"
#include "ctxthread.h"
#include "context.h"
#include "ctxqueue.h"
#include "listeners.h"
#include <sock.h>
#include <string>

extern Context _fc;

ThreadPool::ThreadPool(bool sync):_sync(sync),_alive(0),_max_clients(0),_robin(0),_lock(false)
{
    _count   = GCFG->_pool.min_threads;
    _maxcount = GCFG->_pool.max_threads;
    _max_clients = GCFG->_pool.clients_perthread;

    ::fix(_count, (size_t)1, (size_t)THREADS_CNT);
    ::fix(_maxcount, (size_t)1, (size_t)MAX_THREADS);
    ::fix(_max_clients, (size_t)1, (size_t)MAX_CTXES);

    //_q.set_sync_mode(sync);//config the queue how the thread pool works

    printf("Starting:\t\t%zd out of max:%zd\n", _count, _maxcount);
    for(size_t i=0; i<_count; i++) {
        add_thread(0);
    }
}

ThreadPool::~ThreadPool()
{
    //for all threads
    _commit_log();
    size_t alive = _pool.size();
    do {
        AutoLock a(&_m);
        for(size_t i=0; i<alive; i++) {
            _pool[i]->queue().set_sync_mode(true);
            _pool[i]->signal_to_stop();
        }
    } while(0);

    // send the eos context
    time_t tout = time(0) + 32;

    //_q.set_sync_mode(true);   //make threads to wait
    while(count() && _alive && time(0) < tout) {
        _q.push(0);
        //enqueue(0);
        usleep(0xFFF);
    }
    _pool.clear();
    printf("~THREAD::POOL leftover threads %lu \n", _alive);
}

ClientThread* ThreadPool::add_thread(int dynamic)
{
    if(_pool.size() < _maxcount) {
        AutoLock a(&_m);
        ClientThread* ct = new ClientThread(this, _pool.size(), dynamic);
        _pool.push_back(ct);
        ct->start_thread();
        return ct;
    }
    return 0;
}

bool    ThreadPool::add_context(Context* pc)
{
    lock(true);
    size_t threads = count();
    if(count() < _maxcount)
    {
        ClientThread* pct;
        for(size_t k=0; k < threads; ++k)
        {
            pct = _pool[k];
            if(pct->set_context(pc))
            {
                lock(false);
                return true;
            }
        }

        if((pct = add_thread(1)) != 0)
        {
            pct->set_context(pc);
        }
        lock(false);
        return true;
    }
    return false;
}


void    ThreadPool::remove_thread(ClientThread* pt)
{
    AutoLock a(&_m);
    vector<ClientThread*>::iterator b = _pool.begin();
    for(; b!=_pool.end(); b++) {
        if((*b)==pt) {
            dec();
            delete pt;
            _pool.erase(b);
            break;
        }
    }
}

int    ThreadPool::enqueue(Context* pc)
{
    AutoLock a(&_m);

    //_robin = ++_robin % _pool.size();
    //return _pool[_robin]->queue().push(pc);
    return _q.push(pc);
}

void ThreadPool::get_report(std::ostringstream& s)
{
    AutoLock a(&_m);

    for(size_t i=0; i<_pool.size(); i++) {
        _pool[i]->get_report(s);
    }
}


bool ThreadPool::_pre_thread_foo()
{
    return true;
}

void ThreadPool::_post_thread_foo()
{
}

void ThreadPool::stop_thread()
{
    OsThread::stop_thread();
}

void ThreadPool::signal_to_stop()
{
    OsThread::signal_to_stop();
}

void ThreadPool::thread_main()
{
    time_t tout = (time_t)GCFG->_pool.time_out;
    time_t now = time(0);
    ::fix(tout, (time_t)16, (time_t)120);

    while(!this->is_stopped()){
        if(time(0) - now > tout){
            _commit_log();
            now = time(0);
        }
        sleep(1);
    }

}

void    ThreadPool::_commit_log()
{
    AutoLock __a(&_m);

    if(_logcntn.tellp() == 0)
        return;
    char    logf[512];
    sprintf(logf, "logs/mariux.log0");
    FILE* pf = fopen(logf,"ab");
    size_t flen;
    if(pf) {
        fwrite(_logcntn.str().c_str(), 1, _logcntn.str().length(), pf);
        flen = ftell(pf);
        fclose(pf);
    }
    if(flen > 2000000) {
        _rollup_logs();
    }
    _logcntn.clear();
    _logcntn.seekp(0);
}

void ThreadPool::_rollup_logs()
{
    char oldFn[256];
    char newFn[256];

    int iold = 20;
    sprintf(oldFn,"logs/mariux.log20");
    unlink(oldFn);

    while(iold-- > 0) {
        sprintf(newFn, "logs/mariux.log%d",iold);
        sprintf(oldFn, "logs/mariux.log%d",iold-1);
        rename(oldFn, newFn);
    }
}
inline void  str_time(char* dTime)
{
    time_t  curtime = time(0);
    strcpy(dTime, ctime(&curtime));
    char *pe = strchr(dTime,'\r'); if(pe)*pe=0;
        pe = strchr(dTime,'\n'); if(pe)*pe=0;
}

void ThreadPool::log(kchar* pp, ...)
{
    if(!__tp)   return;
    register kchar* p = pp;
    char     logf[256];
    char     time[128];
    register kchar* pw = pp;
    register size_t lines = 0, chars=0;
    register size_t maxchars=sizeof(logf)-2;

    while(*pw++ && lines<4 && ++chars<254){
        if(*pw=='\r'||*pw=='\n'){
            ++lines;
            continue;
        }
        if(*pw<' ' || *pw>'z')break;
    }
    if(lines && pw>pp)
    {
        maxchars = min(size_t(254), size_t(pw-p)-1);
    }

    va_list vl;
    va_start(vl, pp);
    vsnprintf(logf, maxchars, p, vl);
    va_end(vl);

    str_time(time);

    if(!strstr(logf,".jpg")){
        AutoLock __a(&_m);
        _logcntn << time <<"\t"<< logf << "\n ";
    }

}




