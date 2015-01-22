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

#ifndef MODULES_H
#define MODULES_H

#include <modiface.h>
#include "config.h"

#define MAX_IFACES  32
//------------------------------------------------------------------------------
typedef void* SoHandler;

struct SoEntry
{
    SoEntry(SoHandler);
    ~SoEntry();

    SoHandler                   _sohndl;
    pFn_getFoo                  _so_get;
    pFn_releaseFoo              _so_release;
};

typedef std::map<std::string, SoEntry*> SoMap;
typedef std::map<std::string, SoEntry*>::const_iterator SoMapIt;

class Modules
{
public:
    Modules();
    virtual ~Modules();
    const SoEntry* get_module(kchar* ext);
    const SoMap&   get_modules()const{return _mods;};
    bool is_not_empty()const{return _imods>0;}
protected:
private:
    SoMap   _mods;
    size_t  _imods;
};

extern Modules*  __modules;

#endif // MODULES_H
