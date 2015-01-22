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



#include <assert.h>
#include <iostream>
#include <sstream>
#include <sock.h>
#include <strutils.h>
#include "main.h"
#include "config.h"
#include "context.h"
#include <consts.h>
#include <matypes.h>
#include "threadpool.h"
#include "ctxthread.h"
#include "listeners.h"
#include "contextreg.h"
#include "dnsthread.h"


/*
    PRIVATE SERVER AUTH PROTOCOL
*/

/*
    PHP authentication page comes on this request
*/
//-----------------------------------------------------------------------------
CtsReg::CtsReg(const ConfPrx::Ports* pconf, tcp_xxx_sock& s):Ctx(pconf, s)
{
    _tc= 'A';
    _mode=P_CONTORL; //dns ssh
    LOGI("ACL connect from:" << IP2STR(_cliip));
    _pcall = (PFCLL)&CtsReg::_pend;
}

//-----------------------------------------------------------------------------
CtsReg::~CtsReg()
{
    //dtor
}

void CtsReg::send_exception(const char* desc)
{

}

int CtsReg::_s_send_reply(u_int8_t code, const char* info)
{
    Ctx::_s_send_reply( code, info);
    return 0;
};

int  CtsReg::_close()
{
    destroy();
    return 0;
}

CALLR  CtsReg::_pend()
{
    int  lr = _rec_some();
    size_t  nfs = _hdr.bytes();
    if(lr > 5 || nfs > 5) //&& lr==0/*have bytes and remove closed*/)
    {
        if(!_postprocess())
        {
            return R_KILL;
        }
    }
    _hdr.clear();
    return R_CONTINUE;
}

bool  CtsReg::_postprocess()
{
    // GET /R?PUB:x.x.x.x,PRIV:y.y.y.y HTTP 1.1
    //
    // add tto payers
    GLOGI(_hdr.buf());
    return false;
}


bool CtsReg::_new_request(const u_int8_t* buff, int bytes)
{
    return false;
}


/*
# Create CA private key
openssl genrsa -des3 -passout pass:qwerty -out  private/rootCA.key 2048

# Remove passphrase
openssl rsa -passin pass:qwerty -in private/rootCA.key -out private/rootCA.key

# Create CA self-signed certificate
openssl req -config openssl.cnf -new -x509 -subj '/C=DK/L=Aarhus/O=frogger CA/CN=theheat.dk' -days 999 -key private/rootCA.key -out certs/rootCA.crt


SERVER

# Create private key for the winterfell server
openssl genrsa -des3 -passout pass:qwerty -out private/winterfell.key 2048

# Remove passphrase
openssl rsa -passin pass:qwerty -in private/winterfell.key -out private/winterfell.key

# Create CSR for the winterfell server
openssl req -config openssl.cnf -new -subj '/C=DK/L=Aarhus/O=frogger/CN=winterfell' -key private/winterfell.key -out csr/winterfell.csr

# Create certificate for the winterfell server
openssl ca -batch -config openssl.cnf -days 999 -in csr/winterfell.csr -out certs/winterfell.crt -keyfile private/rootCA.key -cert certs/rootCA.crt -policy policy_anything


CLIENT


# Create private key for a client
openssl genrsa -des3 -passout pass:qwerty -out private/client.key 2048

# Remove passphrase
openssl rsa -passin pass:qwerty -in private/client.key -out private/client.key

# Create CSR for the client.
openssl req -config openssl.cnf -new -subj '/C=DK/L=Aarhus/O=frogger/CN=theClient' -key private/client.key -out csr/client.csr

# Create client certificate.
openssl ca -batch -config openssl.cnf -days 999 -in csr/client.csr -out certs/client.crt -keyfile private/rootCA.key -cert certs/rootCA.crt -policy policy_anything


# Export the client certificate to pkcs12 for import in the browser
openssl pkcs12 -export -passout pass:qwerty -in certs/client.crt -inkey private/client.key -certfile certs/rootCA.crt -out certs/clientcert.p12

APACHE?

SSLCertificateFile /u01/app/myCA/certs/winterfell.crt
SSLCertificateKeyFile /u01/app/myCA/private/winterfell.key
SSLCertificateChainFile /u01/app/myCA/certs/rootCA.crt
SSLCACertificateFile /u01/app/myCA/certs/rootCA.crt
SSLVerifyClient require
SSLVerifyDepth  10





*/

