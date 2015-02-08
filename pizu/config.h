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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <os.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <strutils.h>

using namespace std;
typedef int (*pfatoi) (kchar *);

typedef std::map<string, string> Mapss;
typedef std::map<string, string>::iterator MapssIt;

class Conf
{
    struct StrRule
    {
        char s[32];
        int   val;
    };

    void _assign( kchar* pred, kchar* val, int line);
    void _bind(kchar* lpred, kchar* vname,
               size_t& val, kchar* kv ){
       _bind(lpred, vname, (int&) val, kv );
    }
    void _bind(kchar* lpred, kchar* vname,
                   std::string& val, kchar* kv){

            if(vname){
                if(!str_cmp(lpred,vname)){
                    val = kv;
                    //throw(1);
                    return;
                }
                return;
            }
            val = kv;
            //throw(1);
        }

    void _bind(kchar* lpred, kchar* vname,
               int& val, kchar* kv, StrRule* r=0 ){
        if(!str_cmp(lpred,vname)){
            if(r){
                StrRule* pr = r;
                while(pr->s && *pr->s){
                    if(!str_cmp(pr->s, kv)){
                        val = pr->val;
                        //throw(1);
                    }
                    pr++;
                }
            }
            else{
                val = ::_ttatoi(kv);
                //throw(1);
            }

        }
    }
    void _bind(kchar* lpred, kchar* vname,
                   std::set<string>& val, kchar* kv){
            if(!str_cmp(lpred,vname)){
                std::istringstream iss(kv);
                std::string token;
                while(getline(iss, token, ',')) {
                    val.insert(token);
                }
                //throw(1);
            }
        }

public:
    Conf();
    ~Conf();
public:

    struct Listener
    {
        Listener():pending(0),order(0),dedicated(0){}
        int                pending;
        int                order; //0 1 0 alow deny 1 deny allow
        int                dedicated;
        std::string        cache;
        std::string        runfrom;
        std::string        plugflags;
        std::set<std::string>   allow;
        std::set<std::string>   deny;


    } _marius;

     struct Pool
    {
        Pool():min_threads(2),max_threads(4),
                    clients_perthread(32),
                    min_queue(32),max_queue(32),
                    context_pool(0),
                    database_pool(0),
                    time_out(32),
                    cache_singletons(1/*yes*/){}
        int min_threads;
        int max_threads;
        int clients_perthread;
        int min_queue;
        int max_queue;
        int context_pool;
        int database_pool;
        int time_out;
        int cache_singletons;

    } _pool;

    struct Vhost
    {
        Vhost():port(80),maxupload(2000),order(1),ssl(false),runmode(0),iptabled(0){

        }
        size_t        port;
        size_t        maxupload;
        int           order;
        int           ssl;
        int           runmode;
        int           iptabled;
        string        host;
        string        proxy;
        string        home;
        string        index;
        string        allow;
        set<string>   deny;
        set<string>   ls;

    }_lhost;

    struct Ssl
    {
        string ssl_lib;
        string crypto_lib;
        string certificate;
        string chain;

    }_ssl;

    void load(kchar*);

public:
    std::string                      _section;
    std::map<string, Conf::Vhost>    _hosts;
    Mapss         _extmodules;
};

typedef Conf::Vhost      CfgVhost;
typedef Conf::Vhost  CfgListen;
typedef Conf::Pool   CfgPool;

//-----------------------------------------------------------------------------------
template <typename T>inline void fix(T& val, T minv, T maxv)
{
    if(val < minv)
    {
        val = minv;
        return;
    }
    if(val > maxv)
    {
        val = maxv;
        return;
    }
}

extern Conf* GCFG;
enum{
    eLIST_OK,
    eLIST_FAIL,
    eREC_OK,
    eREC_FAIL,
    eSND_OK,
    eSND_FAIL,
    eREC_KBYTES,
    eSND_KBYTES,
};


#endif //_CONFIG_H_
