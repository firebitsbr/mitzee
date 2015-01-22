/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/
#ifndef ADMINMOD_H
#define ADMINMOD_H

#include <modiface.h>

//-----------------------------------------------------------------------------
// the non script module is in-engine
class AdminModule : public IModule
{
//    NON_COPYABLE(AdminModule);
public:
    AdminModule(const char* cook):IModule(cook),_pctx(0){};
    virtual ~AdminModule(){};

     bool   construct(CtxMod* pctx);
     int    work(CtxMod* pctx,CAN_RW rw);
     void   destroy();
     bool   is_singleton();
     bool   uses_fds()const;
     int    fd_action (FD_OPS op, fd_set& rd, fd_set& wr);

     static  AdminModule*  singleton_get(const char* cook);
private:
    CtxMod* _pctx;
};




#endif // CHUNKPROC_H
