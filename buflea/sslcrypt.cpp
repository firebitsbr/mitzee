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

#include <dlfcn.h>
#include "configprx.h"
#include "sslcrypt.h"


#define SSL_FILETYPE_PEM 1

#define CRYPTO_LOCK  1
static pthread_mutex_t* ssl_mutexes;

struct ssl_func ssl_sw[] =
{
    {"SSL_free",   0},                      //0
    {"SSL_accept",   0},
    {"SSL_connect",   0},
    {"SSL_read",   0},
    {"SSL_write",   0},
    {"SSL_get_error",  0},
    {"SSL_set_fd",   0},
    {"SSL_new",   0},
    {"SSL_CTX_new",   0},
    {"SSLv23_server_method", 0},
    {"SSL_library_init",  0},                   //10
    {"SSL_CTX_use_PrivateKey_file", 0},
    {"SSL_CTX_use_certificate_file",0},
    {"SSL_CTX_set_default_passwd_cb",0},
    {"SSL_CTX_free",  0},
    {"SSL_load_error_strings", 0},              //15
    {"SSL_CTX_use_certificate_chain_file", 0},
    {"SSLv23_client_method",0},
    {"SSL_CTX_set_cipher_list",0},
    {"SSL_CTX_check_private_key",0},
    {"SSL_shutdown",0},
    {0,    0}
};


static struct ssl_func crypto_sw[] =
{
    {"CRYPTO_num_locks",  0},
    {"CRYPTO_set_locking_callback", 0},
    {"CRYPTO_set_id_callback", 0},
    {"ERR_get_error",  0},
    {"ERR_error_string", 0},
    {0,    0}
};


std::string sslNerror(SSL* ctx)
{
    int             nerr = errno;
    long            err = ERR_get_error();
    std::string     strerr;
    char            fmt[256];

    strerr = "ERR_get_error(";
    while(err)
    {
        err = ERR_get_error();
        ::sprintf(fmt,"%ld, ", err);
        strerr += fmt;
    }
    strerr += "), SSL_get_error(";
    long sslerr = SSL_get_error(ctx,err);
    if(sslerr)
    {
        ERR_error_string(err, fmt);
        strerr += fmt;
    }
    sprintf(fmt, ") nerror(%d)", nerr);
    strerr += fmt;
    return strerr;
}

static void ssl_locking_callback(int mode, int mutex_num, kchar *file,
                                 int line)
{
    line = 0; // Unused
    file = 0; // Unused

    if (mode & CRYPTO_LOCK)
    {
        (void) pthread_mutex_lock(&ssl_mutexes[mutex_num]);
    }
    else
    {
        (void) pthread_mutex_unlock(&ssl_mutexes[mutex_num]);
    }
}

static unsigned long ssl_id_callback(void)
{
    return (unsigned long) pthread_self();
}

SslCrypt::SslCrypt():_psslCtx(0),_psslCtxCli(0)
{

    if (!load_dll(GCFG->_ssl.ssl_lib.c_str(), ssl_sw) ||
        !load_dll(GCFG->_ssl.crypto_lib.c_str(), crypto_sw))
    {
        // "sudo apt-get install libssl-dev"
        GLOGE("Cannot load SSL libraries...");
        exit(0);
    }

    SSL_library_init();
    SSL_load_error_strings();
}


bool SslCrypt::init_client()
{

    if ( (_psslCtxCli = SSL_CTX_new(SSLv23_client_method()))==0 )
    {
        GLOGE("SSL_CTX_new(SSLv23_client_method())" << sslNerror());
        return false;
    }

    // CLIENT CERT
    if(!GCFG->_ssl.cCert.empty())
    {
        kchar* cf = GCFG->_ssl.cCert.c_str();

        if(::access(cf,0)==0)
        {
            if(0==SSL_CTX_use_certificate_file(_psslCtx, cf, SSL_FILETYPE_PEM))
            {
                cout << "client:SSL_CTX_use_certificate_file" << cf <<" error: " << sslNerror();
                return false;
            }
        }
        else
        {
            GLOGW("File: " << cf << " not found");
        }
    }

    //CLIENT KEY
    if(!GCFG->_ssl.cPrivKey.empty())
    {
        kchar* pem = GCFG->_ssl.cPrivKey.c_str();
        if(::access(pem,0)==0)
        {
            if(0==SSL_CTX_use_PrivateKey_file(_psslCtx, pem, SSL_FILETYPE_PEM))
            {
                cout << "client:SSL_CTX_use_PrivateKey_file" << pem <<" error: " << sslNerror();
                return false;
            }
        }
        else
        {
            GLOGW("File: " << pem << " not found");
        }
    }
/*
    if ( !SSL_CTX_check_private_key(_psslCtx) )
    {
        cout << "CHECK PRIVATE KEY:" << sslNerror();
    }
*/

    return true;
}

bool SslCrypt::init_server()
{

    if ((_psslCtx = SSL_CTX_new(SSLv23_server_method())) == 0 )
    {
        GLOGE("SSL_CTX_new(SSLv23_server_method())" << sslNerror());
    }
    if(0 == _psslCtx)
    {
        GLOGE("SSL_CTX_new(SSLv23_server_method())" << sslNerror());
        return false;
    }

    // SERVER CERTIFICATE USE
    if(!GCFG->_ssl.sCert.empty())
    {
        kchar* pem = GCFG->_ssl.sCert.c_str();

        if(::access(pem,0)==0 )
        {
            if(0==SSL_CTX_use_certificate_file(_psslCtx, pem, SSL_FILETYPE_PEM))
            {
                cout << "server:SSL_CTX_use_certificate_file" << pem <<" error: " << sslNerror();
                return false;
            }
        }
        else
        {
            GLOGW("File: " << pem << " not found");
        }

    }

    // SERVER PRIVATE KEY
    if(!GCFG->_ssl.sPrivKey.empty())
    {
        kchar* pem = GCFG->_ssl.sPrivKey.c_str();
        if(::access(pem,0)==0)
        {
            if(0==SSL_CTX_use_PrivateKey_file(_psslCtx, pem, SSL_FILETYPE_PEM))
            {
                cout << "server:SSL_CTX_use_certificate_file" << pem <<" error: " << sslNerror();
                return false;
            }
            if (!SSL_CTX_check_private_key(_psslCtx))
            {
                 cout << "server:SSL_CTX_check_private_key" << pem <<" error: " << sslNerror();
                 return false;
            }

        }
        else
        {
            GLOGW("File: " << pem << " not found");
        }




    }

    if(!GCFG->_ssl.sChain.empty())
    {
        kchar* chain = GCFG->_ssl.sChain.c_str();
        if(::access(chain,0)==0)
        {

            if(0 ==SSL_CTX_use_certificate_chain_file(_psslCtx, chain))
            {
                cout << "server:SSL_CTX_use_certificate_chain_file" << chain <<" error: " << sslNerror();
                return false;
            }
            else
            {
                GLOGW("File: " << chain << " not found");
            }
        }
    }

    _mutsize = sizeof(pthread_mutex_t) * CRYPTO_num_locks();
    ssl_mutexes = (pthread_mutex_t *) malloc((size_t)_mutsize);
    if(ssl_mutexes)
    {
        for (int i = 0; i < CRYPTO_num_locks(); i++)
        {
            pthread_mutex_init(&ssl_mutexes[i], 0);
        }
    }

    CRYPTO_set_locking_callback(&ssl_locking_callback);
    CRYPTO_set_id_callback(&ssl_id_callback);
    return true;
}


int SslCrypt::load_dll(kchar *dll_name, struct ssl_func *sw)
{
    union
    {
        void *p;
        void (*fp)(void);
    } u;
    void  *dll_handle;
    struct ssl_func *fp;

    if ((dll_handle = dlopen(dll_name, RTLD_LAZY)) == 0)
    {
        cout << "Cannot open: " << dll_name;
        return 0;
    }

    for (fp = sw; fp->name != 0; fp++)
    {
        u.p = dlsym(dll_handle, fp->name);
        if (u.fp == 0)
        {
            cout << "Cannot load: " << fp->name;
            return 0;
        }
        else
        {
            fp->ptr = u.fp;
        }
    }

    return 1;
}

SslCrypt::~SslCrypt()
{
    if(_psslCtx)
    {
        SSL_CTX_free(_psslCtx);
    }

    if(_psslCtxCli)
    {
        SSL_CTX_free(_psslCtxCli);
    }
    if(ssl_mutexes)
    {
        for (int i = 0; i < CRYPTO_num_locks(); i++)
        {
            pthread_mutex_destroy(&ssl_mutexes[i]);
        }
        free(ssl_mutexes);
    }

}
