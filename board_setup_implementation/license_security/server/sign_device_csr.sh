#!/bin/bash
set -e

# Root CA folder can be set via environment variable or default to ../rootCerts/certs
ROOT_CA_DIR="${ROOT_CA_DIR:-../rootCerts/certs}"
ROOT_CA_KEY="$ROOT_CA_DIR/rootCA.key"
ROOT_CA_CERT="$ROOT_CA_DIR/rootCA.pem"

# Output certificate filenames
DEVICE_CERT="device.crt"
DEVICE_CHAIN_CERT="device_fullchain.crt"
DAYS_VALID=365

# Input: Device CSR
if [ $# -ne 1 ]; then
    echo "Usage: $0 <path-to-device.csr>"
    echo "Hint: Optionally set ROOT_CA_DIR env variable to override default path."
    exit 1
fi

CSR_FILE="$1"

# Check required files
if [ ! -f "$CSR_FILE" ]; then
    echo "[-] CSR file not found: $CSR_FILE"
    exit 1
fi

if [ ! -f "$ROOT_CA_KEY" ] || [ ! -f "$ROOT_CA_CERT" ]; then
    echo "[-] Root CA files not found in: $ROOT_CA_DIR"
    exit 1
fi

echo "[+] Signing CSR: $CSR_FILE"
openssl x509 -req -in "$CSR_FILE" \
    -CA "$ROOT_CA_CERT" -CAkey "$ROOT_CA_KEY" -CAcreateserial \
    -out "$DEVICE_CERT" -days "$DAYS_VALID" -sha256

echo "[+] Certificate created: $DEVICE_CERT"

# Create fullchain certificate (device cert + root cert)
cat "$DEVICE_CERT" "$ROOT_CA_CERT" > "$DEVICE_CHAIN_CERT"
echo "[+] Full chain created: $DEVICE_CHAIN_CERT"

