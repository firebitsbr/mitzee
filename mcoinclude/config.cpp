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


#include <assert.h>
#include <iostream>
#include <stdarg.h>
#include <limits.h>
#include <config.h>
#include <strutils.h>

//-----------------------------------------------------------------------------
 Conf* PCFG;
//-----------------------------------------------------------------------------
Conf::Conf()
{
    assert(0 == PCFG);
    PCFG=this;
    _logs_path = "logs";
}

//-----------------------------------------------------------------------------
Conf::~Conf()
{
    PCFG=0;
}

//-----------------------------------------------------------------------------
bool Conf::finalize(){

    std::string r = "mkdir -p "; r += _logs_path;
    ::system(r.c_str());

    std::string p = _logs_path + "/bytes";
    ::mkdir(p.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::string q = _logs_path + "/logs";
    ::mkdir(q.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    return true;
};

//-----------------------------------------------------------------------------
bool Conf::load(const char* cfgFile)
{
    FILE* pf;
    char locations[512];

    ::sprintf(locations,"/etc/buflea/%s",cfgFile);
    pf = _ttfopen(locations, "rb"); if(pf) goto done;
    ::sprintf(locations,"/etc/%s",cfgFile);
    pf = _ttfopen(locations, "rb"); if(pf) goto done;
    ::sprintf(locations,"/home/%s/.buflea/%s",getenv("USER"),cfgFile);
    pf = _ttfopen(locations, "rb"); if(pf) goto done;
    pf = _ttfopen(cfgFile, "rb");

done:
    if(pf)
    {
        AutoCall<int (*)(FILE*), FILE*>    _a(::fclose, pf);
        char        pred[128];
        char        val[128];
        char        line[512];
        bool        in_comment = false;
        try
        {
            while(!::feof(pf))
            {
                if(::fgets(line,512,pf))
                {
                    ::str_prepline(line);
                    if(*line==0)
                        continue;
                    if(in_comment || *line=='#')
                        continue;
                    const char* pnext = ::str_ccpy(pred, line,'=');
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
                        ::str_scpy(val, (char*)pnext+1, "#");
                        ::str_lrtim(val);
                        ::str_lrtim(pred);

                        //cheak the logs folder
                         if(pred[0]=='l')
                            _bind(pred, "logs_path", _logs_path, val);

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
            ;//noting
        }
    }
    else
    {
        printf( "Cannot find configuration file in any of /etc/buflea, /etc/ ~/.buflea and  ./ \r\n");
        exit(0);
    }
    return finalize();
}



//-----------------------------------------------------------------------------
#define BIND(mem_,name_)     _bind(lpred, #name_,  mem_.name_, val);

//-----------------------------------------------------------------------------
// log files flush and roll-up
void    Conf::check_log_size()
{
    if(_logcntn.tellp() > 0)/* log this right away*/
    {
        AutoLock __a(&_m);

        char    logf[400];

        ::sprintf(logf, "%s/buflea.log0", _logs_path.c_str());
        FILE* pf = ::fopen(logf,"ab");
        long int flen=0;

        if(pf)
        {
            ::fwrite(_logcntn.str().c_str(), 1, _logcntn.str().length(), pf);
            flen = ftell(pf);
            ::fclose(pf);
        }

        _logcntn.str("");
        _logcntn.clear();
        _logcntn.seekp(0);
        _logcntn.seekg(0);

        if(flen > MAX_ROLLUP)
        {
            PCFG->rollup_logs("buflea");
        }
    }
}

//-----------------------------------------------------------------------------
// rolls up the logs, when reaches the max size
void Conf::rollup_logs(const char* rootName)
{
    char oldFn[256];
    char newFn[256];

    int iold = 3;
    ::sprintf(oldFn,"%s/%s.log4",_logs_path.c_str(), rootName);
    ::unlink(oldFn);
    while(iold-- > 0)
    {
        ::sprintf(newFn, "%s/%s.log%d",_logs_path.c_str(), rootName ,iold);
        ::sprintf(oldFn, "%s/%s.log%d",_logs_path.c_str(), rootName, iold-1);
        ::rename(oldFn, newFn);
    }
}

