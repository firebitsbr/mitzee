/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - coinscode.com - N/A
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


#ifndef ERROR_STRINGS_H
#define ERROR_STRINGS_H

#include <consts.h>

#define NOAUTH      0x0
#define GSSAPI      0x1
#define AUTHREQ     0x2
#define IANA        0x3
#define NOMETHS     0xFF
#define SCONNECT    1
#define SBIND       2
#define SUDP        3
#define DN_IPV4     1
#define DN_DNAME    3
#define DN_IPV6     4

#define SUCCESS     0x00        //' succeeded
#define GENFAILURE  0x01     //' general SOCKS server failure
#define PROTOMISMATCH    0x02       //' connection not allowed by ruleset
#define NONETWORK   0x03      //' Network unreachable
#define NOHOST      0x04         //' Host unreachable
#define CONREFUSED  0x05     //' Connection refused
#define TTLERR      0x06         //' TTL expired
#define INVALIDCMD  0x07     //' Command not supported
#define BADADDRESS  0x08     //' Address type not supported
#define UNASSIGNED  0x09     //' to X'FF' unassigned

inline const char* socks_err(int err)
{
    static char* errors[64]={_CC("OK"),
                           _CC("general failure"),
                           _CC("expecting socks5 protocol"),
                           _CC("no network"),
                           _CC("no connection to host"),
                           _CC("connection refused"),
                           _CC("timeout"),
                           _CC("invalid command"),
                           _CC("bad address"),
                           _CC("unassigned")};
    return errors[err % 10];
}

#define CONTEX_BUFF_FULL            APP_ERRBASE+1
#define REMOTE_CLOSED_UNESPECTED    APP_ERRBASE+2
#define BLOCKED_HOST                APP_ERRBASE+3
#define CONNECTION_FAILED           APP_ERRBASE+4
#define INVALID_4_HEADER            APP_ERRBASE+5
#define CANNOT_CONNECT              APP_ERRBASE+6
#define IP_REJECTED                 APP_ERRBASE+7
#define REMOTE_CLOSED_ONSEND        APP_ERRBASE+8
#define INVALID_5_HEADER            APP_ERRBASE+9
#define NO_5_KNOWNMETHODS           APP_ERRBASE+10
#define INVALID_5_COMMAND           APP_ERRBASE+11
#define NOT_5_IMPLEMENTEDIPV6       APP_ERRBASE+12
#define CANNOT_PARSE_HTTP           APP_ERRBASE+13
#define CIRCULAR_LINK               APP_ERRBASE+14
#define OUT_OF_MEMORY               APP_ERRBASE+15
#define CONTEXT_DONE                APP_ERRBASE+16
#define CLIENT_CLOSED               APP_ERRBASE+17
#define CONNECT_TOUT                APP_ERRBASE+18
#define SSH_NO_HNAME                APP_ERRBASE+19
#define STREAMING_DENIED            APP_ERRBASE+20

#endif //ERROR_STRINGS_H


