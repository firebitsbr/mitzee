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

#ifndef SSLCRYPT_H
#define SSLCRYPT_H

#include <string>

// Snatched from OpenSSL includes. I put the prototypes here to be independent
// from the OpenSSL source installation. Having this, mongoose + SSL can be
// built on any system with binary SSL libraries installed.
typedef struct ssl_st SSL;
typedef struct ssl_method_st SSL_METHOD;
typedef struct ssl_ctx_st SSL_CTX;
struct ssl_func {
  const char *name;   // SSL function name
  void  (*ptr)(void); // Function pointer
};

class SslCrypt
{
public:
    SslCrypt(const char* lib1, const char* lib2, int version);
    ~SslCrypt();
    bool init_server(const char* ,const char* ,const char* ,const char* );
    bool init_client(const char* ,const char* ,const char* );

    int load_dll(const char *dll_name, struct ssl_func *sw);
    SSL_CTX * accept_ctx()const{ return _pssl_accept;}
    SSL_CTX * connect_ctx()const{ return _pssl_connect;}
private:
    SSL_CTX *_pssl_accept;
    SSL_CTX *_pssl_connect;
    size_t _mutsize;
    int    _version;
};

extern struct ssl_func ssl_sw[];
std::string sslNerror(SSL* p=0, int* e=0, int* r=0);

#define SSL_ERROR_NONE          0
#define SSL_ERROR_SSL           1
#define SSL_ERROR_WANT_READ     2
#define SSL_ERROR_WANT_WRITE        3
#define SSL_ERROR_WANT_X509_LOOKUP  4
#define SSL_ERROR_SYSCALL       5 /* look at error stack/return value/errno */
#define SSL_ERROR_ZERO_RETURN       6
#define SSL_ERROR_WANT_CONNECT      7
#define SSL_ERROR_WANT_ACCEPT       8
#define SSL_OUT_OF_BOUNDS       9

#define SSL_free (* (void (*)(SSL *)) ssl_sw[0].ptr)
#define SSL_accept (* (int (*)(SSL *)) ssl_sw[1].ptr)
#define SSL_connect (* (int (*)(SSL *)) ssl_sw[2].ptr)
#define SSL_read (* (int (*)(SSL *, void *, int)) ssl_sw[3].ptr)
#define SSL_write (* (int (*)(SSL *, const void *,int)) ssl_sw[4].ptr)
#define SSL_get_error (* (int (*)(SSL *, int)) ssl_sw[5].ptr)
#define SSL_set_fd (* (int (*)(SSL *, SOCKET)) ssl_sw[6].ptr)
#define SSL_new (* (SSL * (*)(SSL_CTX *)) ssl_sw[7].ptr)
#define SSL_CTX_new (* (SSL_CTX * (*)(SSL_METHOD *)) ssl_sw[8].ptr)
#define SSLv23_server_method (* (SSL_METHOD * (*)(void)) ssl_sw[9].ptr)
#define SSL_library_init (* (int (*)(void)) ssl_sw[10].ptr)
#define SSL_CTX_use_PrivateKey_file (* (int (*)(SSL_CTX *, \
        const char *, int)) ssl_sw[11].ptr)
#define SSL_CTX_use_certificate_file (* (int (*)(SSL_CTX *, \
        const char *, int)) ssl_sw[12].ptr)
#define SSL_CTX_set_default_passwd_cb \
  (* (void (*)(SSL_CTX *, mg_callback_t)) ssl_sw[13].ptr)
#define SSL_CTX_free (* (void (*)(SSL_CTX *)) ssl_sw[14].ptr)
#define SSL_load_error_strings (* (void (*)(void)) ssl_sw[15].ptr)
#define SSL_CTX_use_certificate_chain_file \
  (* (int (*)(SSL_CTX *, const char *)) ssl_sw[16].ptr)
#define CRYPTO_num_locks (* (int (*)(void)) crypto_sw[0].ptr)
#define CRYPTO_set_locking_callback \
  (* (void (*)(void (*)(int, int, const char *, int))) crypto_sw[1].ptr)
#define CRYPTO_set_id_callback \
  (* (void (*)(unsigned long (*)(void))) crypto_sw[2].ptr)
#define ERR_get_error (* (unsigned long (*)(void)) crypto_sw[3].ptr)
#define ERR_error_string (* (char * (*)(unsigned long,char *)) crypto_sw[4].ptr)
#define SSLv23_client_method (* (SSL_METHOD * (*)(void)) ssl_sw[17].ptr)
#define SSL_CTX_set_cipher_list (* (int (*)(SSL_CTX *, const char *)) ssl_sw[18].ptr)
#define SSL_CTX_check_private_key (* (int (*)(SSL_CTX *)) ssl_sw[19].ptr)
#define SSL_shutdown (* (int (*)(SSL *)) ssl_sw[20].ptr)
#define SSL_pending (* (int (*)(SSL *)) ssl_sw[21].ptr)
//#define OpenSSL_add_ssl_algorithms (* (int(*)(void)) ssl_sw[22].ptr)
//#define SSLv3_server_method (* (SSL_METHOD * (*)(void))  ssl_sw[23].ptr)
//#define SSLv2_server_method (* (SSL_METHOD * (*)(void))  ssl_sw[24].ptr)
//#define SSLv3_client_method (* (SSL_METHOD * (*)(void))  ssl_sw[25].ptr)
//#define SSLv2_client_method (* (SSL_METHOD * (*)(void))  ssl_sw[26].ptr)

#endif // SSLCRYPT_H

