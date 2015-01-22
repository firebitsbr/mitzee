/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/

#include <assert.h>
#include <stdarg.h>
#include <iostream>
#include "sqwrap.h"

static __thread HookPrint _hook_print;
static __thread HSQUIRRELVM __vm;

/*static*/
void SqEnv::set_print_foo(HookPrint  hp)
{
    _hook_print = hp;
}


SqEnv::SqEnv(size_t sz):_vm(0)
{
    AutoLock a(&_m);
    _init(sz);
}

SqEnv::~SqEnv()
{
    AutoLock a(&_m);
    Sqrat::DefaultVM::Set(*_vm);
    sq_close(*_vm);
    *_vm = 0;
}

void SqEnv::_init(size_t sz)
{

   // AutoLock a(&_m);
    __vm = sq_open(sz);
    _vm= &__vm;

    assert( *_vm );

    Sqrat::DefaultVM::Set(*_vm);
    sq_setprintfunc(*_vm, SqEnv::print_func, SqEnv::print_func);
    sq_newclosure(*_vm, SqEnv::error_handler,0);
    sq_seterrorhandler(*_vm);
    //sq
    sq_pushroottable(*_vm);
    sqstd_register_iolib(*_vm);
    sqstd_register_bloblib(*_vm);
    sqstd_register_mathlib(*_vm);
    sqstd_register_stringlib(*_vm);
    sqstd_register_systemlib(*_vm);

    sqstd_seterrorhandlers(*_vm);
    sqstd_printcallstack(*_vm);

//    setnativedebughook(_vmsys,debug_hook);
    sq_notifyallexceptions(*_vm, true);
}

void SqEnv::debug_hook(HSQUIRRELVM /*v*/,
                       SQInteger tip/*type*/,
                       const SQChar * s/*sourcename*/,
                       SQInteger line/*line*/,
                       const SQChar * func/*funcname*/)
{
//    AutoLock a(&_m);
    char buff[512];
    sprintf (buff, "%d: %d  %s, %s\n",(int)tip, (int)line, func,s);
    if(_hook_print){
        _hook_print(buff);
    }else{
        std::cout << buff << "\r\n";
    }
}

void SqEnv::print_func(HSQUIRRELVM, const SQChar * s,...)
{
//    AutoLock a(&_m);
    char szBuffer[512];

    va_list args;
    va_start(args, s);
    ::vsnprintf(szBuffer,128, s, args);
    va_end(args);
    if(_hook_print){
        _hook_print(szBuffer);
    }else{
        std::cout << szBuffer;
    }
}


SQInteger SqEnv::error_handler(HSQUIRRELVM v)
{
//    AutoLock a(&_m);
    const SQChar *sErr = 0;
    if(sq_gettop(v)>=1)
    {
        if(SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))
        {
            if(_hook_print){
                _hook_print(sErr);
            }else{
                std::cout << _SC("A Script Error Occured: ") << sErr  << "\n";
            }
        }
        else
        {
            if(_hook_print){
                _hook_print("Unknown Script Error");
            }else{
                std::cout << _SC("An Unknown Script Error Occured.") << sErr << "\n";
            }
        }
    }
    return 0;
}

MyScript* SqEnv:: compile_script_new(const SQChar* s,  const SQChar * debugInfo)const
{
   /// AutoLock a(&_m);
    Sqrat::DefaultVM::Set(*_vm);

    SQObject obj;
    if(SQ_FAILED(sqstd_loadfile(*_vm, s, true)))
    {
        //return false;
        throw Sqrat::Exception(Sqrat::LastErrorString(*_vm));
    }
    sq_getstackobj(*_vm,-1,&obj);
    //sq_pop(_vm,1); //mco
    return new MyScript(*_vm, obj);
}

MyScript SqEnv::compile_script(const std::string& s,  const SQChar * debugInfo)const
{
   // AutoLock a(&_m);
    Sqrat::DefaultVM::Set(*_vm);

    SQObject obj;
    if(SQ_FAILED(sqstd_loadfile(*_vm, s.c_str(), true)))
    {
        //return false;
        throw Sqrat::Exception(Sqrat::LastErrorString(*_vm));
    }
    sq_getstackobj(*_vm,-1,&obj);
    //sq_pop(_vm,1); //mco
    return MyScript(*_vm, obj);
}

MyScript SqEnv::compile_buffer(const SQChar *s,  size_t length, const SQChar * debugInfo)const
{
    //AutoLock a(&_m);
    Sqrat::DefaultVM::Set(*_vm);

    SQObject obj;
    if(SQ_FAILED(sq_compilebuffer(*_vm, s,
                                  static_cast<SQInteger>(length),
                                  debugInfo, true)))
    {
        throw Sqrat::Exception(Sqrat::LastErrorString(*_vm));
    }
    sq_getstackobj(*_vm,-1,&obj);
    return MyScript(*_vm, obj);
}

MyScript SqEnv::compile_buffer(const std::string& s, const SQChar * debugInfo)const
{
    return SqEnv::compile_buffer(_SC(s.c_str()), s.length(), debugInfo);
}

void SqEnv::reset()
{
    AutoLock a(&_m);
    sq_pop(*_vm, 1);
	sq_close(*this->_vm);
	this->_init();
}

