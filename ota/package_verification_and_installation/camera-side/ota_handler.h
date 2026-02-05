/**
 * @file ota_handler.h
 * @brief Header for OTA management functions and definitions.
 *
 * Declares interfaces and constants for managing end-to-end OTA (Over-The-Air) update processes.
 *
 * @author Rikin Shah
 * @date 2025-06-27
 */

#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>

#define OTA_DIR         "/mnt/flash/vienna/firmware/ota"
#define OTA_TAR         OTA_DIR "/fw_package.tar.gz"
#define OTA_MANIFEST    OTA_DIR "/manifest.json"
#define VERIFY_BIN      OTA_DIR "/verify_ota"
#define VIENNA_DIR      "/mnt/flash"
#define BACKUP_DIR      VIENNA_DIR "/backup"
#define MAX_RETRIES     3
#define RETRY_DELAY_SEC 1
#define TMP_FILE_LIST   "/mnt/flash/ota_file_list.txt"
#define MAX_OTA_SIZE    2300000  // ~2.2 MB

// Full package; this will have OTA_MANIFEST and OTA_TAR (This implementation is done for the ease of web-developer.
// Doing this will help them to send only one file[ota.tar.gz] instead of two[fw_package.tar.gz and manifest.json].)
#define FULL_PACKAGE_TAR_PATH          OTA_DIR "/ota.tar.gz"
#define FULL_PACKAGE_EXTRACTION_PATH   OTA_DIR

typedef enum {
    e_SIZE_64   = 64,
    e_SIZE_128  = 128,
    e_SIZE_256  = 256,
    e_SIZE_512  = 512,
    e_SIZE_1024 = 1024
} e_BUFFER_SIZE;

/*
Error Code Structure
10–19 → Download
20–29 → Verification
30–39 → Installation
80–89 → Rollback
90    → Success
91–99 → Other/internal/in-progress
*/
typedef enum {
    e_OTA_SUCCESS               = 0,
    e_OTA_SUCCESSFULLY_DONE     = 90,

    e_OTA_ERR_VERIFY_FAILED     = 20,
    e_OTA_ERR_FILELIST_FAILED   = 30,
    e_OTA_ERR_BACKUP_FAILED     = 31,
    e_OTA_ERR_EXTRACTION_FAILED = 32,
    e_OTA_ERR_UPDATE_FAILED     = 33,
    e_OTA_ERR_EXTR_ROLL_FAILED  = 35,
    e_OTA_ERR_ROLLBACK_FAILED   = 80,
    e_OTA_INPROGRESS            = 91,
    e_OTA_ALREADY_INPROGRESS    = 92,
    e_OTA_ERR_SIZE_LIMIT        = 21,
    e_OTA_ERR_MANIFEST_PARSE    = 22,
    e_OTA_ERR_MANIFEST_OPEN     = 23,
    e_OTA_ERR_VERSION_MISMATCH  = 24,
    e_OTA_ERR_FULL_PKG_EXT_FAIL = 34,
    e_OTA_ERR_INVALID_HASH_FRMT = 35,
    e_OTA_ERR_RENAME_FAIL       = 36,
    e_OTA_ERR_FILE_NOT_FOUND    = 37,
    e_OTA_ERR_HASH_MISMATCH     = 38,
    e_OTA_ERR_HASH_FAILED       = 391,
    e_OTA_ERR_STAT_FAILED       = 35,
    e_OTA_ERR_INTERNAL          = 99
} e_OTA_RESULT;

// Function declarations
void log_msg(const char *msg);
int run_command(const char *desc, const char *cmd);
int reject_large_update(void);
int verify_package(void);
int extract_file_list_from_tar(void);
int backup_files_from_list(void);
int extract_tar_package(void);
int run_updated_component(void);
int rollback_partial(void);
int clean_ota_temp_files(void);
int is_ota_in_progress(void);
int set_config_file_var(const char *var, const char *val);
const char* ota_result_to_status_str(e_OTA_RESULT result);
int set_ota_status_env_from_result(e_OTA_RESULT result);
int extract_ota_archive();
int verify_and_extract_ota_archive();

#endif // OTA_HANDLER_H

