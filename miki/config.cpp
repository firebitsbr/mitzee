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

#include <assert.h>
#include <iostream>
#include <stdarg.h>
#include <limits.h>
#include <sock.h>
#include "config.h"
#include <strutils.h>
#include <stdlib.h>

Conf* PCFG;

//-----------------------------------------------------------------------------
Conf::Conf(const char* file):_blog(0)
{
    PCFG=this;
    if(file)
    {
        load(file);
    }
}


//-----------------------------------------------------------------------------
Conf::~Conf()
{
    PCFG=0;
    PCFG=0;
//    system(_srv.postcomand.c_str()); //make sure this is in visudo
}

//-----------------------------------------------------------------------------
void Conf::load(const char* cfgFile)
{
    FILE* pf = _ttfopen(cfgFile, "rb");
    if(pf)
    {
        AutoCall<int (*)(FILE*), FILE*>    _a(::fclose, pf);
        char        pred[128];
        char        val[128];
        char        line[256];
        bool        inComment = false;
        try
        {
            while(!feof(pf))
            {
                if(fgets(line,255,pf))
                {
                    str_prepline(line);
                    if(*line==0)
                        continue;

                    if(inComment || *line=='#')
                        continue;

                    const char* pnext = str_ccpy(pred, line,'=');

                    if(*pred=='[')
                    {
                        str_lrtim(pred);
                        _section = pred;
                        continue;

                    }
                    else   if(*pred=='}')
                    {
                        _assign("}", " ", 0);
                    }
                    if(pnext && *pnext)
                    {
                        str_scpy(val, (char*)pnext+1, "#");

                        str_lrtim(val);
                        str_lrtim(pred);
                        _assign(pred, val, 0);
                    }

                }
                else
                    break;
                if(feof(pf))
                {
                    break;
                }
            }
        }
        catch(int& err)
        {
        }
    }
    else
    {
        printf( "Cannot find configuration file in curent folder\r\n");
        exit(0);
    }

    // system(_srv.precomand.c_str());

}
#define BIND(mem_,name_)     _bind(lpred, #name_,  mem_.name_, val);


static SADDR_46  fromstringip(const std::string& s)
{

    string tmp = s;
    if(tmp.find("http://") == 0)
        tmp = s.substr(7);

    int    nport = 80;
    size_t port = tmp.find(':');
    size_t doc = tmp.find('/');
    if(port!=string::npos)
    {
        nport = ::atoi(tmp.substr(port+1, doc).c_str());
    }
    else if(doc!=string::npos)
    {
        port = doc;
    }

    char phost[256];

    ::strcpy(phost, tmp.substr(0,port).c_str());
    if(isdigit(phost[0]))
        return SADDR_46(phost,nport);
    char test[32];
    SADDR_46 r = sock::dnsgetip(phost, test, nport);
    std::cout<<phost <<"="<<r.c_str() << " / " << test << "\r\n";
    return r;
}


//-----------------------------------------------------------------------------
// this is a quick hack config loading. TO ??? chnage it in the future
void Conf::_assign( const char* pred, const char* val, int line)
{
    char    lpred[256];
    char    loco[256];

    ::strcpy(loco,val);
    ::strcpy(lpred,pred);

    try
    {
        if(_section == "[server]" )
        {
            BIND(_srv,order);  //1 allow-deny  0 deny-allow
            BIND(_srv,port);
            BIND(_srv,threads);
            BIND(_srv,addr);
            BIND(_srv,runfrom);
            BIND(_srv,postcomand);
            BIND(_srv,precomand);
            BIND(_srv,slog);
            BIND(_srv,proxyip);
            BIND(_srv,cache);
            BIND(_srv,nextdnss);
            BIND(_srv,notify);
            BIND(_srv,localip);
            BIND(_srv,onresponse);

            if(lpred[0]=='}')
            {
                if(_srv.proxyip.find("$LOCAL")==0)
                {
                    _srv._prx_addr=sock::GetLocalIP();
                    size_t pcol = _srv.proxyip.find(":");
                    if(pcol!=string::npos)
                    {
                        const int port =  ::atoi(_srv.proxyip.substr(pcol+1).c_str());
                        _srv._prx_addr.set_port(port);
                    }
                }
                else if(!_srv.proxyip.empty())
                {
                    _srv._prx_addr=fromstringip(_srv.proxyip.c_str());
                }
                if(_srv.precomand.empty())
                    _srv.precomand  = "sudo iptables -t nat -A PREROUTING -p udp --dport 53 -j REDIRECT --to-port 8053";
                if(_srv.postcomand.empty())
                    _srv.postcomand = "sudo iptables -t nat -D PREROUTING 1";
                if( _srv.runfrom.empty())
                {
                    char cwd[512];
                    getcwd(cwd,511);
                    _srv.runfrom = cwd ;
                    _srv.runfrom+="/";
                }
                _blog |= _srv.slog.find('I') == string::npos ? 0 : 0x1;
                _blog |= _srv.slog.find('W') == string::npos ? 0 : 0x2;
                _blog |= _srv.slog.find('E') == string::npos ? 0 : 0x4;
                _blog |= _srv.slog.find('T') == string::npos ? 0 : 0x8;
                _blog |= _srv.slog.find('D') == string::npos ? 0 : 0x10;
                _blog |= _srv.slog.find('X') == string::npos ? 0 : 0x20;

                if(!_srv.cache.empty())
                {
                    std::string cachefld;
                    if(_srv.cache.at(0)!='/')
                    {
                        char cd[400];
                        getcwd(cd, sizeof(cd-sizeof(char)));
                        cachefld=cd;
                        cachefld+="/";
                        cachefld+=_srv.cache;
                    }
                    else
                    {
                        cachefld=_srv.cache;
                    }
                    if(::access(cachefld.c_str(),0)!=0)
                    {
                       ::mkdir(cachefld.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                    }
                    if(::access(cachefld.c_str(),0)!=0)
                    {
                        GLOGE("Cannot create: '" << cachefld << "' directory. Do you have access rights?");
                        _srv.cache.clear();
                    }
                }

            }
        }
        if(_section == "[rules]" )
        {
            if(lpred[0]=='}')
            {
                std::map<std::string,SADDR_46>::const_iterator b = rules.begin();
                for(; b!= rules.end(); ++b)
                {
                    GLOGI("rule: " << b->first << "=" << b->second.c_str());
                }
            }
            else
            {
                rules[lpred] = fromstringip(loco);
            }

        }
    }
    catch(int done) {}
}


void    Conf::check_log_size()
{
    if(_logcntn.tellp() > 0)
    {
        char    logf[400];

        sprintf(logf, "logs/mariuz.log0");
        FILE* pf = fopen(logf,"ab");
        long int flen=0;
        if(pf)
        {
            fwrite(_logcntn.str().c_str(), 1, _logcntn.str().length(), pf);
            flen = ftell(pf);
            fclose(pf);
        }
        //_logcntn="";
        _logcntn.str("");
        _logcntn.clear();
        _logcntn.seekp(0);
        _logcntn.seekg(0);
        if(flen > MAX_ROLLUP)
        {
            PCFG->rollup_logs("mariuy");
        }
    }
}


void Conf::rollup_logs(const char* rootName)
{
    char oldFn[256];
    char newFn[256];

    int iold = 40;
    sprintf(oldFn,"%s.log20",rootName);
    unlink(oldFn);
    while(iold-- > 0)
    {
        sprintf(newFn, "%s.log%d",rootName ,iold);
        sprintf(oldFn, "%s.log%d",rootName, iold-1);
        rename(oldFn, newFn);
    }
}
