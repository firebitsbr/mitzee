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


#include <iostream>
#include <sock.h>
#include <minidb.h>
#include <config.h>

#define ALLOWED 0
#define BLOCKED 1

extern bool __alive;

DbAccess*   __db;

static SADDR_46  fromstringip(const std::string& s, size_t& nport, std::string& domain, std::string& doc)
{
    // ggg.com:7777/asd.php
    // ggg.com/asd.php
    // ggg.com:7777/sdfsd/asd.php
    // ggg.com/sdfsd/asd.php

    string tmp = s;
    nport = 80;

    if(tmp.find("http://") == 0)
        tmp = s.substr(7);
    else if(tmp.find("https://") == 0)
        tmp = s.substr(8);

    size_t path   = tmp.find_first_of('/');
    size_t port   = tmp.find_first_of(':');
    if(port!=string::npos)
    {
        nport = ::atoi(tmp.substr(port+1, path).c_str());
        domain = tmp.substr(0,port);
    }
    else
    {
        domain = tmp.substr(0,path);
    }
    doc = tmp.substr( path+1);

    SADDR_46 rv = sock::dnsgetip(domain.c_str(), 0, nport);
    rv.set_port(nport);
    return rv;
}


//-----------------------------------------------------------------------------
DbAccess::DbAccess(time_t sessiontime,
                   int maxrecs,
                   const std::string& banned_ips,
                   const std::string& subscribers,
                   const std::string& admins,
                   const std::string& usercontrol,
                   const std::string& reloadacls,
                   const std::string& hostsfile,
                   int hfr):_reload(false),
    _reloading(false),
    _sessiontime(sessiontime),
    _maxrecs(maxrecs),
    _sbanned_ips(banned_ips),
    _ssubscribers(subscribers),
    _sadmins(admins),
    _susercontrol(usercontrol),
    _reloadacls(reloadacls),
    _shosts(hostsfile),
    _hostsfilerule(hfr)
{
    assert(__db==0);
    _now = time(0);
    __db=this;
}

//-----------------------------------------------------------------------------
// user has activity, keep it's session up
bool DbAccess::touch_usertime(const SADDR_46& kip, size_t tout)
{
    AutoLock   r(&_mqueue);

    StorItem<UI_T>* pt = _insession_ips.find_tac(kip);
    if(pt)
    {
        if(tout==0)tout=3;
        if(pt->t==-1) return true;  //  never expire
        if(_now - pt->t > (time_t)_sessiontime/4)
        {
            GLOGI("Increasing time out for:" << IP2STR(kip) );
            pt->t = _now;
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// local dns database to avoid dns calls
const SADDR_46& DbAccess::dnsgetip(const char* hname)
{
    static SADDR_46 dummy;

    AutoLock   r(&_mqueue);

    StorItem<SADDR_46>* pip = _dns.find_tac(hname);
    if(pip)
    {
        pip->t = _now;
        return pip->data;
    }

    SADDR_46 ip = sock::dnsgetip(hname);
    if(ip.empty())
    {
        return dummy;
    }
    StorItem<SADDR_46> s = {_now, ip};
    _dns.add_ta((const char*&)hname, s);
    pip = _dns.find_tac(hname);
    return pip->data;
}

//-----------------------------------------------------------------------------
// local dns database to avoid dns calls
const string& DbAccess::dnsgetname(const SADDR_46& ip46, bool addit)
{
    static  std::string     saticstring="*";
    AutoLock                r(&_mqueue);

    StorItem<string>* pip = _reverse_dns.find_tac(ip46);
    if(addit==false && pip==0){
        return saticstring;
    }

    if(pip)
    {
        pip->t = _now;
        GLOGI("cahced Host:'" << pip->data <<" = "<< ip46.c_str());
        return pip->data;
    }

    char name[196]={0};
    char* pname=name;
    if(sock::dnsgetnameinfo(ip46, name))
    {
        StorItem<string> s = {_now, name};
        _reverse_dns.add_ta(ip46, s);
    }
    else
    {
        StorItem<string> s = {_now, ip46.c_str()}; // no domain associated
        _reverse_dns.add_ta(ip46, s);
    }
    pip = _reverse_dns.find_tac(ip46);

    GLOGI("new    Host:" << pip->data <<" = "<< ip46.c_str());
    return pip->data;
}


//-----------------------------------------------------------------------------
//
void DbAccess::_src_timeouts(time_t sesstout)
{
    AutoLock   r(&_mqueue);
    _insession_ips.delete_oldies(time(0), sesstout, this);
}

//-----------------------------------------------------------------------------
//
void DbAccess::_dst_timeouts(time_t sesstout)
{
    //AutoLock   r(&_mqueue);
    //_dest_ips.delete_oldies(_now, sesstout);
}

//-----------------------------------------------------------------------------
//
void    DbAccess::_dns_timouts(time_t sesstout)
{
    AutoLock   r(&_mqueue);
    _dns.delete_oldies(_now, sesstout);
}


//-----------------------------------------------------------------------------
//
void    DbAccess::_reversedns_timouts(time_t sesstout)
{
    AutoLock   r(&_mqueue);
    _reverse_dns.delete_oldies(_now, sesstout);
}


//-----------------------------------------------------------------------------
//
void    DbAccess::_bounce_timouts(time_t sesstout)
{
    AutoLock   r(&_mqueue);
    _bouncing.delete_oldies(_now, sesstout);
}

//-----------------------------------------------------------------------------
//
void DbAccess::signal_to_stop()
{
    OsThread::signal_to_stop();
}

//-----------------------------------------------------------------------------
//true if found in hosts
bool    DbAccess::is_host_allowed(const char* pip)
{
    auto const & ipt = _dest_ips.find(pip);
    return (ipt != _dest_ips.end());
}

//-----------------------------------------------------------------------------
bool    DbAccess::check_hostaddr(const SADDR_46& ip)
{
    return !(DbAccess::dnsgetname(ip,false)=="*");
}


//-----------------------------------------------------------------------------
bool    DbAccess::is_host_allowed(const SADDR_46& ip)
{
    const string& dname =  DbAccess::dnsgetname(ip,false);
    auto const &  ipt = _dest_ips.find(dname);
    return ipt != _dest_ips.end();
}

//-----------------------------------------------------------------------------
//
bool DbAccess::is_client_allowed(const SADDR_46& ip)
{
    AutoLock   r(&_mqueue);

    if(_subscribers.size() && _subscribers.find(ip)!=_subscribers.end())
        return true;
    StorItem<UI_T>* ipt = _insession_ips.find_tac(ip);
    return (ipt != 0);
}

//-----------------------------------------------------------------------------
// +IP,-IP,
void DbAccess::instertto(const string& data, bool permanent)
{

    StorItem<UI_T>      c;
    SADDR_46            sin;
    std::istringstream  iss(data);
    std::string         token;
    AutoLock            r(&_mqueue);

    c.data = 1;
    if(permanent)
        c.t = -1;
    else
        c.t = time(0);

    while(getline(iss, token, ','))
    {
        if(token[0]=='~')
            break;
        if(token[1] != 0)
        {
            if(isdigit(token.at(1)))
                sin = sock::sip2ip(token.substr(1).c_str());

            GLOGI("DB  IP[ +/-: [" << token <<"]");

            switch(token[0])
            {
            case 'A':
                remove_bounce(sin, false);
                if(_insession_ips.add_ta(sin ,c, this))
                {
                    GLOGI("source ips added: " << token);
                    this->on_record_changed('A',sin);
                }
                break;
            case 'a':
                if(_insession_ips.remove_ta(sin, this))
                {
                    GLOGI("source ips removed: " << token);
                    this->on_record_changed('R',sin);
                }
                break;
            case 'B':
                remove_bounce(sin, false);
                _banned_ips.insert((const SADDR_46&)sin);
                GLOGI("banned ips added: " << token);
                break;
            case 'b':
                if(_banned_ips.find(sin) != _banned_ips.end())
                {
                    remove_bounce(sin, false);
                    _banned_ips.erase(sin);
                    GLOGI("banned ips removed: " << token);
                }
                break;
            case 'S':
                remove_bounce(sin, false);
                _subscribers.insert((const SADDR_46&)sin);
                GLOGI("subscribers ips added: " << token);
                break;
            case 's':
                if(_subscribers.find(sin) != _subscribers.end())
                {
                    _subscribers.erase(sin);
                    GLOGI("subscribers ips removed: " << token);
                }
            case 'H':
                {
                    if(isdigit(token[1]))
                    {
                        const string& dn =  dnsgetname(sin,true);
                        _dest_ips.insert(dn);
                    }
                    else
                        _dest_ips.insert(token.substr(1));//_shosts
                }
                break;
            case 'h':
                {
                    if(isdigit(token[1]))
                    {
                        const string& dn =  dnsgetname(sin, true);
                        _dest_ips.insert(dn);
                    }
                    else
                        _dest_ips.erase(token.substr(1));
                }
                break;
            default:
                break;
            }
        }
    }
}


//-----------------------------------------------------------------------------
//
void DbAccess::_load()
{
    GLOGI("loading database");
    _reloading=true;
    _now = time(0);

    _banned_ips.clear(); //sam as IPs, in a file passed in
    std::ifstream ifs(_sbanned_ips);
    if(ifs.is_open())
    {
        std::string line;
        while(!ifs.eof())
        {
            line.clear();
            std::getline (ifs, line);
            if(line.length()<4)continue;

            GLOGI("adding ban:" << line);
            _banned_ips.insert(SADDR_46(line.c_str(),0));
        }
        ifs.close();
        GLOGI("Loading:" << _sbanned_ips);
    }


    _subscribers.clear(); //sam as IPs, in a file passed in
    std::ifstream ifs2(_ssubscribers);
    if(ifs2.is_open())
    {
        std::string line;
        while(!ifs2.eof())
        {
            line.clear();
            std::getline (ifs2, line);

            if(line.length()>4)
            {
                SADDR_46 sb(line.c_str(),0);
                GLOGI("adding subscriber:" << sb.c_str());
                _subscribers.insert(sb);
            }
        }
        ifs2.close();
        GLOGI("Loading: " << _ssubscribers << ":" << _subscribers.size());
    }

    _insession_ips.clear();
    std::ifstream ifs3(_reloadacls);
    if(ifs3.is_open())
    {
        std::string line;
        while(!ifs3.eof())
        {
            line.clear();
            std::getline (ifs3, line);
            if(line.length()<4) continue;
            // 2014-08-28 05:48:50: A/a86.173.247.177:
            if(line.find(':')!=string::npos && line.find('A')!=string::npos)
            {
                size_t sep = line.find_last_of(' ');
                size_t esep = line.find_last_of(':');
                if(sep != string::npos && esep!=string::npos)
                {
                    string ip = line.substr(sep+1, esep);
                    GLOGI("adding in session:" << ip);
                    this->instertto(ip);
                }
                else
                {
                    GLOGE("invalid file format:" << _reloadacls);
                    break;
                }
            }
            else
            {
                size_t esep = line.find_last_of(':');
                string ip = "A"+line.substr(0, esep);
                this->instertto(ip);
                GLOGI("adding in session:" << ip);
            }
        }
        ifs3.close();
        GLOGI("Loading: " << _reloadacls << ":" << _insession_ips.count());
    }

    _admin_ips.clear();  // from config
    std::istringstream iss(_sadmins);
    std::string token;
    while(getline(iss, token, ','))
    {
        GLOGI("adding admin:" << token);
        _admin_ips.insert(SADDR_46(token.c_str()) );
    }
    GLOGI("Loaded:" << _sadmins);

    _dest_ips.clear(); //sam as IPs, in a file passed in
    std::ifstream ifs4(_shosts);
    if(ifs4.is_open())
    {
        std::string line;
        while(!ifs4.eof())
        {
            line.clear();
            std::getline (ifs4, line);
            if(line.length()<4)continue;

            if((line[0]>='a' && line[0]<='z') || (line[0]>='A' && line[0]<='Z') )
            {
                GLOGI("Adding remote host:" << line);
                _dest_ips.insert(line);
            }
            else
            {
                const string& dname =  DbAccess::dnsgetname(SADDR_46(line.c_str(),0),true);
                GLOGI("Removed remote host:" << dname);
                _dest_ips.insert(dname);
            }
        }
        ifs2.close();
        GLOGI("Loaded: " << _shosts );
    }
    _reloading=false;
}


void DbAccess::add_subscriber(const SADDR_46& ip)
{
    AutoLock __al(&_mqueue);
    _subscribers.insert(ip);
}

void DbAccess::add_session(const SADDR_46& ip)
{
    if(!touch_usertime(ip, 0))
    {
        AutoLock            __al(&_mqueue);
        StorItem<UI_T>      c;

        c.t = time(0);
        remove_bounce(ip, false);
        _insession_ips.add_ta(ip ,c, this);
    }
}

bool DbAccess::is_subscribed(const SADDR_46& ip)const
{
    AutoLock __al(&_mqueue);

    return _subscribers.find(ip)!=_subscribers.end();
}

bool DbAccess::is_admin(const SADDR_46& sad)const
{
    AutoLock __al(&_mqueue);
    return _admin_ips.find(sad)!=_admin_ips.end();
}

bool DbAccess::is_banned(const SADDR_46& sad)const
{
    AutoLock __al(&_mqueue);
    return _banned_ips.find(sad)!=_banned_ips.end();
}



void  DbAccess::on_record_changed(char plusminus, const SADDR_46& raddr)
{
    if(_reloading)
        return;
    if(!_susercontrol.empty())
    {

        std::string     doc;
        std::string     domain;
        size_t          port;

        SADDR_46        saddr = fromstringip(_susercontrol, port, domain, doc);
        tcp_cli_sock    cli;
        if(cli.raw_connect(saddr, port)==0)
        {
            if(cli.is_really_connected())
            {
                stringstream ost;

                ost << "GET /" << doc <<"?ip="<<plusminus<<raddr.c_str()<<" HTTP/1.1\r\n";
                ost << "Host: "<< domain <<"\r\n";
                ost << "Connection: close\r\n\r\n";
                cli.send((const u_int8_t*)ost.str().c_str(), ost.str().length());
                usleep(10000);
                GLOGI(ost.str().c_str());
            }
            else
            {
                GLOGE("Cannot connect to:"<< _susercontrol );
            }
            cli.destroy();
        }
        else
        {
            GLOGE("Cannot connect to:"<< _susercontrol );
        }
    }
}


void DbAccess::metrics(std::stringstream& str, SinOut&  bpss, const std::string& hname)const
{
    AutoLock __al(&_mqueue);
    time_t  since;

    if(_insession_ips._tadr.size())
    {
        //auto const &  b = _insession_ips._tadr.begin();
        for(auto const& b :  _insession_ips._tadr)
        {
            since =_now-b.second.t;
            str <<"<tr class='acls'><th colspan='3'>session</th><th>" <<
                IP2STR(b.first) <<
                "<a  href='http://"<<hname<<"/"<<"a"  << IP2STR(b.first) << "'>X</a>" <<
                "</th><td>" << (int)since << " secs ago</td></tr>\n";
        }
    }

    if(_bouncing._tadr.size())
    {
        for(auto const& b1 :  _bouncing._tadr)
        {
            str <<"<tr class='bounc'><th  colspan='3'>bouncing</th><th>" <<IP2STR(b1.first) <<
            "<a href='http://"<<hname<<"/"<<"b"  << IP2STR(b1.first) << "'>X</a>" <<
            "</th><td>";//
            if(b1.second.data.istore>=BOUNCE_BLOCKED)
                str <<"BLOCKED</td></tr>\n";
            else
                str <<(int)b1.second.data.istore << " times</td></tr>\n";
        }
    }


    if(_reverse_dns._tadr.size())
    {
        for( auto const &  b2 : _reverse_dns._tadr)
        {
            str <<"<tr class='revsn'><th  colspan='3'>reverse dns</th><td>" <<
                IP2STR(b2.first) << "</th><td>" <<
                b2.second.c_str() << "</td></tr>\n";
        }
    }

    if(_subscribers.size())
    {
        for(auto const &  b3 : _subscribers)
        {
            str <<"<tr class='subs'><th  colspan='4'>subscribers</th><td>"<< IP2STR(b3) <<
            "<a  href='http://"<<hname<<"/"<<"s"  << IP2STR(b3) << "'>X</a>" <<
            "</td></tr>\n";
        }
    }

    if(_banned_ips.size())
    {
        for(auto const &  b4 : _banned_ips)
        {
            str <<"<tr class='band'><th colspan='4'>banned</th><td>"<< IP2STR(b4) <<
            "<a  href='http://"<<hname<<"/"<<"b"  << IP2STR(b4) << "'>X</a>" <<
             "</td></tr>\n";
        }
    }

    if(_dest_ips.size())
    {
        if(_hostsfilerule=='1')
            str <<"<tr class='band'><th colspan='5'>hosts alow</th></tr>";
        else if(_hostsfilerule=='0')
            str <<"<tr class='band'><th colspan='5'>hosts deny</th></tr>";
        else
            str <<"<tr class='band'><th colspan='5'>hosts no rule</th></tr>";

        for(auto const &  b5 : _dest_ips)
        {
            str <<"<tr class='band'><th colspan='4'>host</th><td>"<< b5 <<
            "<a  href='http://"<<hname<<"/"<<"h"  << b5 << "'>X</a>" <<
            "</td></tr>\n";
        }
    }
}


void    DbAccess::remove_bounce(const SADDR_46& ip, bool lock)
{
    if(lock)
    {
        AutoLock __al(&_mqueue);

        if(_bouncing.find_tac(ip))
        {
            GLOGI("removing from bounce:" << IP2STR(ip));
            _bouncing.remove_ta(ip);
        }

    }
    else
    {
        if(_bouncing.find_tac(ip))
        {
            GLOGI("removing from bounce:" << IP2STR(ip));
            _bouncing.remove_ta(ip);
        }

    }
}

bool    DbAccess::has_bounced_max(const SADDR_46& sad)const
{
    AutoLock   r(&_mqueue);

    StorItem<UI_T>* ipt = ((TMap<SADDR_46, StorItem<UI_T> > &)_bouncing).find_tac(sad);
    if(ipt)
    {
        GLOGI(IP2STR(sad) << ": rejected due maxim bouncing ");
        return ipt->data.istore >= BOUNCE_BLOCKED;
    }
    return false;
}

/*
    -1 blocked
    0 not bouncing
    1 bouncing
*/
int    DbAccess::check_bounce(const SADDR_46& ip)
{
    if(is_client_allowed(ip))
    {
        return 0; //OK
    }

    AutoLock   r(&_mqueue);
    StorItem<UI_T>* ipt = _bouncing.find_tac(ip);
    if(ipt)
    {
        time_t ago = _now - ipt->t;

        if(ago > 60 /*seconds*/)
        {
            GLOGI("removing from bounce:" << IP2STR(ip));
            _bouncing.remove_ta(ip);
            return 0; // OK
        }
        // store time stamp now
        ipt->t=_now;
        if(ipt->data.istore < BOUNCE_BLOCKED)
        {
            ++ipt->data.istore;
            return 1; //redirect
        }
        return -1; // blocked, reached maximum
    }

    StorItem<UI_T>  newi = {_now, 1};
    _bouncing.add_ta(ip, newi);
    GLOGI("Bouncing client added to bounce: "<< IP2STR(ip));
    return 1;
}


//-----------------------------------------------------------------------------
//
void DbAccess::thread_main()
{
    tcp_cli_sock    acl_sock;
    int             k = 0;

    _load();
    while(!this->is_stopped() && __alive)
    {
        _now = time(0);

        if(_insession_ips.count() > _maxrecs)
            _src_timeouts(-1);
        //      if(_dest_ips.size()>_maxrecs)
        //          _dst_timeouts(-1);
        if( _dns.count() >  _maxrecs)
            _dns_timouts(-1);
        if(_reverse_dns.count()>_maxrecs)
            _reversedns_timouts(-1);
        if(_bouncing.count()>_maxrecs)
            _bounce_timouts(-1);

        if(k % 180 == 0)   // every 3 min
        {
            _src_timeouts(_sessiontime); //older than time
            _dst_timeouts(_sessiontime/4);
            _dns_timouts(_sessiontime/4);
            _reversedns_timouts(_sessiontime/4);
            _bounce_timouts(900); // 15 min
        }

        if(_reload)   //trigger a clean by creating a file
        {
            _reload=false;

            GLOGI("cleaning database");

            _src_timeouts(1); //older than time
            _dst_timeouts(1);
            _dns_timouts(1);
            _reversedns_timouts(1);
            _bounce_timouts(1);
            do
            {
                AutoLock   r(&_mqueue);

                _reverse_dns.clear();
                _insession_ips.clear();
                _dest_ips.clear();
                _bouncing.clear();
                _banned_ips.clear();
                _subscribers.clear();
            }
            while(0);
            _load();
        }
        sleep(1); //1 sec
        ++k;
    }
    __alive=false;
}

