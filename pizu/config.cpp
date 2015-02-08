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

#include <assert.h>
#include <stdarg.h>
#include <limits.h>
#include <iostream>
#include "config.h"
#include <strutils.h>

Conf* GCFG;

Conf::Conf()
{

}

Conf::~Conf()
{
}

void Conf::load(kchar* cfgFile)
{
    FILE* pf = _ttfopen(cfgFile, "rb");
    if(pf) {
        AutoCall<int (*)(FILE*), FILE*>    _a(::fclose, pf);
        char        pred[128];
        char        val[128];
        char        line[256];
        bool        inComment = false;
        try {
            while(!feof(pf)) {

                if(fgets(line,255,pf)) {
                    str_prepline(line);
                    if(*line==0)
                        continue;

                    if(inComment || *line=='#')
                        continue;

                    kchar* pnext = str_ccpy(pred, line,'=');

                    if(*pred=='[') {
                        str_lrtim(pred);
                        _section = pred;
                        continue;

                    } else   if(*pred=='}') {
                        _assign("}", " ", 0);
                    }
                    if(pnext && *pnext) {
                        str_scpy(val, (char*)pnext+1, "#");

                        str_lrtim(val);
                        str_lrtim(pred);
                        _assign(pred, val, 0);
                    }

                } else
                    break;
                if(feof(pf)) {
                    break;
                }
            }
        } catch(int& err) {
        }
    } else {
        printf( "Cannot find configuration file in curent folder\r\n");
        exit(0);
    }
}
//-----------------------------------------------------------------------------
// this is a quick hack config loading. TO ??? chnage it in the future
void Conf::_assign( kchar* pred, kchar* val, int line)
{
    char    lpred[1024];
    char    loco[1024];
    StrRule s[] = {{"allow-deny",1},{"deny-allow",0},{"",0}};

    strcpy(loco,val);
    strcpy(lpred,pred);
    str_trimall(lpred,' ');
    str_trimall(lpred,'\t');

    try {
        if(_section == "[marius]") {
            _bind(lpred, "cache", _marius.cache, val);
            _bind(lpred, "runfrom", _marius.runfrom, val);
            _bind(lpred, "dedicated", _marius.dedicated, val);
            _bind(lpred, "plugflags", _marius.plugflags, val);
            _bind(lpred, "pending", _marius.pending, val);
            _bind(lpred, "order",   _marius.order, val, s);
            _bind(lpred, "allow",   _marius.allow, val);
            _bind(lpred, "deny",    _marius.deny, val);

            if(lpred[0]=='}') {
                //
                // make some subfolders under cache
                //
                char subdirs[PATH_MAX];
                if(_marius.runfrom.empty()) {
                    char scwd[512];

                    getcwd(scwd,511);
                    strcat(scwd,"/");
                    _marius.runfrom = scwd;
                }
                if(0!=chdir(_marius.runfrom.c_str())) {
                    std::cout << "cannot change folder to: " << _marius.runfrom << "\n";
                    exit(-1);
                } else {
                    //test write
                    system("touch dummy");
                    struct stat s;
                    if(0!=stat("dummy",&s)) {
                        std::cout << "cannot write to folder: " << _marius.runfrom << "\n";
                        exit(-2);
                    }
                    unlink("dummy");
                }

                if(_marius.cache.empty()){
                    _marius.cache = "cache/";
                }
                if(_marius.cache[0]!='/') {
                    string s = _marius.cache;
                    _marius.cache = _marius.runfrom + s;
                }
                if(_marius.cache==_marius.runfrom){
                    std::cout << "invalid cache foder\n";
                    exit(0);
                }
                sprintf(subdirs,"rm -r %s", _marius.cache.c_str());
                system(subdirs);

                mkdir(_marius.cache.c_str(),0755);

                sprintf(subdirs,"%shss/", _marius.cache.c_str());
                mkdir(subdirs,0755);
                sprintf(subdirs,"%ssbin/", _marius.cache.c_str());
                mkdir(subdirs,0755);
                sprintf(subdirs,"%stmp/", _marius.cache.c_str());
                mkdir(subdirs,0755);
                sprintf(subdirs,"%scerts/", _marius.runfrom.c_str());
                mkdir(subdirs,0755);
                sprintf(subdirs,"%srules/", _marius.runfrom.c_str());
                mkdir(subdirs,0755);

            }
        }

        if(_section == "[modules]") {

            if(*loco && *lpred) {
                _extmodules[lpred] = loco;
            }
        }

        if(_section == "[pool]") {
            _bind(lpred, "min_threads",_pool.min_threads, val);
            _bind(lpred, "max_threads",_pool.max_threads, val);
            _bind(lpred, "clients_perthread",_pool.clients_perthread, val);
            _bind(lpred, "min_queue",_pool.min_queue, val);
            _bind(lpred, "max_queue",_pool.max_queue, val);
            _bind(lpred, "time_out",_pool.time_out, val);
            _bind(lpred, "cache_singletons",_pool.cache_singletons, val);
        }

        if(_section == "[host]") {

            _bind(lpred, "host", _lhost.host, val);
            _bind(lpred, "proxy", _lhost.proxy, val);
            _bind(lpred, "home", _lhost.home, val);
            _bind(lpred, "order", _lhost.order, val, s);
            _bind(lpred, "allow", _lhost.allow, val);
            _bind(lpred, "deny", _lhost.deny, val);
            _bind(lpred, "ls", _lhost.ls, val);
            _bind(lpred, "index", _lhost.index, val);
            _bind(lpred, "ssl", _lhost.ssl, val);
            _bind(lpred, "maxupload", _lhost.maxupload, val);

            if(lpred[0]=='}') {


                if(_lhost.home[0]!='/') {
                    _lhost.home = _marius.runfrom+_lhost.home;
                }
                std::set<string>::iterator it = _lhost.ls.begin();

                for(; it != _lhost.ls.end(); ++it) {
                    if((*it)[0] != '/') {
                        std::string ts(*it);
                        (*it).assign(_marius.runfrom + ts);
                    }
                }

                if(_lhost.home[_lhost.home.length()-1] == '/')
                    _lhost.home = _lhost.home.substr(0,_lhost.home.length()-1);

                string hostname = _lhost.host;
                size_t pos = hostname.find(':');
                _lhost.host = hostname.substr(0,pos);
                string ports = hostname.substr(pos+1);
                hostname = hostname.substr(0,pos);

                size_t redi = ports.find(',');
                if(redi != string::npos)//we have redirected port to port
                {
                    _lhost.port     = ::_ttatoi(ports.substr(0,redi).c_str());
                    _lhost.iptabled = ::_ttatoi(ports.substr(redi+1).c_str());
                }
                else
                {
                    _lhost.port = ::_ttatoi(ports.c_str());
                }

                _hosts[hostname] = _lhost;
            }
        }

        if(_section == "[ssl]") {
            _bind(lpred, "ssl_lib", _ssl.ssl_lib, val);
            _bind(lpred, "crypto_lib", _ssl.crypto_lib, val);
            _bind(lpred, "certificate", _ssl.certificate, val);
            _bind(lpred, "chain", _ssl.chain, val);
            if(lpred[0]=='}') {
                if(!_ssl.certificate.empty() && _ssl.certificate[0]!='/') {
                    string cert = _ssl.certificate;
                    _ssl.certificate = _marius.runfrom + cert;
                }
                if(!_ssl.chain.empty() && _ssl.chain[0]!='/') {
                    string chain = _ssl.chain;
                    _ssl.chain = _marius.runfrom + chain;
                }
                /*
                if(_ssl.certificate.empty() || _ssl.certificate=="auto")
                {
                   //generate a autosigned cert
                   char destcert[512];
                   sprintf(destcert,"%sserts/mycert.pem", _marius.cache.c_str());
                   char cmd[1024];
                   sprintf(cmd,"openssl req -x509 -nodes -days 365 -newkey rsa:1024 "
                               "-keyout %s -out %s",destcert,destcert);
                   system(cmd);
                }
                */

            }
        }

    } catch(int done) {

    }
}

