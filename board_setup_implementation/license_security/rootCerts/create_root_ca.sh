#!/bin/bash
set -e

mkdir -p certs
cd certs

CA_KEY="rootCA.key"
CA_CERT="rootCA.pem"

echo "🔐 Generating Root CA private key..."
openssl genrsa -aes256 -out $CA_KEY 4096

echo "📜 Creating Root CA certificate..."
openssl req -x509 -new -key $CA_KEY -sha256 -days 3650 -out $CA_CERT \
  -subj "/C=IN/ST=Karnataka/L=Bangalore/O=Outdu/CN=OutduRootCA"

echo "✅ Root CA created in certs/:"
echo "  🔑 $CA_KEY (KEEP SAFE!)"
echo "  📜 $CA_CERT (Trusted by server/devices)"

