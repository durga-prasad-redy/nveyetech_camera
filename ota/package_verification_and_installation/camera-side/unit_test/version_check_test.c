#include <stdio.h>
#include <string.h>

#include <ctype.h>

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

// Include your is_version_upgrade_valid() implementation here
// -----------------------------------------------------------
/* paste the entire is_version_upgrade_valid() code from above here */
// -----------------------------------------------------------

static void run_test(const char *new_v, const char *cur_v, int expected) {
    int result = is_version_upgrade_valid(new_v, cur_v);
    printf("[TEST] New: %-20s  Current: %-20s  Expected: %d  Got: %d  [%s]\n",
           new_v, cur_v, expected, result, (result == expected) ? "PASS" : "FAIL");
}

int main(void) {
    printf("=== Running version upgrade tests ===\n\n");

    // Basic semver upgrades/downgrades
    run_test("v1.0.1", "v1.0.0", 1);  // patch upgrade
    run_test("v1.1.0", "v1.0.9", 1);  // minor upgrade
    run_test("v2.0.0", "v1.9.9", 1);  // major upgrade
    run_test("v1.0.0", "v1.0.0", 0);  // equal
    run_test("v1.0.0", "v1.0.1", 0);  // patch downgrade

    // cust_ upgrades based on base version
    run_test("cust_436+v1.0.0", "v4.3.6", 1);  // base equal, overlay present
    run_test("cust_437+v1.0.0", "cust_436+v9.9.9", 1);  // base upgrade
    run_test("cust_436+v1.1.0", "cust_436+v1.0.0", 1);  // overlay upgrade
    run_test("cust_436+v1.0.0", "cust_436+v1.1.0", 0);  // overlay downgrade

    // Moving from cust_ to base
    run_test("v4.3.7", "cust_436+v1.9.9", 1);  // base upgrade even if losing overlay
    run_test("v4.3.6", "cust_436+v1.9.9", 0);  // base equal, overlay missing → reject

    // Moving from base to cust_
    run_test("cust_536+v1.0.0", "v5.3.6", 1);  // base equal, overlay present
    run_test("cust_536+v0.9.9", "v5.3.6", 1);  // base equal, overlay present (strictly allowed, base wins)
    
    // Format errors
    run_test("cust_53+v1.0.0", "v5.3.6", 0);   // invalid cust_ base format
    run_test("cust_536+", "v5.3.6", 0);        // missing overlay semver
    run_test("cust_536+v1", "v5.3.6", 0);      // incomplete semver

    printf("\n=== Test run complete ===\n");
    return 0;
}

