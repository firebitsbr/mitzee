/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/


#include "adminmod.h"

extern "C"
{
    IModule* factory_get(const char* cookie)
    {
        return AdminModule::singleton_get(cookie);
    }

    void     factory_release(IModule* pm, const char* cookie)
    {
        ;
    }
}
