/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - coinscode.com - N/A
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


#ifndef _DB_MINI_
#define _DB_MINI_

#include <os.h>
#include <sock.h>
#include <strutils.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <matypes.h>
#include <map>
#include <vector>
#include <config.h>
#include <minidb.h>

//-----------------------------------------------------------------------------
class DbAccess;
extern DbAccess* __db;
typedef enum _TC_STATE{SCONN, SRECV, SCLOSED}TC_STATE;


#define BOUNCE_MAX  32
#define BOUNCE_BLOCKED 100
//----------------------------------------------------------------------------
template <class T>class Notifier
{
public:
    virtual void on_record_changed(char plusminus, const T& raddr)
    {
        GLOGI("virtual pure called");
    };
};


//----------------------------------------------------------------------------
struct UI_T
{
    UI_T():istore(0){::sprintf(sstore,"%d",istore);}
    UI_T(const UI_T& r):istore(r.istore){::sprintf(sstore,"%d",istore);}
    UI_T(size_t i):istore(i){::sprintf(sstore,"%d",istore);}
    const char* c_str()const{
        return sstore;
    }
    operator const char*()const{return sstore;}
    UI_T& operator=(const int r){
        istore = r;
        ::sprintf(sstore,"%d",istore);
        return *this;
    }
    UI_T& operator=(const UI_T& r){
        istore = r.istore;
        ::strcpy(sstore,r.sstore);
        return *this;
    }
    bool operator>=(const UI_T& r){return istore>r.istore;}
    bool operator>=(const int& r){return istore>r;}
    UI_T& operator++(){istore++; return *this;} //  prefix
    UI_T operator++(int){                      // postfix
        UI_T r(*this);
        ++istore;
        return r;
    }
    int  istore;
    char sstore[16];
};

//----------------------------------------------------------------------------
// adds time to the type we store
template <typename T>
struct StorItem
{
    time_t   t;
    T        data;
    StorItem& operator>>(ofstream& o)
    {
        o<<data;//>>o;
        return *this;
    }
    StorItem& operator<<(ifstream& o)
    {
        o>>data;
        return *this;
    }
    const char* c_str()const{return data.c_str();}
};

//-----------------------------------------------------------------------------
template <typename U, typename T>
struct TMap
{
    TMap(){clear();};
    ~TMap() {}
    size_t count()const{return _tadr.size();}
    void  clear(){_tadr.clear();}
    void  delete_oldies(time_t now, time_t sessiontime, Notifier<U>* pn=0)
    {
        typename std::map<U,T>::iterator b;
        bool deteted =false;
        if(sessiontime>0)
        {
            for(b = _tadr.begin(); b!= _tadr.end();)
            {
                time_t& ru = (*b).second.t;
                if(now - ru > sessiontime)
                {
                    deteted = true;
                    GLOGI("DB deleting expired IP: [" << (*b).first.c_str() << "]=" << (*b).second.c_str());
                    if(pn)
                    {
                        pn->on_record_changed('R', (*b).first);
                    }
                    _tadr.erase(b++);
                    continue;
                }
                ++b;
            }
        }
        else //delete older one
        {
            time_t                           toldest = 0;
            typename std::map<U,T>::iterator ioldest;

            for(b = _tadr.begin(); b!= _tadr.end();)
            {
                time_t& ru = (*b).second.t;
                if(now - ru > toldest)
                {
                    toldest = now - ru;
                    ioldest = b;
                }
                ++b;
            }
            if(toldest)
            {
                 GLOGI("DB deleting older IP: [" << (*ioldest).first.c_str()<<"]="<< (*b).second.c_str());
                if(pn)
                {
                    pn->on_record_changed('R', (*ioldest).first);
                }
                _tadr.erase(ioldest);
                deteted=true;
            }
        }
        if(deteted)
        {
            GLOGI("DB records updated. Items left: " << count());
        }
    };


    T*  find_tac(const U& k)
    {
        typename std::map<U, T>::iterator b = _tadr.find(k);
        if(b!=_tadr.end())
        {
            return &b->second;
        }
        return 0; //not found
    }



    bool  add_ta(const U& k, const T& v, Notifier<U>* pn=0)
    {
        _tadr[k] = v;
        return true;
    }

    bool  remove_ta(const U& k, Notifier<U>* pn=0)
    {
        typename std::map<U, T>::iterator b = _tadr.find(k);
        if(b == _tadr.end())
        {
            return false;
        }
        if(pn)
        {
            pn->on_record_changed('R', (*b).first);
        }
        _tadr.erase(k);
        return true;
    }


    std::map<U, T>   _tadr;

};

struct SinOut;
//-----------------------------------------------------------------------------
struct SigningIP
{
    SADDR_46    addr;
    time_t      born;
    int         bounce;
    bool operator=(const SigningIP& r)const{return r.addr==addr;}
    bool operator<(const SigningIP& r)const{return r.addr<addr;}
};

//-----------------------------------------------------------------------------
class DbAccess : public OsThread, public Notifier<SADDR_46>
{
public:
    DbAccess(time_t sessiontime,
                   int maxrecs,
                   const std::string& banned_ips,
                   const std::string& subscribers,
                   const std::string& admins,
                   const std::string& usercontrol,
                   const std::string& reloadacls,
                   const std::string& shosts,
                   int hrule);

    ~DbAccess() {__db=0;};

    void        signal_to_stop();

    bool        is_client_allowed(const SADDR_46& ip);
    bool        is_host_allowed(const char* pip);
    bool        is_host_allowed(const SADDR_46& ip);
    bool        check_hostaddr(const SADDR_46& ip);
    const       SADDR_46&   dnsgetip(const char* hname);
    const       string& dnsgetname(const SADDR_46& ip46, bool addit);

    bool        is_admin(const SADDR_46& ip)const;  // allowed allways
    bool        is_subscribed(const SADDR_46& ip)const;  // allowed allways

    void        add_subscriber(const SADDR_46& ip);
    void        add_session(const SADDR_46& ip);
    void        instertto(const string& data, bool permanent=false);
    bool        touch_usertime(const SADDR_46& ip, size_t tout);
    bool        is_banned(const SADDR_46& ip)const; // rejected at listen time
    bool        has_bounced_max(const SADDR_46& sad)const;
    void        on_record_changed(char plusminus, const SADDR_46& raddr);
    void        metrics(std::stringstream& str, SinOut&  bpss, const std::string& hname)const;
    void        reload(){_reload=true;}
    int         check_bounce(const SADDR_46& ip);
    void        remove_bounce(const SADDR_46& ip, bool lock=false);
protected:
    virtual void thread_main();
private:
    void        _load();
    void        _dns_timouts(time_t sesstout);
    void        _reversedns_timouts(time_t sesstout);
    void        _src_timeouts(time_t sesstout);
    void        _dst_timeouts(time_t sesstout);
    void        _bounce_timouts(time_t sesstout);
    void        _bbanned_timouts(time_t sesstout);

private:
    mutex                                    _mqueue;

    bool                                     _reload;
    bool                                     _reloading;

    time_t                                   _sessiontime;
    size_t                                   _maxrecs;
    time_t                                   _now;

    const string&                            _sbanned_ips;
    const string&                            _ssubscribers;
    const string&                            _sadmins;
    const string&                            _susercontrol;
    const string&                            _reloadacls;
    const string&                            _shosts;
    int                                      _hostsfilerule;

    TMap<string,   StorItem<SADDR_46> >      _dns;           // dnsses
    TMap<SADDR_46, StorItem<string> >        _reverse_dns;   // dest ips
    TMap<SADDR_46, StorItem<UI_T> >          _insession_ips;    // source ips
    TMap<SADDR_46, StorItem<UI_T> >          _bouncing;      // using php with invalid password
    TMap<SADDR_46, StorItem<UI_T> >          _bbanned_ips;
    set<string>                              _dest_ips;      // dest ips
    set<SADDR_46>                            _banned_ips;
    set<SADDR_46>                            _admin_ips;
    set<SADDR_46>                            _subscribers;
//    SADDR_46                                 _cachedip;
//    bool                                     _cachedretval;
//    SADDR_46                                 _cachedipH;
//    bool                                     _cachedretvalH;
};


#endif // IPRULES_H
