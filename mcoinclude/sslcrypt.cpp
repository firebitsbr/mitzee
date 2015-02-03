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
#include <os.h>
#include <sslcrypt.h>
#include <config.h>


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
    {"SSL_pending",0},
//    {"OpenSSL_add_ssl_algorithms",0},
//    {"SSLv3_server_method",0},
//    {"SSLv2_server_method",0},
//    {"SSLv3_client_method",0},
//    {"SSLv2_client_method",0},
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

static const char* __sslstr[32]={
                "SSL_ERROR_NONE",
                "SSL_ERROR_SSL",
                "SSL_ERROR_WANT_READ",
                "SSL_ERROR_WANT_WRITE",
                "SSL_ERROR_WANT_X509_LOOKUP",
                "SSL_ERROR_SYSCALL",
                "SSL_ERROR_ZERO_RETURN",
                "SSL_ERROR_WANT_CONNECT",
                "SSL_ERROR_WANT_ACCEPT",
                "OUT_OF_BOUNDS",
};


std::string sslNerror(SSL* ctx, int* serr, int* nerro)
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
        if(serr)*serr=sslerr;
        ERR_error_string(sslerr, fmt);
        strerr += fmt;
        strerr += ",";
        strerr += __sslstr[sslerr <= SSL_ERROR_WANT_ACCEPT ? sslerr :  SSL_OUT_OF_BOUNDS];
    }
    if(nerro)*nerro=nerr;
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

SslCrypt::SslCrypt(const char* lib1, const char* lib2, int version):_pssl_accept(0),_pssl_connect(0),_version(version)
{

    if (!load_dll(lib1, ssl_sw) ||
        !load_dll(lib2, crypto_sw))
    {
        // "sudo apt-get install libssl-dev"
        GLOGE("Cannot load SSL libraries...");
        exit(0);
    }

    SSL_library_init();
    SSL_load_error_strings();
   // OpenSSL_add_ssl_algorithms();
}

bool SslCrypt::init_server(const char* cert,
                          const char* key,
                          const char* clicacert, const char* chaincert)
{

    if(_version == 23)
        _pssl_accept = SSL_CTX_new(SSLv23_server_method());
  //  else if(_version == 2)
  //      _pssl_accept = SSL_CTX_new(SSLv2_server_method());
  //  else if(_version == 3)
  //      _pssl_accept = SSL_CTX_new(SSLv3_server_method());
    else{
        GLOGE("SSL_CTX_new("<<_version<<"_server_method())" << sslNerror());
        return false;
    }

    if(0 == _pssl_accept)
    {
        GLOGE("SSL_CTX_new(SSLv23_server_method())" << sslNerror());
        return false;
    }
    if(cert)
    {
        if(::access(cert,0)==0 )
        {
            if(0==SSL_CTX_use_certificate_file(_pssl_accept, cert, SSL_FILETYPE_PEM))
            {
                cout << "server:SSL_CTX_use_certificate_file" << cert <<" error: " << sslNerror();
                return false;
            }
        }
        else
        {
            GLOGW("File: " << cert << " not found");
        }
    }

    if(key)
    {
        if(::access(key,0)==0)
        {
            if(0==SSL_CTX_use_PrivateKey_file(_pssl_accept, key, SSL_FILETYPE_PEM))
            {
                cout << "server:SSL_CTX_use_certificate_file" << key <<" error: " << sslNerror();
                return false;
            }
        /*
            if (!SSL_CTX_check_private_key(_pssl_accept))
            {
                 cout << "server:SSL_CTX_check_private_key" << pem <<" error: " << sslNerror();
                 return false;
            }
    */
        }
        else
        {
            GLOGW("File: " << key << " not found");
        }
    }

    //SSL_CTX_load_and_set_client_CA_file(SSL_CTX *ctx, const char *file)
    if(chaincert)
    {
        if(::access(chaincert,0)==0)
        {
            if(0 ==SSL_CTX_use_certificate_chain_file(_pssl_accept, chaincert))
            {
                cout << "server:SSL_CTX_use_certificate_chain_file" << chaincert <<" error: " << sslNerror();
                return false;
            }
            else
            {
                GLOGW("File: " << chaincert << " not found");
            }
        }
        else
        {
            GLOGW("File: " << chaincert << " not found");
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



bool SslCrypt::init_client(const char* cert,const char* key,const char* cacert)
{

    if(_version == 23)
        _pssl_connect = SSL_CTX_new(SSLv23_client_method());
   // if(_version == 2)
    //    _pssl_connect = SSL_CTX_new(SSLv2_client_method());
   // if(_version == 3)
   //     _pssl_connect = SSL_CTX_new(SSLv3_client_method());
    else{
        GLOGE("SSL version: '" << _version << "' No known!.")
        return false;
    }


    if ( _pssl_connect ==0 )
    {
        GLOGE("SSL_CTX_new("<<_version<<"_client_method())" << sslNerror());
        return false;
    }

    if(cert)
    {
        if(::access(cert,0)==0)
        {
            if(0==SSL_CTX_use_certificate_file(_pssl_connect, cert, SSL_FILETYPE_PEM))
            {
                cout << "client:SSL_CTX_use_certificate_file" << cert <<" error: " << sslNerror();
                return false;
            }
        }
        else
        {
            GLOGW("File: " << cert << " not found");
        }
    }

    if(key)
    {
        if(::access(key,0)==0)
        {
            if(0==SSL_CTX_use_PrivateKey_file(_pssl_connect, key, SSL_FILETYPE_PEM))
            {
                cout << "client:SSL_CTX_use_PrivateKey_file" << key <<" error: " << sslNerror();
                return false;
            }
        }
        else
        {
            GLOGW("File: " << key << " not found");
        }
    }
/*
    if ( !SSL_CTX_check_private_key(_pssl_connect) )
    {
        cout << "CHECK PRIVATE KEY:" << sslNerror();
    }
*/

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
            GLOGE("Cannot load function: " << fp->name);
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
    if(_pssl_accept)
    {
        SSL_CTX_free(_pssl_accept);
    }

    if(_pssl_connect)
    {
        SSL_CTX_free(_pssl_connect);
    }
//    ERR_free_strings();
    if(ssl_mutexes)
    {
        for (int i = 0; i < CRYPTO_num_locks(); i++)
        {
            pthread_mutex_destroy(&ssl_mutexes[i]);
        }
        free(ssl_mutexes);
    }

}
