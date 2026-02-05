/**
 * @file ota_handler.c
 * @brief Implementation of core OTA update logic.
 *
 * Handles the complete OTA workflow including preparation, download, validation, and triggering updates.
 *
 * @author Rikin Shah
 * @date 2025-06-27
 */

#include "ota_handler.h"

#define CMD_SIZE            e_SIZE_256
#define VALUE_SIZE          e_SIZE_128
#define OTA_STATUS_ENV_VAR  "ota_status"
#define CONFIG_DIR          "/mnt/flash/vienna/m5s_config"

void log_msg(const char *msg) {
    time_t now = time(NULL);
    char buf[32];
    strftime(buf, sizeof(buf), "%F %T", localtime(&now));
    printf("[%s] %s\n", buf, msg);
}

int reject_large_update() {
    struct stat st;
    if (stat(OTA_TAR, &st) != e_OTA_SUCCESS) {
        perror("stat OTA_TAR");
        return e_OTA_ERR_MANIFEST_OPEN;
    }

    long size = st.st_size;
    if (size > MAX_OTA_SIZE) {
        char msg[e_SIZE_128];
        snprintf(msg, sizeof(msg), "OTA file size %ld exceeds max allowed %d. Rejecting OTA.", size, MAX_OTA_SIZE);
        log_msg(msg);
        return e_OTA_ERR_SIZE_LIMIT;
    }

    char msg[e_SIZE_128];
    snprintf(msg, sizeof(msg), "OTA file size %ld verified under limit %d.", size, MAX_OTA_SIZE);
    log_msg(msg);
    return e_OTA_SUCCESS;
}

int run_command(const char *desc, const char *cmd) {
    log_msg(desc);
    log_msg(cmd);
    int status = system(cmd);
    if (status == -1) {
        perror("system()");
        return e_OTA_ERR_INTERNAL;
    }
    int code = WEXITSTATUS(status);
    if (code != e_OTA_SUCCESS) {
        char err[e_SIZE_128];
        snprintf(err, sizeof(err), "Command failed with exit code %d", code);
        log_msg(err);
    }
    return code;
}

int verify_package() {
    char cmd[e_SIZE_256];
    snprintf(cmd, sizeof(cmd), "LD_LIBRARY_PATH=%s/vienna/lib %s", VIENNA_DIR, VERIFY_BIN);
    return run_command("Verifying OTA package...", cmd);
}

int extract_file_list_from_tar() {
    char cmd[e_SIZE_512];
    snprintf(cmd, sizeof(cmd), "gzip -dc %s | tar -tf - | grep -v '/$' > %s", OTA_TAR, TMP_FILE_LIST);
    return run_command("Extracting file list from OTA package...", cmd);
}

int backup_files_from_list() {
    FILE *fp = fopen(TMP_FILE_LIST, "r");
    if (!fp) {
        perror("fopen (TMP_FILE_LIST)");
        return e_OTA_ERR_BACKUP_FAILED;
    }

    char path[e_SIZE_512];
    while (fgets(path, sizeof(path), fp)) {
        path[strcspn(path, "\n")] = 0;
        if (strlen(path) == 0) continue;

        char src[e_SIZE_512], dst[e_SIZE_512], mkdir_cmd[e_SIZE_512];
        snprintf(src, sizeof(src), "%s/%s", VIENNA_DIR, path);
        snprintf(dst, sizeof(dst), "%s/%s", BACKUP_DIR, path);

        // Ensure backup directory exists
        char dst_dir[e_SIZE_512];
        strncpy(dst_dir, dst, sizeof(dst_dir));
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", dirname(dst_dir));
        run_command("Creating backup directory...", mkdir_cmd);

        char cp_cmd[e_SIZE_1024];
        snprintf(cp_cmd, sizeof(cp_cmd), "cp -a \"%s\" \"%s\" 2>/dev/null", src, dst);
        run_command("Backing up file...", cp_cmd);
    }

    fclose(fp);
    return e_OTA_SUCCESS;
}

int extract_tar_package() {
    char cmd[e_SIZE_512];
    snprintf(cmd, sizeof(cmd), "gzip -dc %s | tar -xf - -C %s", OTA_TAR, VIENNA_DIR);
    return run_command("Extracting OTA package contents...", cmd);
}

int run_updated_component() {
    // [Developer Note]
    // This function should be implemented to verify the correctness of the critical updated firmware components if any.
    // For example, check whether key binaries, services, or scripts introduced by the OTA are running as expected.
    // If verification passes, return 0 (e_OTA_SUCCESS). If not, return an error code to trigger rollback.
    //
    // This is critical to ensure system integrity — OTA should only be marked successful
    // if the updated components are functional in the target runtime environment.
    return e_OTA_SUCCESS;
}

int rollback_partial() {
    FILE *fp = fopen(TMP_FILE_LIST, "r");
    if (!fp) {
        perror("fopen (TMP_FILE_LIST for rollback)");
        return e_OTA_ERR_ROLLBACK_FAILED;
    }

    char path[e_SIZE_512];
    while (fgets(path, sizeof(path), fp)) {
        path[strcspn(path, "\n")] = 0;
        if (strlen(path) == 0) continue;

        char src[e_SIZE_512], dst[e_SIZE_512];
        snprintf(src, sizeof(src), "%s/%s", BACKUP_DIR, path);
        snprintf(dst, sizeof(dst), "%s/%s", VIENNA_DIR, path);

        char cp_cmd[e_SIZE_1024];
        snprintf(cp_cmd, sizeof(cp_cmd), "cp -a \"%s\" \"%s\" 2>/dev/null", src, dst);
        run_command("Restoring backup file...", cp_cmd);
    }

    fclose(fp);
    return e_OTA_SUCCESS;
}

int clean_ota_temp_files() {
    char cmd[e_SIZE_256];
    snprintf(cmd, sizeof(cmd), "rm -f %s/fw_package.tar.gz %s/manifest.json %s", OTA_DIR, OTA_DIR, TMP_FILE_LIST);
    return run_command("Cleaning up OTA temporary files...", cmd);
}

int is_ota_in_progress() {
    FILE *fp = popen("ps | grep ota_service | grep -v grep", "r");
    if (!fp) {
        perror("popen");
        return 0;
    }

    char line[e_SIZE_256];
    int count = 0;
    pid_t self_pid = getpid();

    while (fgets(line, sizeof(line), fp)) {
        int pid;
        if (sscanf(line, "%d", &pid) == 1) {
            if (pid != self_pid) {
                count++;
            }
        }
    }

    pclose(fp);
    return count > 0;
}

int set_config_file_var(const char *var, const char *val) {
    char path[CMD_SIZE];
    FILE *fp;

    // Create the full path to the variable file
    snprintf(path, sizeof(path), CONFIG_DIR "/%s", var);

    // Write value to the file
    fp = fopen(path, "w");
    if (!fp) {
        perror("Failed to open config file for writing");
        return e_OTA_ERR_INTERNAL;
    }

    if (fprintf(fp, "%s\n", val) < 0) {
        perror("Failed to write value to config file");
        fclose(fp);
        return e_OTA_ERR_INTERNAL;
    }

    fclose(fp);

    // Verify the content
    char read_back[VALUE_SIZE] = {0};
    fp = fopen(path, "r");
    if (!fp) {
        perror("Failed to reopen config file for verification");
        return e_OTA_ERR_INTERNAL;
    }

    if (!fgets(read_back, sizeof(read_back), fp)) {
        perror("Failed to read back value");
        fclose(fp);
        return e_OTA_ERR_INTERNAL;
    }
    fclose(fp);

    // Remove trailing newline if present
    read_back[strcspn(read_back, "\n")] = 0;

    if (strcmp(read_back, val) != 0) {
        fprintf(stderr, "Verification failed: Expected '%s', Got '%s'\n", val, read_back);
        return e_OTA_ERR_INTERNAL;
    }

    printf("Config variable '%s' set to '%s' successfully.\n", var, val);
    return e_OTA_SUCCESS;
}

const char* ota_result_to_status_str(e_OTA_RESULT result) {
    static char status_str[e_SIZE_64];

    const char* status;
    switch (result) {
        case e_OTA_SUCCESS:               status = "ota-successful";        break;
        case e_OTA_SUCCESSFULLY_DONE:     status = "ota-successful";        break;
        case e_OTA_ERR_VERIFY_FAILED:     status = "verification-fail";     break;
        case e_OTA_ERR_VERSION_MISMATCH:  status = "compatible-mismatch";   break;
        case e_OTA_ERR_FILELIST_FAILED:   status = "installation-fail";     break;
        case e_OTA_ERR_BACKUP_FAILED:     status = "installation-fail";     break;
        case e_OTA_ERR_EXTRACTION_FAILED: status = "installation-fail";     break;
        case e_OTA_ERR_EXTR_ROLL_FAILED:  status = "installation-fail";     break;
        case e_OTA_ERR_UPDATE_FAILED:     status = "installation-fail";     break;
        case e_OTA_ERR_ROLLBACK_FAILED:   status = "rollback-fail";         break;
        case e_OTA_INPROGRESS:            status = "in-progress";           break;
        case e_OTA_ALREADY_INPROGRESS:    status = "already-in-progress";   break;
        case e_OTA_ERR_SIZE_LIMIT:        status = "verification-fail";     break;
        case e_OTA_ERR_MANIFEST_PARSE:    status = "verification-fail";     break;
        case e_OTA_ERR_MANIFEST_OPEN:     status = "verification-fail";     break;
        case e_OTA_ERR_FULL_PKG_EXT_FAIL: status = "extraction-fail";       break;
        case e_OTA_ERR_INTERNAL:          status = "ota-internal-err";      break;
        default:                          status = "unknown";               break;
    }

    snprintf(status_str, sizeof(status_str), "%s-%d", status, result);
    return status_str;
}

int set_ota_status_env_from_result(e_OTA_RESULT result) {
    const char* str = ota_result_to_status_str(result);
    return set_config_file_var(OTA_STATUS_ENV_VAR, str);
}

int verify_and_extract_ota_archive() {
    char ota_dir[] = OTA_DIR;
    char hash_from_filename[e_SIZE_128];
    char computed_hash[e_SIZE_128];
    char cmd[e_SIZE_512];
    char full_path[e_SIZE_512];
    FILE *fp;
    struct stat st;
    size_t size;

    const char *secret = "";

    // Step 1: Find the OTA file
    snprintf(cmd, sizeof(cmd), "ls %s/ota.tar.gz.*", ota_dir);
    fp = popen(cmd, "r");
    if (!fp || !fgets(full_path, sizeof(full_path), fp)) {
        log_msg("OTA file not found.");
        return e_OTA_ERR_FILE_NOT_FOUND;
    }
    pclose(fp);
    strtok(full_path, "\n");  // Remove newline

    // Step 2: Extract hash from filename
    char *dot = strrchr(full_path, '.');
    if (!dot || strlen(dot + 1) != 64) {
        log_msg("Invalid OTA filename format.");
        return e_OTA_ERR_INVALID_HASH_FRMT;
    }
    strncpy(hash_from_filename, dot + 1, sizeof(hash_from_filename));

    // Step 3: Get file size
    if (stat(full_path, &st) != 0) {
        log_msg("Failed to stat OTA file.");
        return e_OTA_ERR_STAT_FAILED;
    }
    size = st.st_size;

    // Step 4: Compute SHA256(secret + size)
    snprintf(cmd, sizeof(cmd), "echo -n \"%s%zu\" | sha256sum", secret, size);
    fp = popen(cmd, "r");
    if (!fp || !fgets(computed_hash, sizeof(computed_hash), fp)) {
        log_msg("Failed to compute hash.");
        return e_OTA_ERR_HASH_FAILED;
    }
    pclose(fp);
    strtok(computed_hash, " ");  // Remove trailing content

    // Step 5: Compare
    if (strncmp(hash_from_filename, computed_hash, 64) != 0) {
        log_msg("OTA hash mismatch. Aborting.");
        return e_OTA_ERR_HASH_MISMATCH;
    }
    log_msg("OTA file verified successfully.");

    // Step 6: Rename to ota.tar.gz
    snprintf(cmd, sizeof(cmd), "mv %s %s/ota.tar.gz", full_path, ota_dir);
    if (system(cmd) != 0) {
        log_msg("Failed to rename OTA file.");
        return e_OTA_ERR_RENAME_FAIL;
    }

    // Step 7: Extract it
    return extract_ota_archive();
}

int extract_ota_archive() {
    char cmd[e_SIZE_256];

    snprintf(cmd, sizeof(cmd), "mkdir -p %s", FULL_PACKAGE_EXTRACTION_PATH);
    if (system(cmd) != e_OTA_SUCCESS) {
        log_msg("Warning: Failed to create the OTA extraction directory.");
        return e_OTA_ERR_FULL_PKG_EXT_FAIL;
    }

    snprintf(cmd, sizeof(cmd), "gzip -dc %s | tar -xf - -C %s", FULL_PACKAGE_TAR_PATH, FULL_PACKAGE_EXTRACTION_PATH);
    if (system(cmd) != e_OTA_SUCCESS) {
        log_msg("Failed to extract ota.tar.gz archive.");
        return e_OTA_ERR_FULL_PKG_EXT_FAIL;
    }

    log_msg("Extracted ota.tar.gz successfully.");

    snprintf(cmd, sizeof(cmd), "rm -f %s", FULL_PACKAGE_TAR_PATH);
    if (system(cmd) != e_OTA_SUCCESS) {
        log_msg("Warning: Failed to delete ota.tar.gz after extraction.");
        // Not a hard failure — continue
    } else {
        log_msg("Deleted ota.tar.gz after successful extraction.");
    }

    return e_OTA_SUCCESS;
}

