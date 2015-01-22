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

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "os.h"
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <list>
#include <algorithm>
#include "ctxqueue.h"

//using namespace std;


#define MAX_THREADS 256
#define THREADS_CNT 64

class ClientThread;
class ThreadPool : public OsThread
{
public:
    ThreadPool(bool sync);
    virtual ~ThreadPool();


    void       dec(){
        AutoLock a(&_m);
        --_alive;
    }
    void       inc(){
        AutoLock a(&_m);
        ++_alive;
    }
    CtxQueue*  get_q(){return &_q;};
    int     enqueue(Context*);
    size_t  count(){
        AutoLock a(&_m);
        return _alive;
    }
    size_t  capacity()const{return _max_clients;}
    ClientThread*  add_thread(int dynamic);
    bool    add_context(Context* pc);
    void    remove_thread(ClientThread*);
    const   ClientThread* thread(int index){ return _pool[index];}
    void    get_report(std::ostringstream& s);
    void    log(kchar*, ...);
    void    stop_thread();
    void    signal_to_stop();
    void    thread_main();
    void    lock(bool b){
        AutoLock a(&_m);
        _lock = b;
    }
    bool    is_locked()const {
        AutoLock a(&_m);
        return _lock;
    }
private:
    bool _pre_thread_foo();
    void _post_thread_foo();
    void _commit_log();
    void _rollup_logs();

private:
    bool                    _sync;
    size_t                  _maxcount;
    size_t                  _count;
    size_t                  _alive;
    size_t                  _max_clients;
    size_t                  _robin;
    bool                    _lock;
    vector<ClientThread*>   _pool;
    CtxQueue                _q;
    mutex                   _m;
    std::ostringstream      _logcntn;
};


#endif //
