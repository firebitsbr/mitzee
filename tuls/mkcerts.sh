#!/bin/bash

rm -rfv OUT
mkdir OUT

cp openssl.cnf OUT/
pushd OUT

touch certindex.txt        # index file.
mkdir newcerts         # new certs dir
echo 22 > serial 
echo 00 > crlnumber     
mkdir private    
mkdir csr
mkdir certs

EBITS=2048

function crea_key()
{
	NAME=$1
	openssl genrsa -des3 -passout pass:12345678 -out ./private/$NAME $EBITS			# CA.key
	openssl rsa -passin pass:12345678 -in ./private/$NAME -out ./private/$NAME		# remove passout from CA.key
}

function crea_csr()
{
	KEY=$1
	CSR=$2
	SUBJ=$3
	openssl req -config ./openssl.cnf -new -subj "$SUBJ" -key ./private/$KEY -out ./csr/$CSR 										
}

function crea_cert()
{
	RKEY=$1	
	RCRT=$2
	CSR=$3
	CERT=$4
	openssl ca -batch -config ./openssl.cnf -days 740 -in csr/$CSR -out certs/$CERT -keyfile private/$RKEY -cert certs/$RCRT -policy policy_anything # SRV certificate
}

function crea_cert_autosign()
{
	SUBJ=$1	
	PKEY=$2
	CERT=$3
	openssl req -config ./openssl.cnf -new -x509 -subj "$SUBJ" -days 740 -key ./private/$PKEY -out ./certs/$CERT # self sign CA certificate
}


echo "=====================================CA-----------------------------------"
##  CA key and self signed certificate   private/CA.key  certs/CA.crt
crea_key "RCA.key"
crea_cert_autosign "/C=CA/L=rhill/O=coinscode CA/CN=screen.webhop" "RCA.key" "RCA.crt" 


echo "=====================================SRV----------------------------------"
## SRV SSL certificate
crea_key "SRV.key"
crea_csr "SRV.key"  "SRV.csr" "/C=CA/L=rhill/O=coinscode/CN=screen.webhop"
echo "------------------------------------------------------------------------"
crea_cert "RCA.key" "RCA.crt" "SRV.csr" "SRV.crt"



## CLIc certificate
echo "=====================================CLI----------------------------------"
crea_key "CLI.key"
crea_csr "CLI.key" "CLI.csr" "/C=CA/L=rhill/O=coinscode/CN=screen.webhop.CLI"
crea_cert "RCA.key" "RCA.crt" "CLI.csr" "CLI.crt"

#openssl pkcs12 -export -passout pass:12345678 -in certs/CLI.crt -inkey private/CLI.key -certfile certs/RCA.crt -out certs/CLIcert.p12 # export cli cert to p12

# SSLCertificateFile myCA/certs/SRV.crt
# SSLCertificateKeyFile myCA/private/winterfell.key
# SSLCertificateChainFile myCA/certs/RCA.crt
# SSLCACertificateFile myCA/certs/RCA.crt

popd
