#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_LENGTH 2048
#define PUB_EXP 65537
#define KEY_DIR "keys"

void replace_colons(char *dest, const char *src) {
    for (int i = 0; src[i]; i++)
        dest[i] = (src[i] == ':' ? '_' : src[i]);
    dest[strlen(src)] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <MAC_ADDRESS>\n", argv[0]);
        return 1;
    }

    const char *mac = argv[1];
    char mac_clean[64];
    replace_colons(mac_clean, mac);

    char priv_path[256], csr_path[256];
    snprintf(priv_path, sizeof(priv_path), "%s/private_key_%s.pem", KEY_DIR, mac_clean);
    snprintf(csr_path, sizeof(csr_path), "%s/csr_%s.pem", KEY_DIR, mac_clean);

    system("mkdir -p " KEY_DIR);

    // Generate key
    BIGNUM *bne = BN_new();
    BN_set_word(bne, PUB_EXP);
    RSA *rsa = RSA_new();
    RSA_generate_key_ex(rsa, KEY_LENGTH, bne, NULL);
    EVP_PKEY *pkey = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(pkey, rsa);

    // Save private key
    FILE *priv_fp = fopen(priv_path, "wb");
    PEM_write_PrivateKey(priv_fp, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(priv_fp);

    // Create CSR
    X509_REQ *req = X509_REQ_new();
    X509_REQ_set_pubkey(req, pkey);
    X509_NAME *name = X509_NAME_new();
    char cn[128];
    snprintf(cn, sizeof(cn), "Device_%s", mac);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)cn, -1, -1, 0);
    X509_REQ_set_subject_name(req, name);
    X509_REQ_sign(req, pkey, EVP_sha256());

    // Save CSR
    FILE *csr_fp = fopen(csr_path, "wb");
    PEM_write_X509_REQ(csr_fp, req);
    fclose(csr_fp);

    X509_REQ_free(req);
    X509_NAME_free(name);
    EVP_PKEY_free(pkey);
    BN_free(bne);

    printf("✅ Created:\n  🔐 %s\n  📝 %s\n", priv_path, csr_path);
    return 0;
}

