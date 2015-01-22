/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/

#include <fcntl.h>
#include <dirent.h>
#include <sstream>
#include <assert.h>
#include "adminmod.h"
#include <strutils.h>
#include <consts.h>
#include <modiface.h>

//---------------------------------------------------------------------------------------
const char HTTP_200[] = "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/html; charset=utf-8\r\n\r\n";
static const size_t nHTTP_200 = strlen(HTTP_200);

//---------------------------------------------------------------------------------------
AdminModule*  AdminModule::singleton_get(const char* cook)
{
    static AdminModule a(cook);
    return &a;
}

//---------------------------------------------------------------------------------------
bool AdminModule::construct(CtxMod* pctx){return true;}
void AdminModule::destroy(){}

//---------------------------------------------------------------------------------------
int   AdminModule::work( CtxMod* pctx,CAN_RW rw)
{
    const char* p =
            "<html><meta http-equiv='refresh' content='2;url=http://localhost:8000/admin.admin'>"
            "<h1>ADMIN MODULE</h1><font face='Courier New'>";
    pctx->socket_write(HTTP_200, nHTTP_200);
    pctx->socket_write(p,strlen(p));

    std::ostringstream ss;// (ostringstream::in | ostringstream::out);
    assert(pctx);
    pctx->get_report(ss);
    ss << "</font></html>";
    std::string s = ss.str();
    pctx->socket_write(s.c_str(),s.length());
    return _DONE;
};

//---------------------------------------------------------------------------------------
bool  AdminModule::is_singleton()
{
    return true;
}

//---------------------------------------------------------------------------------------
 bool   AdminModule::uses_fds()const{return false;};

 //---------------------------------------------------------------------------------------
 int    AdminModule::fd_action (FD_OPS op, fd_set& rd, fd_set& wr){return 0;};
