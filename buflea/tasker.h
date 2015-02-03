#ifndef TASKER_H
#define TASKER_H

#include <os.h>

class tasker : public OsThread
{
public:
    typedef enum E_TASKS{
        eNA=0,
        eREDNS_REDIRECTS=1,
    }E_TASKS;

    tasker();
    virtual ~tasker();
    void    schedule(E_TASKS et, time_t t=0);
protected:
    virtual void thread_main();
    private:


    E_TASKS  _dotask;
    time_t  _when;
    mutex   _m;
};
extern tasker* __task;

#endif // TASKER_H
