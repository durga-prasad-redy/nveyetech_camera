/**
 * @file ota_service.c
 * @brief OTA service manager.
 *
 * Implements the service and responsible for monitoring and executing OTA updates.
 *
 * @author Rikin Shah
 * @date 2025-06-27
 */

#include "ota_handler.h"

#define COMPATIBLE_VERSION_MISMATCH 24

static inline int ota_return_with_status(e_OTA_RESULT result) {
    int status = 0;
    status = set_ota_status_env_from_result(result);
    return status;
}

int main() {
    log_msg("=== OTA Update Initiated ===");

    if (is_ota_in_progress()) {
        log_msg("OTA already in progress. Exiting.");
        return e_OTA_ALREADY_INPROGRESS;
    }

    int status = e_OTA_SUCCESS;
    if( (status = set_ota_status_env_from_result(e_OTA_INPROGRESS)) != e_OTA_SUCCESS) {
        return ota_return_with_status(status);
    }

    if ( (status = verify_and_extract_ota_archive()) != e_OTA_SUCCESS) {
        return ota_return_with_status(status);
    }

    if ( (status = reject_large_update()) != e_OTA_SUCCESS) {
        log_msg("OTA rejected due to size constraint.");
        return ota_return_with_status(status);
    }

    if ( (status = verify_package()) != e_OTA_SUCCESS) {
        log_msg("OTA package verification failed or version compatibility issue. Aborting.");
        if (status == COMPATIBLE_VERSION_MISMATCH)
            return ota_return_with_status(e_OTA_ERR_VERSION_MISMATCH);
        else
            return ota_return_with_status(e_OTA_ERR_VERIFY_FAILED);
    }

    if (extract_file_list_from_tar() != e_OTA_SUCCESS) {
        log_msg("Failed to extract file list from tar. Aborting.");
        return ota_return_with_status(e_OTA_ERR_FILELIST_FAILED);
    }

    run_command("Cleaning previous backup...", "rm -rf " BACKUP_DIR);
    if (backup_files_from_list() != e_OTA_SUCCESS) {
        log_msg("Backup failed. Aborting.");
        return ota_return_with_status(e_OTA_ERR_BACKUP_FAILED);
    }

    if (extract_tar_package() != e_OTA_SUCCESS) {
        log_msg("Extraction failed. Attempting rollback...");
        if (rollback_partial() == e_OTA_SUCCESS) {
            log_msg("Rollback successful.");
            clean_ota_temp_files();
            return ota_return_with_status(e_OTA_ERR_EXTRACTION_FAILED);
        } else {
            return ota_return_with_status(e_OTA_ERR_EXTR_ROLL_FAILED);
        }
    }

    int success = e_OTA_SUCCESS;
    for (int i = 1; i <= MAX_RETRIES; ++i) {
        log_msg("Verifying updated firmware (attempt)...");

        if (run_updated_component() == e_OTA_SUCCESS) {
            log_msg("Update verification successful.");
            success = 1;
            break;
        } else {
            char msg[64];
            snprintf(msg, sizeof(msg), "Retry %d failed. Waiting %ds...", i, RETRY_DELAY_SEC);
            log_msg(msg);
            sleep(RETRY_DELAY_SEC);
        }
    }

    if (!success) {
        log_msg("All verification retries failed. Rolling back...");
        if (rollback_partial() == e_OTA_SUCCESS) {
            log_msg("Rollback successful.");
            clean_ota_temp_files();
            return ota_return_with_status(e_OTA_ERR_UPDATE_FAILED);
        } else {
            log_msg("Rollback failed. Manual recovery required.");
            return ota_return_with_status(e_OTA_ERR_ROLLBACK_FAILED);
        }
    }

    log_msg("OTA Update Complete.");
    clean_ota_temp_files();
    return ota_return_with_status(e_OTA_SUCCESSFULLY_DONE);
}

