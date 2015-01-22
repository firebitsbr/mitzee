/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <time.h>
#include <sys/dir.h> //mco
#include <sys/stat.h> //mco
#include <fcntl.h>
#include <unistd.h> //mco
#include <string.h> //mco
#include <stdlib.h>
#include <stdio.h>
#include <sqstdsystem.h>

#ifdef SQUNICODE
#include <wchar.h>
#define scgetenv _wgetenv
#define scsystem _wsystem
#define scasctime _wasctime
#define scremove _wremove
#define screname _wrename
#else
#define scgetenv getenv
#define scsystem system
#define scasctime asctime
#define scremove remove
#define screname rename
#endif

static SQInteger _system_getenv(HSQUIRRELVM v)
{
	const SQChar *s;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
        sq_pushstring(v,scgetenv(s),-1);
		return 1;
	}
	return 0;
}


static SQInteger _system_system(HSQUIRRELVM v)
{
	const SQChar *s;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
		sq_pushinteger(v,scsystem(s));
		return 1;
	}
	return sq_throwerror(v,_SC("wrong param"));
}

static SQInteger _system_clock(HSQUIRRELVM v)
{
	sq_pushfloat(v,((SQFloat)clock())/(SQFloat)CLOCKS_PER_SEC);
	return 1;
}

static SQInteger _system_time(HSQUIRRELVM v)
{
	time_t t;
	time(&t);
	sq_pushinteger(v,*((SQInteger *)&t));
	return 1;
}

static SQInteger _system_remove(HSQUIRRELVM v)
{
	const SQChar *s;
	sq_getstring(v,2,&s);
	if(scremove(s)==-1)
		return sq_throwerror(v,_SC("remove() failed"));
	return 0;
}

static SQInteger _system_rename(HSQUIRRELVM v)
{
	const SQChar *oldn,*newn;
	sq_getstring(v,2,&oldn);
	sq_getstring(v,3,&newn);
	if(screname(oldn,newn)==-1)
		return sq_throwerror(v,_SC("rename() failed"));
	return 0;
}


static void _set_integer_slot(HSQUIRRELVM v,const SQChar *name,SQInteger val)
{
	sq_pushstring(v,name,-1);
	sq_pushinteger(v,val);
	sq_rawset(v,-3);
}

// mco
static void _set_string_slot(HSQUIRRELVM v,const SQChar *name,const SQChar *val)
{
	sq_pushstring(v,name,-1);
	sq_pushstring(v,val,-1);
	sq_rawset(v,-3);
}


static SQInteger _system_date(HSQUIRRELVM v)
{
	time_t t;
	SQInteger it;
	SQInteger format = 'l';
	if(sq_gettop(v) > 1) {
		sq_getinteger(v,2,&it);
		t = it;
		if(sq_gettop(v) > 2) {
			sq_getinteger(v,3,(SQInteger*)&format);
		}
	}
	else {
		time(&t);
	}
	tm *date;
    if(format == 'u')
		date = gmtime(&t);
	else
		date = localtime(&t);
	if(!date)
		return sq_throwerror(v,_SC("crt api failure"));
	sq_newtable(v);
	_set_integer_slot(v, _SC("sec"), date->tm_sec);
    _set_integer_slot(v, _SC("min"), date->tm_min);
    _set_integer_slot(v, _SC("hour"), date->tm_hour);
    _set_integer_slot(v, _SC("day"), date->tm_mday);
    _set_integer_slot(v, _SC("month"), date->tm_mon);
    _set_integer_slot(v, _SC("year"), date->tm_year+1900);
    _set_integer_slot(v, _SC("wday"), date->tm_wday);
    _set_integer_slot(v, _SC("yday"), date->tm_yday);
	return 1;
}
// mco
static SQInteger _system_getcwd(HSQUIRRELVM v)
{
    SQChar cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd)-1);
    sq_pushstring(v,cwd, strlen(cwd));
	return 1;
}


// mco
static SQInteger _system_chdir(HSQUIRRELVM v)
{
    const SQChar*  dir_name;
	sq_getstring(v,2,&dir_name);
	if(0!=chdir(dir_name))
        return sq_throwerror(v,_SC("chdir() failed"));
    return 1;
}

// mco
static SQInteger _system_mkdir(HSQUIRRELVM v)
{
    const SQChar*  dir_name;
    SQInteger  mode;

	sq_getstring(v,2,&dir_name);
	sq_getinteger(v,3,&mode);

	if(0!=mkdir(dir_name, mode))
        return sq_throwerror(v,_SC("mkdir() failed"));
    return 1;
}

// mco
static SQInteger _system_opendir(HSQUIRRELVM v)
{
    const SQChar*  dir_name;
	sq_getstring(v,2,&dir_name);
	DIR* pd = opendir(dir_name);
	if(pd == 0)
        return sq_throwerror(v,_SC("opendir() failed"));
    sq_pushuserpointer(v, (SQUserPointer)pd);
    return 1;
}
// mco
static SQInteger _system_closedir(HSQUIRRELVM v)
{
    SQUserPointer pd = 0;
    sq_getuserpointer(v, 2, &pd);
    if(0 == pd)
        return sq_throwerror(v,_SC("closedir() failed. pointer is null"));
    closedir((DIR*)pd);
    return 1;
}
// mco
static SQInteger _system_readdir(HSQUIRRELVM v)
{
    SQUserPointer pd = 0;
    sq_getuserpointer(v, 2, &pd);
    if(0 == pd)
        return sq_throwerror(v,_SC("closedir() failed. pointer is null"));
    struct  dirent  *dp = readdir((DIR*)pd);
    if(dp == 0)
        return 0;
    sq_pushstring(v,dp->d_name, strlen(dp->d_name));
    return 1;
}

// mco
static SQInteger _system_stat(HSQUIRRELVM v)
{
    const SQChar*  filename;
	sq_getstring(v,2,&filename);

    struct   stat     stt;
	stat(filename, &stt);
    struct tm * timeinfo = localtime(&stt.st_mtime);
    sq_newtable(v);
    _set_integer_slot(v, _SC("perm"), stt.st_mode);
    _set_string_slot(v, _SC("date"),  asctime(timeinfo));
    _set_integer_slot(v, _SC("size"), stt.st_size);
	return 1;
}

// mco
static SQInteger _system_popen(HSQUIRRELVM v)
{
	const SQChar *s;
	sq_getstring(v,2,&s);
	if(0 == s)
        return sq_throwerror(v,_SC("popen(null_pointer) error"));
    FILE* pipe = popen(s, "r");
    if(pipe)
        ::fcntl(fileno(pipe), F_SETFD, FD_CLOEXEC);
        sq_pushuserpointer(v,(SQUserPointer)pipe);
        return 1;
    return 0;
}

// mco
static SQInteger _system_pclose(HSQUIRRELVM v)
{
    SQUserPointer pd = 0;
    sq_getuserpointer(v, 2, &pd);
    if(0 != pd)
        pclose((FILE*)pd);
   return 1;
}

// mco
static SQInteger _system_pread(HSQUIRRELVM v)
{
    SQUserPointer pipe = 0;
    sq_getuserpointer(v, 2, &pipe);
    if(0 !=pipe)
    {
        if(!feof((FILE*)pipe)){
            char buffer[256];
            int ret = fread(buffer, 1, 255, (FILE*)pipe);
            if(ret){
                buffer[ret] = 0;
                sq_pushstring(v, buffer, ret);
                return 1;
            }
        }
    }
    return 0;
}

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_system_##name,nparams,pmask}
static SQRegFunction systemlib_funcs[]={
	_DECL_FUNC(getenv,2,_SC(".s")),
	_DECL_FUNC(system,2,_SC(".s")),
	_DECL_FUNC(clock,0,NULL),
	_DECL_FUNC(time,1,NULL),
	_DECL_FUNC(date,-1,_SC(".nn")),
	_DECL_FUNC(remove,2,_SC(".s")),
	_DECL_FUNC(rename,3,_SC(".ss")),
	//mco
	_DECL_FUNC(getcwd,0,NULL),
	_DECL_FUNC(chdir,2,_SC(".s")),
	_DECL_FUNC(mkdir,3,_SC(".sn")),
	_DECL_FUNC(opendir,2,_SC(".s")),  // string
	_DECL_FUNC(readdir,2,_SC(".p")), // user pointer
	_DECL_FUNC(closedir,2,_SC(".p")), // user pointer
    _DECL_FUNC(stat,2,_SC(".s")),    // filename
    _DECL_FUNC(popen,2,_SC(".s")),    // exec
    _DECL_FUNC(pclose,2,_SC(".p")),    // exec
    _DECL_FUNC(pread,2,_SC(".p")),    // exec
	{0,0}
};
#undef _DECL_FUNC

SQInteger sqstd_register_systemlib(HSQUIRRELVM v)
{
	SQInteger i=0;
	while(systemlib_funcs[i].name!=0)
	{
		sq_pushstring(v,systemlib_funcs[i].name,-1);
		sq_newclosure(v,systemlib_funcs[i].f,0);
		sq_setparamscheck(v,systemlib_funcs[i].nparamscheck,systemlib_funcs[i].typemask);
		sq_setnativeclosurename(v,-1,systemlib_funcs[i].name);
		sq_newslot(v,-3,SQFalse);
		i++;
	}
	return 1;
}
