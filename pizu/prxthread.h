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

#ifndef PRXTHREAD_H
#define PRXTHREAD_H
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <list>
#include <algorithm>
#include "tinyclasses.h"
#include "ctxqueue.h"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class ProxyContext;
class CtxQueue;
class ThreadPool;
class PrxThread : public ThreadBase
{
public:

    friend class ThreadPool;
    friend class ProxyContext;
    PrxThread(ThreadPool* tp, int index, bool dynamic);
    virtual ~PrxThread();
private:
    bool    _pre_thread_foo();
    void    _post_thread_foo();
    void    stop_thread();
    void    signal_to_stop();
    void    thread_main();
private:
    Bucket<ProxyContext*, MAX_CTXES>  _pctxs;
 };

#endif // CTXTHREAD_H
