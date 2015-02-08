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

#include <dlfcn.h>
#include "tinyclasses.h"
#include <strutils.h>
#include "modules.h"

Modules*  __modules;

Modules::Modules():_imods(0)
{

    char  mod_fname[512];
    getcwd(mod_fname, sizeof(mod_fname)-1);//debug only ???

    //
    // default one
    //
    SoHandler h = dlopen("pizu-plugins/libhtml_mod.so", RTLD_NOW);//release if any
    if(0 == h)
        h = dlopen("pizu-plugins/libhtml_modd.so", RTLD_NOW);   //debug version
    if(0 == h){
        cout << "cannot load libhtml_mod(d).so plugin\n";
        return;
    }
    cout << "Loading module:\t\tplugins/libhtml_mod.so \n";
    ++_imods;
    _mods["*"] = new SoEntry(h);

    Mapss&  rm = GCFG->_extmodules;
    MapssIt it = rm.begin();
    for(; it != rm.end(); it++){
        //one per thread but one per extension ????
        //kchar* ftypes = it->first.c_str(); //debug inspect
        sprintf(mod_fname,"pizu-plugins/%s", it->second.c_str());
        if(it->second == "libhtml_mod.so"||it->second == "libhtml_modd.so")
            continue; // loaded by default
        SoHandler h = dlopen(mod_fname, RTLD_LAZY);
        if(h != NULL) {
            _mods[it->first] = new SoEntry(h);
            cout << "Loading module:\t\t" << mod_fname << "\n";
            ++_imods;
        }else
            cout << mod_fname << ": " << mod_fname << ", "<< dlerror() << "\r\n";
    }//for
}

Modules::~Modules()
{
    SoMapIt it = _mods.begin();
    for(; it != _mods.end(); it++){
        delete it->second;
    }
}

const SoEntry* Modules::get_module(kchar* by_name)
{
    static std::string sstar("*");

    if(0 == by_name || '\0' == by_name){
        return _mods[sstar];
    }
    SoMapIt it = _mods.begin();
    for(; it != _mods.end(); it++){
        const std::string& pkey = (*it).first;
        if(pkey.find(by_name) != string::npos){
            return (*it).second;
        }
    }
    //allways fallback the html one
    return _mods[sstar];
}


SoEntry::SoEntry(SoHandler ph):_sohndl(ph)
{
    _so_get = (pFn_getFoo)(dlsym(ph,"factory_get"));
    _so_release = (pFn_releaseFoo )(dlsym(ph,"factory_release"));
}

SoEntry::~SoEntry()
{
    dlclose(_sohndl);
}
