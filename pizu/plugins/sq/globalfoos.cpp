/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/
#include <fcntl.h>
#include <sys/stat.h>

#include <string>
#include <fstream>
#include <streambuf>
#include <strutils.h>
#include "sqmod.h"

extern __thread SqMod*  __pSQ; //this is one per thread.

static SQInteger _mod_echo(HSQUIRRELVM v) {
    const SQChar*  content;
    sq_getstring(v,2,&content);
    if(0==content)return sq_throwerror(v,_SC("null content was passed to echo"));
    if(__pSQ)
        return (SQInteger)__pSQ->swrite((kchar*)content);
    return 0;
}

static SQInteger _mod_cookie(HSQUIRRELVM v) {
    const SQChar*  key, *val;
    SQInteger      secs;
    sq_getstring(v,2,&key);
    sq_getstring(v,3,&val);
    sq_getinteger(v,4,&secs);
    if(__pSQ)
        return (SQInteger)__pSQ->setCookie(key,val,secs);
    return 0;
}

static SQInteger _mod_header(HSQUIRRELVM v) {
    const SQChar*  hdr;
    sq_getstring(v,2,&hdr);
    return (SQInteger)__pSQ->header(hdr);
}

//save a br string
static SQInteger _mod_br(HSQUIRRELVM v) {
    return (SQInteger)__pSQ->swrite((kchar*)"<br>");
}

static SQInteger _mod_fopen(HSQUIRRELVM v) {
    const SQChar *fname,*how;
    sq_getstring(v,2,&fname);
    sq_getstring(v,3,&how);
    if(fname) {
        char fullname[512];
        if(fname[0]=='/')
            sprintf(fullname,"%s",fname);
        else
            sprintf(fullname,"%s/%s",__pSQ->ctx()->_dir_home,fname);
        FILE* pf = fopen(fullname, how);
        if(pf) {
            // ::fcntl(fileno(pf), F_SETFD, FD_CLOEXEC);
            sq_pushuserpointer(v,(SQUserPointer)pf);
            return 1;
        }
    }
    // ?? ERROR handling
    return 0;
}

static SQInteger _mod_fclose(HSQUIRRELVM v) {
    SQUserPointer pd = 0;
    sq_getuserpointer(v, 2, &pd);
    if(pd) {
        fclose((FILE*)pd);
    }
    return 1;
}

static SQInteger _mod_fread(HSQUIRRELVM v) {
    SQUserPointer pf = 0;
    SQInteger bytes;
    sq_getuserpointer(v, 2, &pf);
    sq_getinteger(v, 3, &bytes);

    if(pf) {
        char* pc = new char[bytes+1];
        if(pc) {
            bytes = fread(pc, 1, bytes, (FILE*)pf);
            if(bytes>0) {
                pc[bytes] = 0;
                sq_pushstring(v, pc, bytes);
                delete[] pc;
                return 1;
            }
            sq_pushstring(v, "", 0);
            delete[] pc;
        }
    }
    return 0;
}


static SQInteger _mod_fgets(HSQUIRRELVM v) {
    SQUserPointer pf = 0;
    char* bytes;
    sq_getuserpointer(v, 2, &pf);

    if(pf) {
        if(feof((FILE*)pf))
        {
            sq_pushstring(v, "", 0);
            return 0;
        }
        char local[256] = {0};
        bytes = fgets(local, 255, (FILE*)pf);
        if(bytes)
        {
            const char* pl = str_deleol(local);
            size_t len = strlen(pl);
            sq_pushstring(v, pl, len);
            return len;
        }
        sq_pushstring(v, "", 0);
    }
    return 0;
}


static SQInteger _mod_fwrite(HSQUIRRELVM v) {
    SQUserPointer pf = 0;
    const SQChar* data;
    sq_getuserpointer(v, 2, &pf);
    sq_getstring(v, 3, &data);
    if(pf) {
        return (SQInteger)fprintf((FILE*)pf,"%s", data);
    }
    return 0;
}
static SQInteger _mod_fseek(HSQUIRRELVM v) {
    SQUserPointer pf = 0;
    SQInteger seek;
    SQInteger pos;
    sq_getuserpointer(v, 2, &pf);
    sq_getinteger(v, 3, &seek);
    sq_getinteger(v, 4, &pos);
    if(pf) {
        fseek((FILE*)pf,seek,pos);
        return 1;
    }
    return 0;

}

static SQInteger _mod_ftell(HSQUIRRELVM v) {
    SQUserPointer pf = 0;
    sq_getuserpointer(v, 2, &pf);
    if(pf) {
        sq_pushinteger(v, ftell((FILE*)pf));
        return 1;
    }
    return -1;
}

static SQInteger _mod_fsize(HSQUIRRELVM v) {
    SQUserPointer pf = 0;
    sq_getuserpointer(v, 2, &pf);
    if(pf) {
        struct stat stt;
        if(fstat(fileno((FILE*)pf), &stt)) {
            sq_pushinteger(v, stt.st_size);
            return stt.st_size;
        }
    }
    return -1;
}

static SQInteger _mod_readfile(HSQUIRRELVM v) {
    const SQChar* fname;
    SQInteger   bytes=0;
    sq_getstring(v, 2, &fname);
    sq_getinteger(v, 3, &bytes);
    FILE * pf = fopen(fname,"rb");
    if(!pf) {return sq_throwerror(v,_SC("popen(null_pointer, 0 bytes) error"));}
    if(pf) {
        char* pc = new char[bytes+1];
        if(pc) {
            bytes = fread(pc, 1, bytes, (FILE*)pf);
            if(bytes>0) {
                pc[bytes] = 0;
                sq_pushstring(v, pc, bytes);
            }
            delete[] pc;
        }
    }
    return 1;
}

static SQInteger _mod_filesize(HSQUIRRELVM v) {
    const SQChar* fname;
    sq_getstring(v, 2, &fname);
    if(fname) {
        char fullname[512];
        if(fname[0]=='/')
            sprintf(fullname,"%s",fname);
        else
            sprintf(fullname,"%s/%s",__pSQ->ctx()->_dir_home,fname);
        struct stat stt;
        if(stat(fullname, &stt)==0) {
            sq_pushinteger(v, stt.st_size);
            return stt.st_size;
        }else{
            sq_pushinteger(v, 0);
        }
    }
    return 1;
}

static SQInteger _mod_filestrstr(HSQUIRRELVM v) {
    const SQChar* fname;
    const SQChar* fsubstr;
    sq_getstring(v, 2, &fname);
    sq_getstring(v, 3, &fsubstr);

    if(fname && fsubstr) {
        int found = 0;
        char fullname[512];
        if(fname[0]=='/')
            sprintf(fullname,"%s",fname);
        else
            sprintf(fullname,"%s/%s",__pSQ->ctx()->_dir_home,fname);
        FILE* pf = fopen(fullname, "rb");
        if(!pf) {return sq_throwerror(v,_SC("popen(null_pointer, 0 bytes) error"));}
        char line[256], *pc;
        while(!feof(pf)) {
            if(0==fgets(line, 255, pf)) break;
            if((pc = strstr(line, fsubstr))) {
                found = ftell(pf)+(pc-line);
                break;
            }
        }
        fclose(pf);
        sq_pushinteger(v, found);
        return found;
    }
    return -1;
}

static SQInteger _mod_filereplace(HSQUIRRELVM v) {
    const SQChar* fname;
    const SQChar* oldStr,*newStr;
    sq_getstring(v, 2, &fname);
    sq_getstring(v, 3, &oldStr);
    sq_getstring(v, 4, &newStr);
    if(fname && oldStr && newStr) {
        char fullname[512];
        if(fname[0]=='/')
            sprintf(fullname,"%s",fname);
        else
            sprintf(fullname,"%s/%s",__pSQ->ctx()->_dir_home,fname);

        std::ifstream t(fullname);
        std::string str;
        if(!t.bad())
        {
            t.seekg(0, std::ios::end);
            str.reserve(t.tellg());
            t.seekg(0, std::ios::beg);

            str.assign((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
            size_t pos = 0;
            while((pos = str.find(oldStr, pos)) != std::string::npos)
            {
                str.replace(pos, strlen(oldStr), newStr);
                pos += strlen(newStr);
            }
            t.close();
            std::ofstream t(fullname);
            t << str;
            t.close();
            return 1;
        }
    }
    return -1;
}



// mco
static SQInteger _mod_spawn(HSQUIRRELVM v) {
    const SQChar *s;
    SQInteger  bytes;

    sq_getstring(v,2,&s);
    sq_getinteger(v,3,&bytes);
    if(0 == s || bytes==0)
        return sq_throwerror(v,_SC("popen(null_pointer, 0 bytes) error"));
    SQInteger ret =-1;
    FILE* pipe = popen(s, "r");
    if(pipe) {
        ::fcntl(fileno(pipe), F_SETFD, FD_CLOEXEC);
        char* buffer = new char[bytes+1];
        if(0 == buffer) {
            pclose((FILE*)pipe);
            return sq_throwerror(v,_SC("popen(null_pointer, 0 bytes) error"));
        }
        ret = fread(buffer, 1, bytes, pipe);
        if(ret) {
            buffer[ret] = 0;
            sq_pushstring(v, buffer, ret);
        }
        delete[] buffer;
        pclose((FILE*)pipe);
    }
    return ret;
}

static SQInteger _mod_sendfile(HSQUIRRELVM v) {
    const SQChar* fname;
    sq_getstring(v, 2, &fname);
    if(fname) {
        size_t sz,ammnt=0;
         char fullname[512];
        if(fname[0]=='/')
            sprintf(fullname,"%s",fname);
        else
            sprintf(fullname,"%s/%s",__pSQ->ctx()->_dir_home,fname);
        FILE* pf = fopen(fullname, "rb");

        if(!pf) {return sq_throwerror(v,_SC("popen(null_pointer, 0 bytes) error"));}
        char line[1024];
        while(!feof(pf)) {
            if(!fgets(line, 1022, pf)) break;
            sz = strlen(line);
            if(0!=__pSQ->swrite(line)){
                break;
            }
            ammnt+=sz;
        }
        fclose(pf);
        return ammnt;
    }
    return -1;
}

inline void  str_time(char* dTime)
{
    time_t  curtime = time(0);
    strcpy(dTime, ctime(&curtime));
    char *pe = strchr(dTime,'\r'); if(pe)*pe=0;
        pe = strchr(dTime,'\n'); if(pe)*pe=0;
}

static SQInteger _mod_filelog(HSQUIRRELVM v) {
    const SQChar* fname;
    const SQChar* text;
    sq_getstring(v, 2, &fname);
    sq_getstring(v, 3, &text);
    if(fname && text) {
        char fullname[512],tmm[128];
        if(fname[0]=='/')
            sprintf(fullname,"%s",fname);
        else
            sprintf(fullname,"%s/%s",__pSQ->ctx()->_dir_home,fname);
        FILE* pf = fopen(fullname, "ab");
        if(!pf) {return sq_throwerror(v,_SC("fopen(null_pointer, 0 bytes) error"));}
        str_time(tmm);
        fprintf(pf,"[%s] \t%s\n", tmm, text);
        fclose(pf);
        return 1;
    }
    return -1;
}

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_mod_##name,nparams,pmask}
static SQRegFunction global_funcs[]= {
    _DECL_FUNC(echo,2,_SC(".s")),  // _this.write
    _DECL_FUNC(header,2,_SC(".s")),  // _this.write
    _DECL_FUNC(cookie,4,_SC(".ssn")),  // _this.write
    _DECL_FUNC(br,1,NULL),  // _this.write
    _DECL_FUNC(fopen,3, _SC(".ss")),
    _DECL_FUNC(fclose,2,_SC(".p")),
    _DECL_FUNC(fread,3,_SC(".pn")),
    _DECL_FUNC(fwrite,3,_SC(".ps")),
    _DECL_FUNC(fseek,4,_SC(".snn")),
    _DECL_FUNC(ftell,2,_SC(".p")),
    _DECL_FUNC(fsize,2,_SC(".p")),
    _DECL_FUNC(fgets,2,_SC(".p")),
    //
    _DECL_FUNC(readfile,3,_SC(".sn")),
    _DECL_FUNC(filesize,2,_SC(".s")),
    _DECL_FUNC(filestrstr,3,_SC(".ss")),
    _DECL_FUNC(spawn,3,_SC(".sn")),
    _DECL_FUNC(sendfile,2,_SC(".s")),
    _DECL_FUNC(filelog,3,_SC(".ss")),
    _DECL_FUNC(filereplace,4,_SC(".sss")),

    {0,0}
};
#undef _DECL_FUNC


SQInteger regGlobals(HSQUIRRELVM v) {
    SQInteger i=0;
    SQRegFunction* pfpps = global_funcs;
    while(pfpps[i].name!=0) {
        sq_pushstring(v,pfpps[i].name,-1);
        sq_newclosure(v,pfpps[i].f,0);
        sq_setparamscheck(v,pfpps[i].nparamscheck,pfpps[i].typemask);
        sq_setnativeclosurename(v,-1,pfpps[i].name);
        sq_newslot(v,-3,SQFalse);
        i++;
    }
    return 1;
}

