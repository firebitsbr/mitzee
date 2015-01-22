/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/
#ifndef SQWRAP_H
#define SQWRAP_H

#include <string>
#include <stdio.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include <sqstdblob.h>
#include <sqstdmath.h>
#include <sqstdsystem.h>
#include <sqstdstring.h>
#include <sqrat.h>
#include <os.h>


class SqEnv;
class MyScript
{
public:
    friend class SqEnv;
    MyScript(const MyScript& r)
    {
        _vm = r._vm;
        _obj = r._obj;
        sq_addref(_vm, &_obj);
    }
    ~MyScript()
    {
        reset();
    }

    void reset()
    {
        if(_vm && _obj._type!=OT_NULL)
            sq_release(_vm, &_obj);
        else
            if( _obj._type!=OT_NULL && _obj._unVal.pRefCounted )
                printf( "SquirrelObject::~SquirrelObject - Cannot release\n" );
        sq_resetobject(&_obj);

        sq_pop(_vm, 1);
    }

    bool run_script() // <---- FIX (from SqPLus)
    {
        Sqrat::DefaultVM::Set(_vm);
        sq_pushobject(_vm,_obj);
        sq_pushroottable(_vm);
        if(SQ_SUCCEEDED(sq_call(_vm,1,true,true/*we need a return value*/)))
        {
            SQObject obj;
            sq_getstackobj(_vm,-1,&obj);
            sq_pop(_vm,2);
            return true;
        }
        sq_pop(_vm,1);
        throw Sqrat::Exception(Sqrat::LastErrorString(_vm));
        return false;//never reached
    }

    bool RunOld()const
    {
        Sqrat::DefaultVM::Set(_vm);

        if(!sq_isnull(_obj))
        {
            sq_pushobject(_vm, _obj);
            sq_pushroottable(_vm);
            if(SQ_FAILED(sq_call(_vm, 1, false, true)))
            {
                //return false;
                throw Sqrat::Exception(Sqrat::LastErrorString(_vm));
            }
        }
        return true;
    }
    template <class R>
    R f_call(Sqrat::Function& f)
    {
        return f.Evaluate<R>();
    }

    template <class R, class T>
    R f_call(Sqrat::Function& f, const T t)
    {
        return f.Evaluate<R>(t);
    }

    template <class R, class T, class T1>
    R f_call(Sqrat::Function& f, const T t, const T1 t1)
    {
        return f.Evaluate<R>(t, t1);
    }


    template <class R>
    R f_call(const SQChar* foo)
    {
        Sqrat::Function f(Sqrat::RootTable(), foo);
        return f.Evaluate<R>();
    }

    template <class R, class T>
    R f_call(const SQChar* foo, const T t)
    {
        Sqrat::Function f(Sqrat::RootTable(), foo);
        return f.Evaluate<R>(t);
    }

    template <class R, class T, class T1>
    R f_call(const SQChar* foo, const T t, const T1 t1)
    {
        Sqrat::Function f(Sqrat::RootTable(), foo);
        return f.Evaluate<R>(t, t1);
    }

    void WriteCompiledFile(const std::string& path)
    {
        Sqrat::DefaultVM::Set(_vm);

        if(!sq_isnull(_obj))
        {
            sq_pushobject(_vm, _obj);
            sqstd_writeclosuretofile(_vm, path.c_str());
        }
    }
private:
    MyScript(HSQUIRRELVM vm, const SQObject& o )
    {
        sq_resetobject(&_obj);
        _vm = vm;
        _obj = o;
        sq_addref(_vm, &_obj);
    }
    HSQOBJECT      _obj; //HSQOBJECT
    HSQUIRRELVM   _vm;
};

typedef void (*HookPrint)(const SQChar*);

class SqEnv
{

public:
    SqEnv(size_t stack=1024);
    ~SqEnv();

    void acquire(){Sqrat::DefaultVM::Set(*_vm);}

    static void set_print_foo(HookPrint  hp);//{_hook_print = hp;};
    MyScript compile_script(const std::string& s,  const SQChar * debugInfo=_SC("console_buffer"))const;
    MyScript compile_buffer(const SQChar *s , size_t length, const SQChar * debugInfo=_SC("console_buffer"))const;
    MyScript compile_buffer(const std::string& s, const SQChar * debugInfo=_SC("console_buffer"))const;

    MyScript* compile_script_new(const SQChar* s,  const SQChar * debugInfo=_SC("console_buffer"))const;
    void reset();
    HSQUIRRELVM theVM(){return *_vm;}
private:
    static void debug_hook(HSQUIRRELVM /*v*/,
                    SQInteger tip/*type*/,
                    const SQChar * s/*sourcename*/,
                    SQInteger line/*line*/,
                    const SQChar * func/*funcname*/);
    static void print_func(HSQUIRRELVM, const SQChar * s,...);
    static SQInteger error_handler(HSQUIRRELVM v);
    void _init(size_t sz=1024);



private:
    HSQUIRRELVM* _vm;
    mutex        _m;
};



#endif // SQWRAP_H
