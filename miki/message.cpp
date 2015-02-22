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


#include <stdio.h>
#include <sock.h>
#include <strutils.h>
#include "message.h"
#include "watcher.h"
#include <consts.h>
#include "config.h"
#include "theapp.h"

void    Message::set_id(u_int16_t id)
{
    HEADER * ph = (HEADER *)_buff;
    ph->id = ntohs(id);
    //printf("msg id isset: %d \n", ntohs(ph->id));
}

bool    Message::parse(size_t len)
{
    GLOGX("-----------------------------------------------------");
    const char * pdata = (const char *)_buff;
    _sz = len;
    /*
    for (size_t n = 0; n<len; n++)
    {
        printf("%02x ", _buff[n]&0xff);
        if(n%12==0)
            GLOGX("");
    }
    */

    GLOGX("Heading section\n");
    memcpy(&_h, pdata, sizeof(HEADER));
    pdata+=sizeof(HEADER);

    _h.id = ntohs(_h.id);
    //printf("msg id is: %d \n", _h.id);

    GLOGX( "QR = "<< (_h.qr ? "Response":"Query") );

    char * opcode;
    switch ((_h.rcode>>3)&0x0f)
    {
    case 0:
        opcode=_CC("QUERY");
        break;
    case 1:
        opcode=_CC("IQUERY");
        break;
    case 2:
        opcode=_CC("STATUS");
        break;
    default:
        opcode=_CC("RESERVED");
    }
    GLOGX("OPCODE = "<< opcode);
    GLOGX("ID = "<< (_h.id));
    GLOGX("QR = "<< ((_h.qr) ? "RESPONSE":"QUERY"));
    GLOGX("AA = "<< ((_h.aa) ? "AUTHORITATIVE":"NON AUTHORITATIVE"));
    GLOGX("TC = "<< ((_h.tc) ? "TRUNCATED":"NON TRUNCATED"));
    GLOGX("RD = "<< ((_h.rd) ? "DESIRED":"NON DESIRED"));
    GLOGX("RA = "<< ((_h.ra) ? "AVAILABLE":"NON AVAILABLE"));

    char * rcode;
    switch (_h.rcode)
    {
    case 0:
        rcode=_CC("OK");
        break;
    case 1:
        rcode=_CC("Format Error");
        break;
    case 2:
        rcode=_CC("Server Failure");
        break;
    case 3:
        rcode=_CC("Name Error");
        break;
    case 4:
        rcode=_CC("Not Implemented");
        break;
    case 5:
        rcode=_CC("Refused");
        break;
    default:
        rcode=_CC("Unknown rcode");
    }
    GLOGX("RCODE = "<< rcode);

    _h.ancount = ntohs(_h.ancount);
    _h.qdcount = ntohs(_h.qdcount);
    _h.nscount = ntohs(_h.nscount);
    _h.arcount = ntohs(_h.arcount);

    GLOGX("QDCOUNT = "<< _h.qdcount);
    GLOGX("ANCOUNT = "<< _h.ancount);
    GLOGX("NSCOUNT = "<< _h.nscount);
    GLOGX("ARCOUNT = "<< _h.arcount);

    _mid << "F_"<<_h.qdcount;

    u_int16_t qdcount = _h.qdcount;
    while (qdcount--)
    {
        GLOGX("---Question section\n");
        Question    q;

        pdata += decodeName(_buff, (const u_int8_t *)pdata, q.name);
        q.qtype = ntohs(*((unsigned short *)pdata));
        pdata+=2;
        q.qclass = ntohs(*((unsigned short *)pdata));
        pdata+=2;
        GLOGX("QNAME = "<< q.name);
        GLOGX("QTYPE = "<< q.qtype);
        GLOGX("QCLASS = "<< q.qclass);

        _mid << "F_"<<q.name <<"_"<< q.qtype <<"_"<< q.qclass <<"-";

        _requests.push_back(q);
    }

//    int type;
//    int cclass;
//    unsigned int ttl;
//    int rdlength;
//    bool ancountprint=false,nscountprint=false,arcountprint=false;

    u_int16_t ancount = _h.ancount;
    u_int16_t nscount = _h.nscount;
    u_int16_t arcount = _h.arcount;
    GLOGX("-----------Answer an  :"<< ancount);
    _decode_answer(pdata, ancount,_responses);
    GLOGX("-----------Answer ns: "<< nscount);
    _decode_answer(pdata, nscount,_nss);
    GLOGX("-----------Answer ar: "<< arcount);
    _decode_answer(pdata, arcount,_additional);

    _mid << ".cache";
//    GLOGD("message signature:" << _mid.str().c_str());



    return true;

}

bool ares_expand_name(const u_int8_t *encoded,
                      const u_int8_t *abuf,
                      int alen, char *s, long *enclen);

void  Message::_decode_answer(const char*& pdata, u_int16_t cnt, std::vector<Answer>& vec)
{
    //const char* hostname = _requests[0].name;

    while(cnt--)
    {
        GLOGX(" :---------------- Answer Number: "<<   cnt);
        Answer a;

        pdata += decodeName((const u_int8_t *)_buff,
                            (const u_int8_t *)pdata, a.name);
        a.qtype = ntohs(*((unsigned short *)pdata));
        pdata+=2;
        a.qclass = ntohs(*((unsigned short *)pdata));
        pdata+=2;
        a.pttl = (u_int32_t *)pdata;
        a.ttl = ntohl(*((unsigned long *)pdata));
        pdata+=4;
        a.rdlength = ntohs(*((u_int16_t *)pdata));
        pdata+=2;

        GLOGX("aname  = "<< a.name);
        GLOGX("qtype  = "<<  getRRTypeName(a.qtype) <<"/"<<a.qtype);
        GLOGX("qclass = "<<  getRRClassName(a.qclass)<<"/"<<a.qclass);
        GLOGX("ttl    = "<<  a.ttl);
        GLOGX("rdlen  = "<<  a.rdlength);

        a.p_real_host = 0;

        if (a.qclass   == C_IN &&
            a.qtype    == T_A &&
            a.rdlength == sizeof(struct in_addr))
        {
			::memcpy(&a.real_ip_host, pdata, sizeof(struct in_addr));

			GLOGI("real_host IP:" << inet_ntoa(a.real_ip_host));

            a.p_real_host = (uint8_t*)pdata;            // ptr in buffer where the addr we replace is located
            pdata   += a.rdlength;
        }
        else if (a.qclass == C_IN && a.qtype == T_CNAME)
        {
            strcat(a.alias, a.name);
            strcat(a.alias, ",");
            long len = 0;
            //ares_expand_name(name, _buff,  _sz, dest, &enclen);
            if(!ares_expand_name((const u_int8_t *)pdata, _buff, _sz, (char*)a.bytes, &len))
            {
                break;
            }
            //hostname = (const char*)a.u.bytes;
            GLOGX("alias = "<<   a.alias <<" len:"<< len);
            GLOGX("ip = "<<   a.bytes);
            pdata += len;
        }
        else
        {
            pdata += a.rdlength;
        }

        if ((char*)pdata - (char*)_buff > _sz)
        {
            GLOGE("error. invalid length buffer:  out sz" << (char*)pdata - (char*)_buff  << " sz " <<_sz);
            break;
        }
        GLOGX("\n");
        vec.push_back(a);
    }
}

int Message::decodeName(const u_int8_t * data, const u_int8_t * name, char * dest)
{
    long enclen = 0;
    ares_expand_name(name, _buff,  _sz, dest, &enclen);
    return enclen;
}

const char * Message::getRRTypeName(unsigned short t)
{
    switch (t)
    {
    case A:
        return "A";
    case NS:
        return "NS";
    case MD:
        return "MD";
    case MF:
        return "MF";
    case CNAME:
        return "CNAME";
    case SOA:
        return "SOA";
    case MB:
        return "MB";
    case MG:
        return "MG";
    case MR:
        return "MR";
    case NULLRR:
        return "NULL";
    case WKS:
        return "WKS";
    case PTR:
        return "PTR";
    case HINFO:
        return "HINFO";
    case MINFO:
        return "MINFO";
    case MX:
        return "MX";
    case TXT:
        return "TXT";
    }
    return "UNKNOWN";
};

const char * Message::getRRClassName(unsigned short c)
{
    switch (c)
    {
    case CLASSIN:
        return "IN";
    case CLASSCS:
        return "CS";
    case CLASSCH:
        return "CH";
    case CLASSHS:
        return "HS";
    }
    return "UNKNOWN";
}

bool    Message::_find_in_questions(const char* respname)
{
    return true;
    /* check is the answer is related to our question ?!?!
    if(!strncmp(respname,"ns",2))
        return false;
    std::vector<Question>::iterator it =  _requests.begin();
    for(;it != _requests.end(); ++it)
    {
        if(!strcasecmp(it->name, respname))
        {
            return true;
        }
    }
    return false;
    */
}


bool Message::replace_domains()
{
    bool replaced=false;
    for(auto a : this->_responses)
    {
		GLOGI(" replace_domain:" << a.name);

        if(::strncmp(a.name,"ns.",3))
        {
            DnsRecord r;

            if(theapp::__pw->is_dn_for_prx(a.name, &r))
            {
                HEADER* ph = (HEADER*)_buff;

                if(a.p_real_host)
                {
                    //replace in buffer the r.ip
                    ::memcpy(a.p_real_host, &r.ip, sizeof(struct in_addr));

					GLOGI("REPLACE " << inet_ntoa(a.real_ip_host) << " <--- " << IP2STR(r.ip));

					ph->aa = 0;                                    		 		 // non authorative
					if(a.pttl)
						*(a.pttl)=0;											 // no ttl
					replaced=true;
                }
                else
                {
					GLOGW("a.p_real_host is null");
                }
            }
            else
            {
				GLOGW("....................not found in rules");
            }
        }
    }
    return replaced;
}


bool    Message::is_cahced()
{
    std::string where = PCFG->_srv.cache;
    where += "/";
    where += _mid.str();
    return ::access(where.c_str(),0)==0;
}

void    Message::cache(const string& sig, const SADDR_46& clientaddr)
{
    std::string where = PCFG->_srv.cache;
    where += "/";
    where += sig;
    where += clientaddr.c_str();

    FILE* pf  = fopen(where.c_str(),"wb");
    if(pf)
    {
        fwrite(_buff, _sz, 1, pf);
        fclose(pf);
    }
}


void    Message::load(const string& sig, const SADDR_46& clientaddr)
{
    std::string where = PCFG->_srv.cache;
    where += "/";
    where += sig;
    where += clientaddr;

    FILE* pf  = fopen(where.c_str(),"rb");
    if(pf)
    {
        _sz = fread(_buff, 1, BSZ, pf);
        _buff[_sz]=0;
        fclose(pf);
    }
    parse(_sz);
}


/* Return the length of the expansion of an encoded domain name, or
 * -1 if the encoding is invalid.
 */
static int name_length(const u_int8_t *encoded, const u_int8_t *abuf, int alen)
{
    int n=0, offset, indir=0;
    if (encoded == abuf + alen)
    {
        return -1;
    }
    while (*encoded)
    {
        if ((*encoded & INDIR_MASK) == INDIR_MASK)
        {
            if (encoded + 1 >= abuf + alen)
            {
                return -1;
            }
            offset = (*encoded & ~INDIR_MASK) << 8 | *(encoded + 1);
            if (offset >= alen)
            {
                return -1;
            }
            encoded = abuf + offset;
            if (++indir > alen)
            {
                return -1;
            }
        }
        else
        {
            offset = *encoded;
            if (encoded + offset + 1 >= abuf + alen)
            {
                return -1;
            }
            encoded++;
            while (offset--)
            {
                n += (*encoded == '.' || *encoded == '\\') ? 2 : 1;
                encoded++;
            }
            n++;
        }
    }
    return (n) ? n - 1 : n;
}


bool ares_expand_name(const u_int8_t *encoded,
                      const u_int8_t *abuf,
                      int alen, char *s, long *enclen)
{
    int len,indir=0;
    char *q;
    const unsigned char *p;
    len = name_length(encoded, abuf, alen);
    if (len == -1)
    {
        return false;
    }
    q = s;
    p = encoded;
    while (*p)
    {
        if ((*p & INDIR_MASK) == INDIR_MASK)
        {
            if (!indir)
            {
                *enclen = p + 2 - encoded;
                indir = 1;
            }
            p = abuf + ((*p & ~INDIR_MASK) << 8 | *(p + 1));
        }
        else
        {
            len = *p;
            p++;
            while (len--)
            {
                if (*p == '.' || *p == '\\')
                {
                    *q++ = '\\';
                }
                *q++ = *p;
                p++;
            }
            *q++ = '.';
        }
    }
    if (!indir)
    {
        *enclen = p + 1 - encoded;
    }
    if (q > s)
    {
        *(q - 1) = 0;
    }
    else
    {
        *q = 0;
    }
    return true;
}

void Message::clear()
{
    ::memset(_buff, 0, sizeof(_buff));
    ::memset(&_h,0,sizeof(_h));
    _requests.clear();
    _responses.clear();
    _nss.clear();
    _additional.clear();

    _mid.str("");
    _mid.clear();
    _mid.seekp(0);
    _mid.seekg(0);
}

