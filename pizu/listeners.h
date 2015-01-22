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

#ifndef LISTENERS_H
#define LISTENERS_H

#include <string>
#include <vector>
#include "os.h"
#include "config.h"
#include <sock.h>

using namespace std;

class CtxQueue;
class ClientThread;
class ThreadPool;
class Listeners;

//-----------------------------------------------------------------------------
class SrvSock : public tcp_srv_sock
{
public:
    friend class Listeners;
    SrvSock(size_t port):_port(port),_cons(0),_acons(0),_bssl(false){}
    virtual ~SrvSock() {}
    const Conf::Vhost* _vHoost(kchar* name)const;
    void setSsl(bool b){ _bssl = true;};
    bool hasSsl()const{return _bssl;}
    size_t port()const{return _port;}
private:
    size_t                    _port;
    size_t                    _cons;
    size_t                    _acons;
    bool                      _bssl;
    map<string, Conf::Vhost*> _vhss;
};

//-----------------------------------------------------------------------------
class SslCrypt;
class Context;
class Listeners : public OsThread
{
public:

    Listeners(ThreadPool* pa); //_pssl
    virtual ~Listeners();

    mutex&  mut() {return  _m;}
    virtual int  start_thread();
    virtual void stop_thread();
    bool    san(){ return _sanity;}
    bool    initSsl();
    void    get_report(std::ostringstream& s);
private:
    void    _new_thread(Context* pc, int ignore);
    void    _put_inqueue(Context* pc, int every_sec);
    void    thread_main();
    void    _clear();
    bool    _allowed(Context* pc);
private:
    vector<SrvSock*>  _ss;
    CtxQueue*         _pqa;
    ThreadPool*       _pa;
    SslCrypt*         _pssl;
    bool              _sanity;
    int               _cons;
    mutex             _m;
};

//extern Context   __dynamic_ctx_exit;
#endif // LISTENERS_H
