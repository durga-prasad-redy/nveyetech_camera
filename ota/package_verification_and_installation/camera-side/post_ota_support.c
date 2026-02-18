#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define DELETED_FILES        "/mnt/flash/vienna/deleted_files.txt"
#define EXTRA_FILE_TO_DELETE "/mnt/flash/generate_ota_package.sh"
#define EXTRA_FILE_TO_DELETE_1 "/mnt/flash/vienna/firmware/ota/fw_package.tar.gz"
#define EXTRA_FILE_TO_DELETE_2 "/mnt/flash/vienna/firmware/ota/manifest.json"
#define MAX_PATH_LENGTH      1024
#define BACKUP_DIR_PATH      "/mnt/flash/backup"

static int remove_directory_recursive(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    char full_path[MAX_PATH_LENGTH];

    while ((entry = readdir(dir)) != NULL) {
        // Skip '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat statbuf;
        if (stat(full_path, &statbuf) == -1) {
            perror("stat");
            closedir(dir);
            return -1;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Recursively delete subdirectory
            if (remove_directory_recursive(full_path) != 0) {
                closedir(dir);
                return -1;
            }
        } else {
            // Delete file
            if (remove(full_path) != 0) {
                perror("remove");
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);

    // Remove the now-empty directory
    if (rmdir(path) != 0) {
        perror("rmdir");
        return -1;
    }

    return 0;
}

// Generic function: check if dir exists, then delete
int delete_directory_if_exists(const char *dir_path) {
    struct stat statbuf;

    // Check if directory exists
    if (stat(dir_path, &statbuf) != 0) {
        if (errno == ENOENT) {
            printf("Directory does not exist: %s\n", dir_path);
            return 0; // No error if not found
        } else {
            perror("stat");
            return -1;
        }
    }

    if (!S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "Path exists but is not a directory: %s\n", dir_path);
        return -1;
    }

    printf("Directory exists. Deleting: %s\n", dir_path);
    return remove_directory_recursive(dir_path);
}

void delete_file_if_exists(const char *file_path) {
    if (access(file_path, F_OK) == 0) {
        printf("Deleting: %s\n", file_path);
        if (remove(file_path) != 0) {
            perror("Error deleting file");
        }
    } else {
        printf("File not found, skipping: %s\n", file_path);
    }
}

int main() {
    FILE *fp;
    char file_path[MAX_PATH_LENGTH];

    printf("[INFO] Checking and deleting extra file: %s\n", EXTRA_FILE_TO_DELETE);
    delete_file_if_exists(EXTRA_FILE_TO_DELETE);

    printf("[INFO] Checking and deleting extra file: %s\n", EXTRA_FILE_TO_DELETE_1);
    delete_file_if_exists(EXTRA_FILE_TO_DELETE_1);

    printf("[INFO] Checking and deleting extra file: %s\n", EXTRA_FILE_TO_DELETE_2);
    delete_file_if_exists(EXTRA_FILE_TO_DELETE_2);

    if (delete_directory_if_exists(BACKUP_DIR_PATH) == 0) {
        printf("Operation completed successfully. delete_directory_if_exists: %s\n", BACKUP_DIR_PATH);
    } else {
        printf("Operation failed. delete_directory_if_exists: %s\n", BACKUP_DIR_PATH);
    }

    fp = fopen(DELETED_FILES, "r");
    if (fp == NULL) {
        printf("[INFO] No deleted_files.txt found, skipping file deletion\n");
        return 0;
    }

    printf("[INFO] Deleting files listed in %s\n", DELETED_FILES);

    while (fgets(file_path, sizeof(file_path), fp) != NULL) {
        // Remove newline character if present
        size_t len = strlen(file_path);
        if (len > 0 && file_path[len - 1] == '\n') {
            file_path[len - 1] = '\0';
        }

        if (access(file_path, F_OK) == 0) {
            printf("Deleting: %s\n", file_path);
            if (remove(file_path) != 0) {
                perror("Error deleting file");
            }
        } else {
            printf("File not found, skipping: %s\n", file_path);
        }
    }

    fclose(fp);

    return 0;
}

