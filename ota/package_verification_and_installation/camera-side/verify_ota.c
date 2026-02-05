/**
 * @file verify_ota.c
 * @brief OTA package integrity and package verification.
 *
 * Provides mechanisms to validate the authenticity, integrity, and compatibility
 * of downloaded OTA packages before applying them.
 *
 * @author Rikin Shah
 * @date 2025-06-27
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <ctype.h>  /* required for isdigit */

#define DEBUG 0
#define LOG(fmt, ...) do { if (DEBUG) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while (0)
#define HEXDUMP(label, buf, len) do { \
    if (DEBUG) { \
        fprintf(stderr, "[DEBUG] %s (%zu bytes): ", label, len); \
        for (size_t i = 0; i < len; i++) fprintf(stderr, "%02x", (unsigned char)(buf[i])); \
        fprintf(stderr, "\n"); \
    } \
} while (0)

#define OTA_FILE "/mnt/flash/vienna/firmware/ota/fw_package.tar.gz"
#define MANIFEST_FILE "/mnt/flash/vienna/firmware/ota/manifest.json"
#define PUBLIC_KEY_FILE "/mnt/flash/vienna/firmware/ota/public.pem"
#define JFFS2_VERSION_FILE "/mnt/flash/jffs2_version"

typedef struct {
    int major;
    int minor;
    int patch;
    int valid;
} SemVer;

// Parse strict semver from start of string (vX.Y.Z or X.Y.Z)
static SemVer parse_semver(const char *s) {
    SemVer v = {0, 0, 0, 0};
    // Must have exactly 3 dot-separated integers
    if (sscanf(s, "v%d.%d.%d", &v.major, &v.minor, &v.patch) == 3 ||
        sscanf(s, "%d.%d.%d", &v.major, &v.minor, &v.patch) == 3) {
        v.valid = 1;
    }
    return v;
}

// Find first valid semver in the string
static SemVer find_semver_anywhere(const char *str) {
    for (const char *p = str; *p; p++) {
        if (*p == 'v' || isdigit((unsigned char)*p)) {
            SemVer v = parse_semver(p);
            if (v.valid) return v;
        }
    }
    return (SemVer){0,0,0,0};
}

// Extract cust_ base version (cust_XXX maps to major.minor.patch)
static SemVer parse_cust_base(const char *str) {
    const char *cust = strstr(str, "cust_");
    if (!cust) return (SemVer){0,0,0,0};
    cust += 5; // skip "cust_"

    int digits = 0;
    for (const char *p = cust; isdigit((unsigned char)*p); p++) digits++;

    // Must have exactly 3 digits (major*100 + minor*10 + patch)
    if (digits != 3) return (SemVer){0,0,0,0};

    int major = cust[0] - '0';
    int minor = cust[1] - '0';
    int patch = cust[2] - '0';

    return (SemVer){major, minor, patch, 1};
}

// Compare semver: return 1 if a>b, -1 if a<b, 0 if equal
static int compare_semver(SemVer a, SemVer b) {
    if (a.major != b.major) return (a.major > b.major) ? 1 : -1;
    if (a.minor != b.minor) return (a.minor > b.minor) ? 1 : -1;
    if (a.patch != b.patch) return (a.patch > b.patch) ? 1 : -1;
    return 0;
}

int is_version_upgrade_valid(const char *new_version, const char *current_version) {
    int new_is_cust = strstr(new_version, "cust_") != NULL;
    int cur_is_cust = strstr(current_version, "cust_") != NULL;

    // --- Parse base versions ---
    SemVer new_base = new_is_cust ? parse_cust_base(new_version) : find_semver_anywhere(new_version);
    SemVer cur_base = cur_is_cust ? parse_cust_base(current_version) : find_semver_anywhere(current_version);

    if (!new_base.valid || !cur_base.valid) {
        fprintf(stderr, "Invalid base version format.\n");
        return 0;
    }

    // --- Parse overlay versions ---
    SemVer new_overlay = {0,0,0,0};
    SemVer cur_overlay = {0,0,0,0};

    if (new_is_cust) {
        const char *plus = strchr(new_version, '+');
        if (plus && *(plus + 1)) {
            new_overlay = find_semver_anywhere(plus + 1);
        }
    }

    if (cur_is_cust) {
        const char *plus = strchr(current_version, '+');
        if (plus && *(plus + 1)) {
            cur_overlay = find_semver_anywhere(plus + 1);
        }
    }

    // --- Compare base versions ---
    int base_cmp = compare_semver(new_base, cur_base);
    if (base_cmp > 0) return 1;   // higher base → always valid
    if (base_cmp < 0) return 0;   // lower base → invalid

    // --- Base equal → overlay logic ---
    if (new_is_cust && !new_overlay.valid) {
        fprintf(stderr, "cust_ version missing valid overlay.\n");
        return 0;
    }
    if (cur_is_cust && !cur_overlay.valid) {
        fprintf(stderr, "cust_ version missing valid overlay.\n");
        return 0;
    }

    // If both have overlays → compare overlays
    if (new_overlay.valid && cur_overlay.valid) {
        return compare_semver(new_overlay, cur_overlay) > 0;
    }

    // If current has no overlay but new has → allow
    if (!cur_overlay.valid && new_overlay.valid) {
        return 1;
    }

    return 0;
}

unsigned char* base64_decode(const char *b64_input, size_t *out_len) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new_mem_buf((void*)b64_input, -1);
    bmem = BIO_push(b64, bmem);
    BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);
    size_t buf_size = strlen(b64_input);
    unsigned char *buffer = malloc(buf_size);
    *out_len = BIO_read(bmem, buffer, buf_size);
    BIO_free_all(bmem);
    LOG("Base64 decoded signature length: %zu", *out_len);
    return buffer;
}

int extract_json_field(const char *data, const char *key, char *output, size_t max_len) {
    char *found = strstr(data, key);
    if (!found) return -1;
    found = strchr(found, ':');
    if (!found) return -1;
    found += 1;
    while (*found == ' ' || *found == '\"') found++;
    char *end = strchr(found, '\"');
    if (!end) end = strchr(found, ',');
    if (!end) return -1;
    size_t len = end - found;
    if (len >= max_len) return -1;
    strncpy(output, found, len);
    output[len] = '\0';
    LOG("Extracted key '%s': %s", key, output);
    return 0;
}

int version_check(char *json) {
    char new_version[128];
    char compatible_version[128];

    // Extract OTA version
    if (extract_json_field(json, "version", new_version, sizeof(new_version))) {
        fprintf(stderr, "Failed to extract version from manifest\n");
        return 1;
    }

    // Extract compatible version
    if (extract_json_field(json, "compatible", compatible_version, sizeof(compatible_version))) {
        fprintf(stderr, "Failed to extract compatible version from manifest\n");
        return 1;
    }

    // Read current JFFS2 version
    FILE *vf = fopen(JFFS2_VERSION_FILE, "r");
    if (!vf) {
        perror("jffs2_version");
        return 1;
    }
    char cur_version[128] = {0};
    if (!fgets(cur_version, sizeof(cur_version), vf)) {
        fprintf(stderr, "Failed to read current version\n");
        fclose(vf);
        return 1;
    }
    fclose(vf);
    cur_version[strcspn(cur_version, "\r\n")] = 0;

    printf("[*] Current Version : %s\n", cur_version);
    printf("[*] OTA Version     : %s\n", new_version);
    printf("[*] Compatible Ver  : %s\n", compatible_version);

    // --- Check if current version matches compatible ---
    if (strcmp(cur_version, compatible_version) != 0) {
        fprintf(stderr, "[-] Current version %s is not compatible with OTA %s. Aborting.\n",
                cur_version, new_version);
        return 1;
    }

    // Commenting out this logic as we no longer need it...
    // now we are checking whether the current version matches the compatible
    // --- Check OTA version upgrade validity ---
    /*
    if (!is_version_upgrade_valid(new_version, cur_version)) {
        fprintf(stderr, "[-] OTA version is not newer than current version. Aborting.\n");
        return 1;
    }
    */

    printf("[+] Version check passed.\n");
    return 0;
}

int main() {
    FILE *fp = fopen(MANIFEST_FILE, "r");
    if (!fp) { perror("manifest"); return 1; }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);
    char *json = malloc(len + 1);
    if (!json) { perror("malloc"); fclose(fp); return 1; }
    if (fread(json, 1, len, fp) != len) {
        fprintf(stderr, "Failed to read manifest fully\n");
        free(json);
        fclose(fp);
        return 1;
    }
    json[len] = '\0';
    fclose(fp);
    LOG("Loaded manifest.json (%ld bytes)", len);

    if (version_check(json) != 0) {
        return 24;
    }

    char hash_hex[128], sig_b64[4096];
    if (extract_json_field(json, "hash", hash_hex, sizeof(hash_hex)) ||
        extract_json_field(json, "signature", sig_b64, sizeof(sig_b64))) {
        fprintf(stderr, "Failed to extract fields from manifest\n");
        return 1;
    }

    LOG("Computing SHA-256 hash of %s", OTA_FILE);
    fp = fopen(OTA_FILE, "rb");
    if (!fp) { perror("zip"); return 1; }
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    unsigned char buf[4096];
    int n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        SHA256_Update(&ctx, buf, n);
    fclose(fp);

    unsigned char zip_hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(zip_hash, &ctx);
    LOG("SHA-256 hash calculated from ZIP");
    HEXDUMP("ZIP SHA-256", zip_hash, SHA256_DIGEST_LENGTH);

    unsigned char manifest_hash[SHA256_DIGEST_LENGTH];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sscanf(&hash_hex[i * 2], "%2hhx", &manifest_hash[i]);
    HEXDUMP("Manifest hash", manifest_hash, SHA256_DIGEST_LENGTH);

    if (memcmp(zip_hash, manifest_hash, SHA256_DIGEST_LENGTH) != 0) {
        fprintf(stderr, "[-] Hash mismatch!\n");
        return 1;
    }
    printf("[+] Hash verified.\n");

    size_t sig_len;
    unsigned char *sig = base64_decode(sig_b64, &sig_len);
    HEXDUMP("Decoded signature", sig, sig_len);

    LOG("Loading public key from %s", PUBLIC_KEY_FILE);
    fp = fopen(PUBLIC_KEY_FILE, "r");
    if (!fp) { perror("pubkey"); return 1; }
    EVP_PKEY *pubkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);

    if (!pubkey) {
        fprintf(stderr, "[-] Error reading public key\n");
        return 1;
    }

    RSA *rsa = EVP_PKEY_get1_RSA(pubkey);
    if (!rsa) {
        fprintf(stderr, "[-] Could not get RSA from public key\n");
        EVP_PKEY_free(pubkey);
        return 1;
    }

    LOG("Verifying signature using RSA_verify...");
    int ret = RSA_verify(NID_sha256, zip_hash, SHA256_DIGEST_LENGTH, sig, sig_len, rsa);
    RSA_free(rsa);
    EVP_PKEY_free(pubkey);
    free(sig);
    free(json);

    if (ret == 1) {
        printf("[+] Signature verified. Firmware is authentic.\n");
        return 0;
    } else {
        fprintf(stderr, "[-] Signature invalid!\n");
        LOG("Signature verification failed: ret = %d", ret);
        return 1;
    }
}

