#include "tasker.h"
#include "configprx.h"



extern  int __alivethrds;
extern  int __alive;
tasker* __task;

tasker::tasker()
{
    assert(__task==0);
    __task=this;
}

tasker::~tasker()
{
    __task=0;
}

void    tasker::schedule(E_TASKS et, time_t t)
{
    _when=t; // todo
    _dotask=et;
}

void tasker::thread_main()
{
    ++__alivethrds;
    while(!_bstop && __alive)
    {
        sleep(2);
        if(_dotask!=eNA)
        {
            AutoLock __a(&_m);


            switch(_dotask)
            {
                case eREDNS_REDIRECTS:
                    GLOGD("refreshing domains configured on redirect so wont overwelm one IP if the domain has more than one IP");
                    GCFG->refresh_domains();
                    break;
                default:
                    break;
            }
            _dotask=eNA;
        }
    }
    --__alivethrds;
    GLOGD("tasker thread exits");

}





