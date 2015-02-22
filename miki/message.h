/**
# Copyright (C) 2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - coinscode.com - N/A
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



#ifndef _DNSMESSAGE_H
#define	_DNSMESSAGE_H

#include <sys/types.h>
#include <string.h>
#include <string>
#include <vector>
#include "arpa/nameser_compat.h"


#define NOERR    0
#define FMT_ERR  1
#define FATAL    2
#define NAMEERR  3
#define NOTIMPL  4
#define REFUSEDX 5
#define  BSZ     1024



class Message
{
public:
    Message():_sz(0),_fromfile(false){};
    virtual ~Message() {}


    struct Question
    {
        Question(){
            memset(this, 0, sizeof(*this));
        }
        char        name[256];
        bool        is_ref;
        u_int16_t   qtype;
        u_int16_t   qclass;
    };

    struct Answer
    {
        Answer(){
            memset(this, 0, sizeof(*this));
        }
        char        name[256];
        char        alias[256];
        bool        is_ref;
        u_int16_t   qtype;
        u_int16_t   qclass;
        u_int32_t   ttl;
        u_int16_t   rdlength;

        in_addr     real_ip_host;
        in_addr     newip;

        uint8_t*    p_real_host; // ptr in buffer
        uint32_t*   pttl;    // ptr in buffer
		u_int8_t	bytes[160];
    };
    void    clear();
    bool    parse(size_t len);
    void    replace();
    bool    is_cahced();
    void    cache(const string& sig, const SADDR_46& clientadd);
    void    load(const string& sig, const SADDR_46& clientadd);
    void    set_id(u_int16_t id);
    bool    replace_domains();
private:
    int          decodeName(const u_int8_t * data, const u_int8_t * name, char * dest);
    const char*  getRRTypeName(unsigned short t);
    const char*  getRRClassName(unsigned short c);
    void        _decode_answer(const char*& pdata, u_int16_t cnt, std::vector<Answer>& vec);
    bool        _find_in_questions(const char* respname);

public:
    HEADER                _h;
    std::vector<Question> _requests;
    std::vector<Answer>   _responses;
    std::vector<Answer>   _nss;
    std::vector<Answer>   _additional;
    u_int8_t              _buff[BSZ];
    int                   _sz;
    std::stringstream    _mid;
    bool                 _fromfile;
};


typedef enum
{
    A       = 1,
    NS      = 2,
    MD      = 3,
    MF      = 4,
    CNAME   = 5,
    SOA     = 6,
    MB      = 7,
    MG      = 8,
    MR      = 9,
    NULLRR  = 10,
    WKS     = 11,
    PTR     = 12,
    HINFO   = 13,
    MINFO   = 14,
    MX      = 15,
    TXT     = 16
} DNSRRTYPE;

typedef enum
{
    CLASSIN = 1,
    CLASSCS = 2,
    CLASSCH = 3,
    CLASSHS = 4
} DNSCLASS;

#endif	/* _DNSMESSAGE_H */

