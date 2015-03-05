#ifndef OOHNGINE_H
#define OOHNGINE_H

#include <string>

class oohngine;
typedef int (oohngine::*prun)();
class oohngine
{
public:
    enum oohcmds
    {
        ooh_esc = 27,
        ooh_til = 96,
        ooh_q = 17,
        ooh_c = 3,
        ooh_d = 4,
        ooh_m = 13,
        ooh_j = 10,
        ooh_x = 24,
        ooh_z = 26,
        ooh_o = 15,
        ooh_h = 8,
        ooh_p = 16,
        ooh_ent = 13,
        ooh_localf = 5263131,
        ooh_alt_c = 25371,

    };
    oohngine(int nargs, char* vargs[]);
    virtual ~oohngine();
    bool    ok()const{return _port!=0;}
    int     run();
protected:
    int _filetp();
    int _server();
    int _client();

    void _execute_cmd();
    void _local_cmds(char* buff, int bytes, int kcode, tcp_cli_sock&  c);
private:

    bool            _cmd_mode;
    std::string     _ip;
    int             _port;
    std::string     _command;
    char            _pg;
    std::string     _fl;
    std::string     _fr;
    prun            _prun;
};

#endif // OOHNGINE_H
