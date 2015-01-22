#!/bin/bash

CLOC="myCA"

if [[ ! -d "$CLOC" ]];then
    mkdir -p $CLOC
    mkdir -p $CLOC/csr
    mkdir -p $CLOC/newcerts
    mkdir -p $CLOC/certs
    mkdir -p $CLOC/private
    mkdir -p $CLOC/signedcerts
else
    rm -r "$CLOC"/*
    mkdir -p $CLOC/csr
    mkdir -p $CLOC/newcerts
    mkdir -p $CLOC/certs
    mkdir -p $CLOC/private
    mkdir -p $CLOC/signedcerts
fi

    cp certs.txt $CLOC/openssl.cnf



pushd $CLOC

    wd=$(pwd)
    sed -i "s,THIS__FOLDER,$wd,g" openssl.cnf
    echo 00 > serial
    echo 00 > crlnumber
    touch index.txt

    echo "creating certificate"

    ######################
    # Create CA private key#############################################################################################################################
    openssl genrsa -des3 -passout pass:mariusc -out  private/rootCA.key 2048
    # Remove passphrase
    openssl rsa -passin pass:mariusc -in private/rootCA.key -out private/rootCA.key
    # Create CA self-signed certificate
    openssl req -config openssl.cnf -new -x509 -subj '/C=DK/L=Aarhus/O=frogger CA/CN=theheat.dk' -days 732 -key private/rootCA.key -out certs/rootCA.crt
    ## openssl req -x509 -newkey rsa:2048 -out cacert.pem -outform PEM -days 1825

    #########################################
    # Create private key for the buflea server###########################################################################################################
    openssl genrsa -des3 -passout pass:mariusc -out private/buflea.key 2048
    # Remove passphrase
    openssl rsa -passin pass:mariusc -in private/buflea.key -out private/buflea.key

    # Create CSR for the buflea server
    openssl req -config openssl.cnf -new -subj '/C=DK/L=Aarhus/O=frogger/CN=buflea' -key private/buflea.key -out csr/buflea.csr
    # Create certificate for the buflea server
    openssl ca -batch -config openssl.cnf -days 732 -in csr/buflea.csr -out certs/buflea.crt -keyfile private/rootCA.key -cert certs/rootCA.crt -policy policy_anything

    #################################
    # Create private key for a client#######################################################################################################################
    openssl genrsa -des3 -passout pass:12345678 -out private/client.key 2048
    # Remove passphrase
    openssl rsa -passin pass:12345678 -in private/client.key -out private/client.key
    # Create CSR for the client.
    openssl req -config openssl.cnf -new -subj '/C=DK/L=Aarhus/O=frogger/CN=theClient' -key private/client.key -out csr/client.csr
    # Create client certificate.
    openssl ca -batch -config openssl.cnf -days 732 -in csr/client.csr -out certs/client.crt -keyfile private/rootCA.key -cert certs/rootCA.crt -policy policy_anything

    # Export the client certificate to pkcs12 for import in the browser#######################################################################################
    openssl pkcs12 -export -passout pass:qwerty -in certs/client.crt -inkey private/client.key -certfile certs/rootCA.crt -out certs/clientcert.p12

popd


#   SSLCertificateFile /u01/app/myCA/certs/winterfell.crt
#   SSLCertificateKeyFile /u01/app/myCA/private/winterfell.key
#   SSLCertificateChainFile /u01/app/myCA/certs/rootCA.crt
#   SSLCACertificateFile /u01/app/myCA/certs/rootCA.crt
#   SSLVerifyClient require
#   SSLVerifyDepth  10


# [ssl]
# {
#     ssl_lib                     = libssl.so
#     crypto_lib                  = libcrypto.so
#
#     srv_certificate_file        = buflea.crt
#     srv_certificate_key_file    = bufklea.key
#     srv_certificate_chain_file  = rootCA.crt
#     srv_ca_certificate_file     = rootCA.crt
#     cli_pkcs12_file             = clientcert.p12
#     cli_certificate_file        = client.crt
#     cli_csr_file                = client.csr#
#
# }

