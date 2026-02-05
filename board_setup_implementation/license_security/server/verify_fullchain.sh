#!/bin/bash

# Usage:
#   ./verify_fullchain.sh <device_fullchain.crt> <rootCA.pem>

DEVICE_CHAIN="$1"
TRUSTED_ROOT="$2"

if [[ -z "$DEVICE_CHAIN" || -z "$TRUSTED_ROOT" ]]; then
    echo "Usage: $0 <device_fullchain.crt> <rootCA.pem>"
    exit 1
fi

echo "[+] Verifying $DEVICE_CHAIN against root CA $TRUSTED_ROOT..."

# Command:
openssl verify -CAfile "$TRUSTED_ROOT" "$DEVICE_CHAIN"

# Return value tells success or failure
if [[ $? -eq 0 ]]; then
    echo "[✓] Certificate chain is VALID and TRUSTED."
else
    echo "[✗] Certificate chain is INVALID or UNTRUSTED."
    exit 1
fi

