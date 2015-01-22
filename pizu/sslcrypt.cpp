/**
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
#include "config.h"
#include "sslcrypt.h"


#define SSL_ERROR_WANT_READ 2
#define SSL_ERROR_WANT_WRITE 3
#define SSL_FILETYPE_PEM 1
#define CRYPTO_LOCK  1
static pthread_mutex_t* ssl_mutexes;



struct ssl_func ssl_sw[] = {
    {"SSL_free",   0},
    {"SSL_accept",   0},
    {"SSL_connect",   0},
    {"SSL_read",   0},
    {"SSL_write",   0},
    {"SSL_get_error",  0},
    {"SSL_set_fd",   0},
    {"SSL_new",   0},
    {"SSL_CTX_new",   0},
    {"SSLv23_server_method", 0},
    {"SSL_library_init",  0},
    {"SSL_CTX_use_PrivateKey_file", 0},
    {"SSL_CTX_use_certificate_file",0},
    {"SSL_CTX_set_default_passwd_cb",0},
    {"SSL_CTX_free",  0},
    {"SSL_load_error_strings", 0},
    {"SSL_CTX_use_certificate_chain_file", 0},
    {0,    0}
};


static struct ssl_func crypto_sw[] = {
    {"CRYPTO_num_locks",  0},
    {"CRYPTO_set_locking_callback", 0},
    {"CRYPTO_set_id_callback", 0},
    {"ERR_get_error",  0},
    {"ERR_error_string", 0},
    {0,    0}
};


kchar *sslNerror(void)
{
    unsigned long err;
    err = ERR_get_error();
    if(err) {
        kchar *pe =ERR_error_string(err, 0);
        return pe;
    }
    return "";

}

static void ssl_locking_callback(int mode, int mutex_num, kchar *file,
                                 int line)
{
    if (mode & CRYPTO_LOCK) {
        (void) pthread_mutex_lock(&ssl_mutexes[mutex_num]);
    } else {
        (void) pthread_mutex_unlock(&ssl_mutexes[mutex_num]);
    }
}

static unsigned long ssl_id_callback(void)
{
    return (unsigned long) pthread_self();
}


SslCrypt::SslCrypt():_psslCtx(0)
{

}


bool SslCrypt::init()
{
    if (!load_dll(GCFG->_ssl.ssl_lib.c_str(), ssl_sw) ||
            !load_dll(GCFG->_ssl.crypto_lib.c_str(), crypto_sw)) {
        return false;
    }

    SSL_library_init();
    SSL_load_error_strings();

    if ((_psslCtx = SSL_CTX_new(SSLv23_server_method())) == 0) {
        printf("%s,%d %s\n", __FILE__ ,__LINE__, sslNerror());
    }
    if(_psslCtx && !GCFG->_ssl.certificate.empty()) {
        kchar* pem = GCFG->_ssl.certificate.c_str();
        if(0==SSL_CTX_use_certificate_file(_psslCtx, pem, SSL_FILETYPE_PEM))
            printf("%s,%d  %s\n", __FILE__ ,__LINE__,sslNerror());
        if(0==SSL_CTX_use_PrivateKey_file(_psslCtx, pem,SSL_FILETYPE_PEM))
            printf("%s,%d  %s\n", __FILE__ ,__LINE__,sslNerror());
    }
    if(_psslCtx && !GCFG->_ssl.chain.empty()) {
        kchar* chain = GCFG->_ssl.certificate.c_str();
        if(0 ==SSL_CTX_use_certificate_chain_file(_psslCtx, chain))
            printf("%s,%d  %s\n", __FILE__ ,__LINE__,sslNerror());
    }

    _mutsize = sizeof(pthread_mutex_t) * CRYPTO_num_locks();
    ssl_mutexes = (pthread_mutex_t *) malloc((size_t)_mutsize);
    if(ssl_mutexes) {
        for (int i = 0; i < CRYPTO_num_locks(); i++) {
            pthread_mutex_init(&ssl_mutexes[i], 0);
        }
    }

    CRYPTO_set_locking_callback(&ssl_locking_callback);
    CRYPTO_set_id_callback(&ssl_id_callback);
    return true;
}


int SslCrypt::load_dll(kchar *dll_name, struct ssl_func *sw)
{

    union {
        void *p;
        void (*fp)(void);
    } u;
    void  *dll_handle;
    struct ssl_func *fp;

    if ((dll_handle = dlopen(dll_name, RTLD_LAZY)) == 0) {
        printf("Cannot open %s \n", dll_name);
        return 0;
    }

    for (fp = sw; fp->name != 0; fp++) {
        u.p = dlsym(dll_handle, fp->name);
        if (u.fp == 0) {
            printf("Cannot loaf function  %s \n", fp->name);
            return 0;
        } else {
            fp->ptr = u.fp;
        }
    }
    return 1;
}

SslCrypt::~SslCrypt()
{
    if(_psslCtx){
        SSL_CTX_free(_psslCtx);
    }
    if(ssl_mutexes) {
        for (int i = 0; i < CRYPTO_num_locks(); i++) {
            pthread_mutex_destroy(&ssl_mutexes[i]);
        }
        free(ssl_mutexes);
    }
}
