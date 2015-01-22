/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/

#include "sqmod.h"
extern "C"
{
    IModule* factory_get(kchar* cookie)
    {
        return SqMod::get_threaded_singleton(cookie);
    }

    void     factory_release(IModule* pm, kchar* cookie)
    {
        SqMod::release_threaded_singleton(pm, cookie);
    }
}
