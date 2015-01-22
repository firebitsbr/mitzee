//
// SqratScript: Script Compilation and Execution
//

//
// Copyright (c) 2009 Brandon Jones
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#if !defined(_SCRAT_SCRIPT_H_)
#define _SCRAT_SCRIPT_H_

#include <squirrel.h>
#include <sqstdio.h>
#include <string.h>

#include "sqratObject.h"

namespace Sqrat {

class Script : public Object {
public:
    Script(HSQUIRRELVM v = DefaultVM::Get()) : Object(v, false) {
    }
    virtual ~Script(){//mco
    }

    void CompileString(const char* script, size_t length) {
        if(obj._type != OT_NULL)
            sq_release(vm, &obj);

        if(SQ_FAILED(sq_compilebuffer(vm, script,
                                      static_cast<SQInteger>(length),
                                      _SC(""), true)))
        {
            throw Exception(LastErrorString(vm));
        }
        sq_getstackobj(vm,-1,&obj);
    }


    void CompileString(const string& script) {
        if(obj._type != OT_NULL)
            sq_release(vm, &obj);

        if(SQ_FAILED(sq_compilebuffer(vm, script.c_str(),
                                      static_cast<SQInteger>(script.length() * sizeof(SQChar)),
                                      _SC(""), true)))
        {
            throw Exception(LastErrorString(vm));
        }
        sq_getstackobj(vm,-1,&obj);
    }

    bool CompileFile(const string& path) {
        if(obj._type != OT_NULL)
            sq_release(vm, &obj);

        if(SQ_FAILED(sqstd_loadfile(vm, path.c_str(), true))) {
            //return false;
            throw Exception(LastErrorString(vm));
        }
        sq_getstackobj(vm,-1,&obj);
        return true;
    }

    bool Run()const {
        if(!sq_isnull(obj)) {
            sq_pushobject(vm, obj);
            sq_pushroottable(vm);
            if(SQ_FAILED(sq_call(vm, 1, false, true))) {
                //return false;
                throw Exception(LastErrorString(vm));
            }
        }
        return true;
    }

    void WriteCompiledFile(const string& path) {
        if(!sq_isnull(obj)) {
            sq_pushobject(vm, obj);
            sqstd_writeclosuretofile(vm, path.c_str());
        }
    }
};
}

#endif
