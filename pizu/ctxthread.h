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

#ifndef CTXTHREAD_H
#define CTXTHREAD_H
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <list>
#include <algorithm>
#include "tinyclasses.h"
#include "ctxqueue.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class Context;
class CtxQueue;
class ThreadPool;
class IModule;
class ClientThread : public OsThread
{
public:
    friend class ThreadPool;
    friend class Context;
    ClientThread(ThreadPool* tp, int index, int dynamic);
    virtual ~ClientThread();
    int     index()const
    {
        return _index;
    }
    void    sc_write(kchar* s);
    int     sc_getid();
    void    get_report(std::ostringstream& s);
    void    purge(time_t t, time_t tout);
    CtxQueue& queue()
    {
        return _queue;
    }
    void    checkin(size_t rec, size_t sent);
    bool set_context(Context* pctx)
    {
        AutoLock a(&_m);
        _lat = time(0);
        if(_pctxs.size()==0)
        {
            _owned=true;
            _pctxs.push(pctx);
            return true;
        }
        return false;
    }

private:
    bool    _pre_thread_foo();
    void    _post_thread_foo();
    void    stop_thread();
    void    signal_to_stop();
    void    thread_main();
    void    _modules_load();
    int     _get_from_q(CtxQueue* pq, size_t maxctxes);
    void   _commit_stats(time_t secs);
private:
    ThreadPool* _tp;
    int         _index;
    int         _dynamic;
    bool        _owned;
    time_t      _lat;
    Context*    _locked;
    bool        _flushed;
    int64_t     _inbytes;
    int64_t     _outbytes;
    int64_t     _bpss[2];
    int64_t     _bpsr[2];

    mutex       _m;
    CtxQueue    _queue;
    Bucket<Context*, MAX_CTXES>  _pctxs;
    Bucket<IModule*,MAX_MODULES> _pmodules;//kepp a steady reference fo all singleton modules
    // if configured
};

#endif // CTXTHREAD_H
