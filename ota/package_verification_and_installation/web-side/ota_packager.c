#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <errno.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#define SECRET_STRING ""
#define INPUT_FILE "ota.tar.gz"
#define FIRMWARE_FILE "fw_package.tar.gz"
#define MANIFEST_FILE "manifest.json"

#define FW_PACKAGE "fw_package.tar.gz"
#define PRIVATE_KEY_PATH "../certs/private.pem"
#define SIGNATURE_BIN "signature.bin"
#define MANIFEST_PATH "manifest.json"

// Helper to compute SHA256 hash as hex string
int sha256sum(const char *filename, char *out_hex) {
    FILE *file = fopen(filename, "rb");
    if (!file) return -1;

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    unsigned char buffer[8192];
    size_t read_len;
    while ((read_len = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        SHA256_Update(&ctx, buffer, read_len);
    }
    fclose(file);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        sprintf(out_hex + i * 2, "%02x", hash[i]);

    return 0;
}

// Signs a file using RSA and writes binary signature
int sign_with_private_key(const char *data_path, const char *key_path, const char *sig_path) {

    FILE *key_file = NULL;

    FILE *data_file = NULL;

    FILE *sig_file = NULL;

    EVP_PKEY *pkey = NULL;

    EVP_MD_CTX *md_ctx = NULL;

    unsigned char *sig = NULL;

    int ret = -1;
    
    key_file = fopen(key_path, "r");
    if (!key_file) goto cleanup;

    pkey = PEM_read_PrivateKey(key_file, NULL, NULL, NULL);
    if (!pkey) goto cleanup;

    data_file = fopen(data_path, "rb");
    if (!data_file) goto cleanup;

    md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        goto cleanup;
    }

    if (EVP_DigestSignInit(md_ctx, NULL, EVP_sha256(), NULL, pkey) != 1) {
        goto cleanup;
    }

    unsigned char buffer[8192];
    size_t len;
    while ((len = fread(buffer, 1, sizeof(buffer), data_file)) > 0) {
        if (EVP_DigestSignUpdate(md_ctx, buffer, len) != 1) goto cleanup;
    }

    size_t sig_len;
    EVP_DigestSignFinal(md_ctx, NULL, &sig_len);
    sig = malloc(sig_len);

    if (EVP_DigestSignFinal(md_ctx, sig, &sig_len) != 1) {
        goto cleanup;
    }

    sig_file = fopen(sig_path, "wb");
    fwrite(sig, 1, sig_len, sig_file);

    ret = 0;
cleanup:

    if (sig_file) fclose(sig_file);

    if (data_file) fclose(data_file);

    if (key_file) fclose(key_file);

    if (md_ctx) EVP_MD_CTX_free(md_ctx);

    if (pkey) EVP_PKEY_free(pkey);

    if (sig) free(sig);

    return ret;
}

// Base64 encodes a binary file
char *base64_encode_file(const char *file_path) {
    FILE *f = fopen(file_path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *data = malloc(len);
    if (fread(data, 1, len, f) != (size_t)len) {
        fprintf(stderr, "Error reading file\n");
        free(data);
        fclose(f);
        return NULL;
    }
    fclose(f);

    BIO *bmem = BIO_new(BIO_s_mem());
    BIO *b64 = BIO_new(BIO_f_base64());
    b64 = BIO_push(b64, bmem);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // No line breaks
    BIO_write(b64, data, len);
    BIO_flush(b64);

    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);

    char *encoded = malloc(bptr->length + 1);
    memcpy(encoded, bptr->data, bptr->length);
    encoded[bptr->length] = '\0';

    BIO_free_all(b64);
    free(data);

    return encoded;
}

int generate_manifest() {
    struct stat st;
    if (stat(FW_PACKAGE, &st) != 0) {
        perror("Error: firmware package not found");
        return -1;
    }
    long size = st.st_size;

    char hash_hex[65] = {0};
    if (sha256sum(FW_PACKAGE, hash_hex) != 0) {
        fprintf(stderr, "Failed to hash firmware\n");
        return -1;
    }

    if (sign_with_private_key(FW_PACKAGE, PRIVATE_KEY_PATH, SIGNATURE_BIN) != 0) {
        fprintf(stderr, "Signature generation failed\n");
        return -1;
    }

    char *sig_b64 = base64_encode_file(SIGNATURE_BIN);
    if (!sig_b64) {
        fprintf(stderr, "Failed to base64 encode signature\n");
        return -1;
    }

    // --- Get version from fw_package.tar.gz ---
    char version[128] = {0};
    FILE *fp = popen("tar -xOzf fw_package.tar.gz jffs2_version", "r");
    if (!fp) {
        perror("Error reading version from package");
        free(sig_b64);
        return -1;
    }
    if (!fgets(version, sizeof(version), fp)) {
        fprintf(stderr, "Failed to read version from tar\n");
        pclose(fp);
        free(sig_b64);
        return -1;
    }
    pclose(fp);
    version[strcspn(version, "\r\n")] = 0;  // Remove trailing newline

    // --- Get compatible version ---
    char compatible[128] = {0};
    FILE *cf = fopen("compatible_on", "r");
    if (cf) {
        if (fgets(compatible, sizeof(compatible), cf) != NULL) {
            compatible[strcspn(compatible, "\r\n")] = 0;  // Remove newline
        }
        fclose(cf);
    } else {
        fprintf(stderr, "Warning: compatible_on file not found, leaving empty\n");
    }

    // --- Write manifest.json ---
    FILE *mf = fopen(MANIFEST_PATH, "w");
    if (!mf) {
        perror("Error creating manifest.json");
        free(sig_b64);
        return -1;
    }

    fprintf(mf,
        "{\n"
        "  \"version\": \"%s\",\n"
        "  \"compatible\": \"%s\",\n"
        "  \"filename\": \"%s\",\n"
        "  \"size\": %ld,\n"
        "  \"hash\": \"%s\",\n"
        "  \"signature\": \"%s\"\n"
        "}\n",
        version, compatible, FW_PACKAGE, size, hash_hex, sig_b64
    );

    fclose(mf);
    free(sig_b64);
    printf("[+] Manifest written to %s\n", MANIFEST_PATH);
    return 0;
}

int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

int create_ota_tar() {
    if (!file_exists(FIRMWARE_FILE)) {
        fprintf(stderr, "Error: %s not found!\n", FIRMWARE_FILE);
        return -1;
    }

    printf("Creating OTA package...\n");
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "tar -czf %s %s %s", INPUT_FILE, FIRMWARE_FILE, MANIFEST_FILE);
    if (system(cmd) != 0) {
        fprintf(stderr, "Error: Failed to create ota.tar.gz!\n");
        return -1;
    }

    printf("OTA package created: %s\n", INPUT_FILE);
    return 0;
}

int rename_ota_with_sha() {
    struct stat st;
    if (stat(INPUT_FILE, &st) != 0) {
        perror("Error: Cannot stat ota.tar.gz");
        return -1;
    }

    long long size = (long long)st.st_size;

    char input_string[128];
    snprintf(input_string, sizeof(input_string), "%s%lld", SECRET_STRING, size);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)input_string, strlen(input_string), hash);

    char hash_hex[65];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(&hash_hex[i * 2], "%02x", hash[i]);
    }

    char new_filename[256];
    snprintf(new_filename, sizeof(new_filename), "%s.%s", INPUT_FILE, hash_hex);

    if (rename(INPUT_FILE, new_filename) != 0) {
        perror("Error renaming OTA file");
        return -1;
    }

    printf("Final OTA package: %s\n", new_filename);
    return 0;
}

int main() {
    if (generate_manifest() != 0) {
        fprintf(stderr, "Error generating manifest\n");
        return 1;
    }

    if (create_ota_tar() != 0) {
        return 1;
    }

    if (rename_ota_with_sha() != 0) {
        return 1;
    }

    return 0;
}

