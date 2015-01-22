#include <assert.h>
#include <algorithm>
#include "tinyclasses.h"
#include "threadpool.h"
#include "config.h"
#include "ctxqueue.h"
#include "prxctx.h"
#include <strutils.h>

//====================================================================
//====================================================================
PrxThread::PrxThread(ThreadPool* tp, int idx,
                           bool dynamic):ThreadBase(tp, idx, dynamic)
{
}

PrxThread::~PrxThread()
{
    this->stopThread();
}

static void callClear(ProxyContext* p){
    p->clear();
}

void PrxThread::signalStop()
{
    ThreadBase::signalStop();

    AutoLock a(&_m);
    //std::for_each(_pctxs.begin(), _pctxs.end(), callClear);
}

void PrxThread::stopThread()
{
    ThreadBase::stopThread();
}

bool PrxThread::preThreadFoo()
{
    _tp->inc();
    return true;
}

void PrxThread::threadMain()
{
   // FT();
    time_t      st = time(0);
    size_t      maxctxes = _tp->capacity();
    CtxQueue*   pq = _tp->theQueue();
    fd_set      rd,wr;
    int         ndfs;
    timeval     tv {0,0xFFF};

    ::fix(maxctxes, 1, MAX_CTXES);

    while(!this->_bstop) {
    }//while

}

//
// clean thread context loaded libraries and values
//
void PrxThread::postThreadFoo()
{
    _tp->popThread(this);
}
