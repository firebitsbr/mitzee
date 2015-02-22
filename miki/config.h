/**
# Copyright (C) 2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - coinscode.com - N/A
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
#include "os.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <strutils.h>


//-----------------------------------------------------------------------------
using namespace std;
typedef int (*pfatoi) (const char *);

//-----------------------------------------------------------------------------
typedef std::map<string, string> Mapss;
typedef std::map<string, string>::iterator MapssIt;

#define MAX_ROLLUP 10000000 //4Mb
//-----------------------------------------------------------------------------
class Conf
{
    struct StrRule
    {
        char s[32];
        int   val;
    };

    void _assign( const char* pred, const char* val, int line);
    void _bind(const char* lpred, const char* vname,
               size_t& val, const char* kv )
    {
        _bind(lpred, vname, (int&) val, kv );
    }
    void _bind(const char* lpred, const char* vname,
               std::string& val, const char* kv)
    {
        if(vname)
        {
            if(!strcmp(lpred,vname))
            {
                val = kv;
                //throw(1);
                return;
            }
            return;
        }
        val = kv;
        //throw(1);
    }

 void _bind(const char* lpred, const char* vname,
               int& val, const char* kv, StrRule* r=0 )
    {
        if(!strcmp(lpred,vname))
        {
            if(r)
            {
                StrRule* pr = r;
                while(pr->s && *pr->s)
                {
                    if(!strcmp(pr->s, kv))
                    {
                        val = pr->val;
                        //throw(1);
                    }
                    pr++;
                }
            }
            else
            {
                if(kv[0]==':')
                {
                    int a,b,c,d;
                    sscanf(&kv[1],"%d.%d.%d.%d",&a,&b,&c,&d);
                    a&=0xff;
                    b&=0xff;
                    c&=0xff;
                    d&=0xff;
                    val = (d<<24|c<<16|b<<8|a);
                }
                else
                {
                    val = ::_ttatoi(kv);
                }
                //throw(1);
            }

        }
    }
    void _bind(const char* lpred, const char* vname,
               std::set<string>& val, const char* kv)
    {
        if(!strcmp(lpred,vname))
        {
            std::istringstream iss(kv);
            std::string token;
            while(getline(iss, token, ','))
            {
                val.insert(token);
            }
            //throw(1);
        }
    }
    void _bind(const char* lpred, const char* vname,
               std::vector<string>& val, const char* kv)
    {
        if(!strcmp(lpred,vname))
        {
            std::istringstream iss(kv);
            std::string token;
            while(getline(iss, token, ','))
            {
                val.push_back(token);
            }
            //throw(1);
        }
    }

public:
    Conf(const char* file=0);
    ~Conf();
    void    check_log_size();
    void    rollup_logs(const char* rootName);
public:

    struct Srv
    {
        Srv():order(1),port(8888),threads(1),timeout(16),notify(1),onresponse(0) {};
        int             order;  //1 allow-deny  0 deny-allow
        int             port;
        int             threads;
        int             timeout;
        std::string     addr;
        std::vector<string> nextdnss;
        std::string     postcomand;
        std::string     precomand;
        std::string     runfrom;
        std::string     runas;
        std::string     proxyip;
        std::string     slog;
        std::string     cache;
        std::string     localip;
        SADDR_46        _prx_addr;
        int             notify;  //s or c
        int				onresponse;
    } _srv;

    std::map<std::string,SADDR_46> rules;
    void load(const char*);

public:
    std::string       _section;
    size_t            _blog;
    std::stringstream _logcntn;
    mutex             _m;
};

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
extern Conf* PCFG;

#if 1
//-----------------------------------------------------------------------------
#define GLOGI(x) if(PCFG->_blog & 0x1) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" I: " << x << "\n";\
    std::cout << str_time() <<" I: " << x << "\n";\
}while(0);

//-----------------------------------------------------------------------------
#define GLOGW(x) if(PCFG->_blog & 0x2) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" W: " << x << "\n";\
    std::cout << str_time() <<" W: " << x << "\n";\
}while(0);

#define GLOGE(x) if(PCFG->_blog & 0x4) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" E: " << x << "\n";\
    std::cout << str_time() <<" E: " << x << "\n";\
}while(0);

//-----------------------------------------------------------------------------
#define GLOGT(x) if(PCFG->_blog & 0x8) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" T: " << x << "\n";\
    std::cout << str_time() <<" T: " << x << "\n";\
}while(0);

//-----------------------------------------------------------------------------
#define GLOGD(x) if(PCFG->_blog & 0x10) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" D: " << x << "\n";\
    std::cout << str_time() <<" D: " << x << "\n";\
}while(0);

//-----------------------------------------------------------------------------
#define GLOGX(x) if(PCFG->_blog & 0x20) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" X: " << x << "\n";\
    std::cout << str_time() <<" X: " << x << "\n";\
}while(0);

#else

//-----------------------------------------------------------------------------
#define GLOGI(x) if(PCFG->_blog & 0x1) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" I: " << x << "\n";\
}while(0);


//-----------------------------------------------------------------------------
#define GLOGW(x) if(PCFG->_blog & 0x2) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" W: " << x << "\n";\
}while(0);

//-----------------------------------------------------------------------------
#define GLOGE(x) if(PCFG->_blog & 0x4) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" E: " << x << "\n";\
}while(0);

//-----------------------------------------------------------------------------
#define GLOGT(x) if(PCFG->_blog & 0x8) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" T: " << x << "\n";\
}while(0);

//-----------------------------------------------------------------------------
#define GLOGD(x) if(PCFG->_blog & 0x10) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" D: " << x << "\n";\
}while(0);

//-----------------------------------------------------------------------------
#define GLOGX(x) if(PCFG->_blog & 0x20) \
do{\
    AutoLock __a(&PCFG->_m); \
    PCFG->_logcntn << str_time() <<" X: " << x << "\n";\
}while(0);
#endif


#endif //_CONFIG_H_


