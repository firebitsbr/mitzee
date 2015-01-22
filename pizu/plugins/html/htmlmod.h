/*
    Author: Marius Octavian Chincisan, Jan-Aug 2012
    Copyright: Marius C.O.
*/
#ifndef CHUNKPROC_H
#define CHUNKPROC_H

#include <modiface.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <map>
#include <set>
//-----------------------------------------------------------------------------
// the non script module is in-engine
class HtmlModule : public IModule
{
    NON_COPYABLE(HtmlModule);
public:
    typedef int (HtmlModule::*PF_hMeth)(CtxMod* pctx, CAN_RW rw );

    enum POST_STATE{
        eRECPOST,
    };

    HtmlModule(const char* cook);
    virtual ~HtmlModule();
     bool   construct(CtxMod* pctx);
     int  work(CtxMod* pctx, CAN_RW rw);
     void   destroy();
     bool   is_singleton();
     bool   uses_fds()const;
     int    fd_action (FD_OPS op, fd_set& rd, fd_set& wr);

private:
    int _reply_directory_listing(const char* path, const char*, const char*);
    int _reply_file_content(const char* file, bool jh=false);
    int _reply_raw_content(const char* file, const char* ext, bool jh=false);
    int _keep_streaming_on();
    int64_t _sendHeader(FILE* fp, const char* ext, const struct stat& stt);
    const char* _getHeader(const char* key)const;

private:
    int get_work(CtxMod* pctx, CAN_RW rw);
    int post_work(CtxMod* pctx, CAN_RW rw);
    int head_work(CtxMod* pctx, CAN_RW rw);
    int put_work(CtxMod* pctx, CAN_RW rw);
private:
    FILE*         _pf;
    int64_t       _left_bytes;
    int64_t       _initial;
    CtxMod*       _pctx;
    PF_hMeth      _pfn_method;
    struct stat   _stat;
    char          _ext[16];
};




#endif // CHUNKPROC_H
