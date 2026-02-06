#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define CONFIG_DIR "/mnt/flash/vienna/m5s_config"
#define SNAPSHOT_FILE "/mnt/flash/vienna/default_snapshot.txt"

#define FACTORY_BACKUP_DIR "/mnt/flash/vienna/factory_backup"
#define FACTORY_VIENNA_BACKUP FACTORY_BACKUP_DIR "/vienna"
#define FACTORY_ETC_BACKUP FACTORY_BACKUP_DIR "/etc"

#define FACTORY_RESET_STATUS_FILE \
    "/mnt/flash/vienna/m5s_config/factory_reset_status"
#define STATUS_IDLE        "idle"
#define STATUS_IN_PROGRESS "in-progress"
#define STATUS_FAIL        "fail"

int run_cmd(const char *cmd)
{
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Command failed (%d): %s\n", ret, cmd);
        return -1;
    }
    return 0;
}

// Write value to file
int write_value(const char *filename, const char *value)
{
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    fprintf(f, "%s\n", value);
    fclose(f);
    return 0;
}

static void update_factory_reset_status(const char *status)
{
    if (write_value(FACTORY_RESET_STATUS_FILE, status) != 0) {
        fprintf(stderr,
                "Warning: Failed to update factory reset status to %s\n",
                status);
    } else {
        printf("Factory reset status -> %s\n", status);
    }
    system("sync");
}

// ---------------------------------------------------------
// Load snapshot
// ---------------------------------------------------------
typedef struct {
    char key[128];
    char value[512];
} kv_t;

kv_t entries[256];
int entry_count = 0;

void load_snapshot()
{
    FILE *f = fopen(SNAPSHOT_FILE, "r");
    if (!f) {
        printf("Snapshot missing. Run device_setup to capture defaults.\n");
        exit(1);
    }

    char line[700];
    while (fgets(line, sizeof(line), f))
    {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;

        char *key = line;
        char *val = eq + 1;
	char *newline = strchr(val, '\n');
	if (newline)
		*newline = '\0';

        strncpy(entries[entry_count].key, key, sizeof(entries[entry_count].key));
        strncpy(entries[entry_count].value, val, sizeof(entries[entry_count].value));
        entry_count++;
    }

    fclose(f);
}

// ---------------------------------------------------------
// FACTORY RESET
// ---------------------------------------------------------
void camera_configuration_reset()
{
    load_snapshot();
    int i;
    for (i = 0; i < entry_count; i++)
    {
        char dst[256];
        snprintf(dst, sizeof(dst), "%s/%s", CONFIG_DIR, entries[i].key);
        write_value(dst, entries[i].value);
        printf("Restored %s\n", entries[i].key);
    }

    system("sync");
    printf("Factory reset complete.\n");
}

// ---------------------------------------------------------
// FACTORY RESET
// ---------------------------------------------------------
void camera_factory_reset()
{
    printf("Starting camera factory reset...\n");

    update_factory_reset_status(STATUS_IDLE);

    // 1. Validate backup folder
    DIR *test = opendir(FACTORY_BACKUP_DIR);
    if (!test) {
        printf("Factory backup folder missing! Aborting.\n");
        update_factory_reset_status(STATUS_FAIL);
        return;
    }
    closedir(test);

    // 2. Mark in-progress
    update_factory_reset_status(STATUS_IN_PROGRESS);

    // 3. Restore vienna
    printf("Restoring /mnt/flash/vienna from TAR...\n");
    if (run_cmd(
            "tar -xpf /mnt/flash/vienna/factory_backup/vienna.tar "
            "-C /mnt/flash") != 0)
    {
        update_factory_reset_status(STATUS_FAIL);
        return;
    }

    // 4. Restore etc
    printf("Restoring /mnt/flash/etc from TAR...\n");
    if (run_cmd(
            "tar -xpf /mnt/flash/vienna/factory_backup/etc.tar "
            "-C /mnt/flash") != 0)
    {
        update_factory_reset_status(STATUS_FAIL);
        return;
    }

    // 5. Sync filesystem
    system("sync");

    // 6. Back to idle (success)
    update_factory_reset_status(STATUS_IDLE);
    printf("Camera factory reset complete.\n");
}

// ---------------------------------------------------------
int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage:\n");
        printf("  factory_reset_bin camera_configuration_reset\n");
        printf("  factory_reset_bin camera_factory_reset\n");
        return 0;
    }

    if (strcmp(argv[1], "camera_configuration_reset") == 0) {
        camera_configuration_reset();
    }
    else if (strcmp(argv[1], "camera_factory_reset") == 0) {
        camera_factory_reset();
    }
    else {
        printf("Unknown command.\n");
    }
    return 0;
}

