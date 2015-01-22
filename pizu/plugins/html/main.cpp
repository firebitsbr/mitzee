/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/

#include "htmlmod.h"

extern "C"
{

    IModule* factory_get(const char* cookie)
    {
        IModule* pm = new HtmlModule(cookie);
        if(pm)
            return pm;
        return 0;
    }

    void     factory_release(IModule* pm, const char* cookie)
    {
        //unused const char* cookie
        pm ? delete pm : void(0);
    }
}
