/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/
#ifndef SQ__CLASS_H
#define SQ__CLASS_H

#include <matypes.h>
#include <modiface.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "sqwrap.h"

//class Sqrat::DefaultAllocator;
struct FastBuff;

class SqMod : public IModule
{
    friend class Sqrat::DefaultAllocator<SqMod>;
    NON_COPYABLE(SqMod);
public:
    SqMod(kchar* cook);
    virtual ~SqMod();
     bool   construct(CtxMod* pctx);
     int    work(CtxMod* pctx, CAN_RW rw);
     void   destroy();
     bool   is_singleton();
     bool   uses_fds()const;
     int    fd_action (FD_OPS op, fd_set& rd, fd_set& wr);


    static IModule*  get_threaded_singleton(kchar* cookie);
    static void    release_threaded_singleton(IModule* pm, kchar* cookie);
    size_t  swrite(kchar* s);
    size_t  save_file(kchar* where);
    size_t  sread(char* s,size_t len){
        return _pctx->socket_read(s, len);
    }
    size_t setCookie(kchar* key, kchar* val, int secondsfromnow);
    size_t header(kchar* hdr);
    CtxMod*  ctx(){return _pctx;};


private:
    void    log_string(kchar*, ...){
    }
private:
    int  _runScript(kchar* docu, kchar* docu_hash, time_t now, bool istmpl);
    int  _include( kchar* fin, FILE* fout, size_t ninsz);
    char* _newFileCntn(kchar* fname, size_t sz);
    const KeyValx* _getHdr(kchar* key)const;
    bool  _fill_form_data(Sqrat::Table& post, FILE* tempfile, size_t len);
    bool  _fill_mpart_data(Sqrat::Table& post, FILE* tempfile, size_t len, kchar* delim);
    bool  _save_put_data(Sqrat::Table& put, FILE* tempfile, size_t len);
    bool  _processTemplate(const char* docuname, std::string& buffer);
private:
    SqEnv   *_penv;
    CtxMod  *_pctx;
    size_t  _thread_id;
    bool    _header_sent;
    std::string  _cookies;
    std::string   _tmpfile;
    std::vector<void*> _exts;
};

#endif // SQCLASS_H
