#!/bin/bash

# generate_certificates.sh - Script to generate SSL/TLS certificates

CERT_DIR="./ssl_certificates"
DAYS_VALID=365

echo "Generating SSL/TLS certificates..."

# Create certificate directory if it doesn't exist
mkdir -p $CERT_DIR

# Generate private key
openssl genrsa -out $CERT_DIR/private.key 2048

# Generate certificate signing request (CSR)
openssl req -new -key $CERT_DIR/private.key -out $CERT_DIR/request.csr \
    -subj "/C=US/ST=State/L=City/O=Organization/OU=Unit/CN=yourdomain.com"

# Generate self-signed certificate
openssl x509 -req -days $DAYS_VALID -in $CERT_DIR/request.csr -signkey $CERT_DIR/private.key -out $CERT_DIR/certificate.crt

echo "Certificates generated in $CERT_DIR."
