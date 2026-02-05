#!/bin/bash
set -e

mkdir -p ../certs
cd ../certs

# Generate 2048-bit RSA private key
openssl genpkey -algorithm RSA -out private.pem -pkeyopt rsa_keygen_bits:2048

# Extract public key
openssl rsa -in private.pem -pubout -out public.pem

echo "[+] RSA key pair generated in certs/"

