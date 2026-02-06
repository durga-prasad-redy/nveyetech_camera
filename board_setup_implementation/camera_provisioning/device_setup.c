/**
 * @file        device_setup.c
 * @author      Rikin Shah
 * @date        2025-10-27
 * @brief       Program to set MAC, serial number, mfg date and hotspot_ssid.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <strings.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define FACTORY_BACKUP_DIR "/mnt/flash/vienna/factory_backup"
#define SRC_ROOT "/mnt/flash/vienna"
#define SRC_ETC "/mnt/flash/etc"

#define SERIAL_FILE      "/mnt/flash/vienna/m5s_config/serial_number"
#define MFG_FILE         "/mnt/flash/vienna/m5s_config/mfg_date"
#define HOTSPOT_FILE     "/mnt/flash/vienna/m5s_config/hotspot_ssid"
#define DEVICE_SETUP_BIN "/mnt/flash/vienna/firmware/board_setup/device_setup"

#define CONFIG_DIR "/mnt/flash/vienna/m5s_config"
#define SNAPSHOT_FILE "/mnt/flash/vienna/default_snapshot.txt"

void capture_defaults()
{
    DIR *dp = opendir(CONFIG_DIR);
    if (!dp) {
        printf("Config directory missing.\n");
        return;
    }

    FILE *snap = fopen(SNAPSHOT_FILE, "w");
    if (!snap) {
        printf("Cannot create snapshot file.\n");
        closedir(dp);
        return;
    }

    struct dirent *e;
    while ((e = readdir(dp)) != NULL)
    {
        if (strcmp(e->d_name, ".") == 0) continue;
	if (strcmp(e->d_name, "..") == 0) continue;
	if (strcmp(e->d_name, "user_dob") == 0) continue;

	char path[256];
	snprintf(path, sizeof(path), "%s/%s", CONFIG_DIR, e->d_name);

	FILE *f = fopen(path, "r");
	if (!f) continue;

	if (fgets(val, sizeof(val), f) != NULL) {
		size_t idx = strcspn(val, "\n");
		/* Sonar-safe: prove index is within buffer */
		if (idx < sizeof(val)) {
			val[idx] = '\0';
		} else {
			val[sizeof(val) - 1] = '\0';
		}
	} else {
		/* Read failed: keep empty string */
		val[0] = '\0';
	}

	fprintf(snap, "%s=%s\n", e->d_name, val);
    }

    fclose(snap);
    closedir(dp);

    system("sync");
    printf("Defaults captured.\n");
}

int run_cmd(const char *cmd)
{
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Command failed: %s\n", cmd);
        return -1;
    }
    return 0;
}

void capture_factory_backup()
{
    printf("Capturing full factory backup...\n");

    mkdir(FACTORY_BACKUP_DIR, 0755);

    run_cmd(
        "tar -cpf /mnt/flash/vienna/factory_backup/vienna.tar "
        "--exclude=vienna/lib "
        "--exclude=vienna/firmware "
        "--exclude=vienna/scripts "
        "--exclude=vienna/bin "
        "--exclude=vienna/factory_backup "
        "-C /mnt/flash vienna"
    );

     run_cmd(
        "tar -cpf /mnt/flash/vienna/factory_backup/etc.tar "
        "-C /mnt/flash etc"
    );

    system("sync");
    printf("Factory backup complete.\n");
}

/* Validate MAC address format */
int validate_mac(const char *mac) {
    regex_t regex;
    int ret = regcomp(&regex, "^([0-9A-Fa-f]{2}:){5}([0-9A-Fa-f]{2})$", REG_EXTENDED);
    if (ret) return 0;
    ret = regexec(&regex, mac, 0, NULL, 0);
    regfree(&regex);
    return ret == 0;
}

/* Reject all-00, all-FF and multicast MACs */
int validate_mac_value(const char *mac) {
    if (strcasecmp(mac, "00:00:00:00:00:00") == 0 ||
        strcasecmp(mac, "FF:FF:FF:FF:FF:FF") == 0) {
        fprintf(stderr, "Invalid MAC address: all-zero or all-FF not allowed.\n");
        return 0;
    }

    unsigned char first_byte = 0;
    if (sscanf(mac, "%2hhx", &first_byte) != 1)
        return 0;
    if (first_byte & 1) {
        fprintf(stderr, "Invalid MAC address: multicast bit set.\n");
        return 0;
    }

    return 1;
}

/* Validate MFG date in YYYY-MM-DD format */
int validate_date(const char *date) {
    regex_t regex;
    int ret = regcomp(&regex, "^[0-9]{4}-[0-9]{2}-[0-9]{2}$", REG_EXTENDED);
    if (ret) return 0;
    ret = regexec(&regex, date, 0, NULL, 0);
    regfree(&regex);
    if (ret != 0) return 0;

    int year, month, day;
    if (sscanf(date, "%4d-%2d-%2d", &year, &month, &day) != 3)
        return 0;
    if (month < 1 || month > 12 || day < 1 || day > 31)
        return 0;

    return 1;
}

/* Write string to file */
int write_to_file(const char *path, const char *value) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    fprintf(fp, "%s\n", value);
    fclose(fp);
    return 0;
}

/* Update hotspot_ssid with last 4 chars of MAC */
int update_hotspot_ssid(const char *mac) {
    FILE *fp = fopen(HOTSPOT_FILE, "r");
    if (!fp) {
        perror("fopen read hotspot");
        return -1;
    }

    char base_ssid[128];
    if (!fgets(base_ssid, sizeof(base_ssid), fp)) {
        fclose(fp);
        fprintf(stderr, "Failed to read hotspot_ssid\n");
        return -1;
    }
    fclose(fp);

    base_ssid[strcspn(base_ssid, "\n")] = '\0';  // Strip newline

    char mac_clean[32];
    int j = 0;
    for (int i = 0; mac[i]; i++) {
        if (mac[i] != ':')
            mac_clean[j++] = mac[i];
    }
    mac_clean[j] = '\0';

    if (j < 4) {
        fprintf(stderr, "Invalid MAC length after cleaning.\n");
        return -1;
    }

    char last4[5];
    strncpy(last4, mac_clean + j - 4, 4);
    last4[4] = '\0';

    char new_ssid[256];
    snprintf(new_ssid, sizeof(new_ssid), "%s_%s", base_ssid, last4);

    return write_to_file(HOTSPOT_FILE, new_ssid);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr,
                "Usage: %s <MAC> <SERIAL> <MFG_DATE> <EPOCH_TIME>\n"
                "\n"
                "Example:\n"
                "   %s 02:1A:2B:3C:4D:5E SN20251027001 2025-10-27 1767619974\n"
                "\n"
                "Arguments:\n"
                "  MAC        : XX:XX:XX:XX:XX:XX (no all-00 or all-FF)\n"
                "  SERIAL     : Device serial number string\n"
                "  MFG_DATE   : Format YYYY-MM-DD\n"
                "  EPOCH_TIME : 10-digit Unix timestamp (numeric only)\n",
                argv[0], argv[0]
               );
        return 1;
    }

    const char *mac = argv[1];
    const char *serial = argv[2];
    const char *mfg_date = argv[3];
    const char *epoch_time = argv[4];

    /* Step 1: Validate MAC address */
    if (!validate_mac(mac)) {
        fprintf(stderr, "Invalid MAC address format.\n");
        return 1;
    }
    if (!validate_mac_value(mac)) {
        return 1;
    }

    /* Step 2: Validate MFG date format */
    if (!validate_date(mfg_date)) {
        fprintf(stderr, "Invalid manufacturing date format. Use YYYY-MM-DD.\n");
        return 1;
    }

    /* Step 3: Set ethaddr using fw_setenv */
    char command[128];
    snprintf(command, sizeof(command), "fw_setenv ethaddr %s", mac);
    if (system(command) != 0) {
        fprintf(stderr, "Failed to set ethaddr.\n");
        return 1;
    }
    printf("Successfully set ethaddr to %s\n", mac);

    /* Step 4: Write serial number and mfg date */
    if (write_to_file(SERIAL_FILE, serial) != 0) {
        fprintf(stderr, "Failed to write serial_number.\n");
        return 1;
    }
    if (write_to_file(MFG_FILE, mfg_date) != 0) {
        fprintf(stderr, "Failed to write mfg_date.\n");
        return 1;
    }
    printf("Serial and Mfg date written successfully.\n");

    /* Step 5: Update hotspot_ssid */
    if (update_hotspot_ssid(mac) != 0) {
        fprintf(stderr, "Failed to update hotspot_ssid.\n");
        return 1;
    }
    printf("Hotspot SSID updated successfully.\n");

    /* Step 6: write epoch time and updating date */
    char command_buffer[128];
    snprintf(command_buffer, sizeof(command_buffer),
             "/mnt/flash/vienna/scripts/time_manager.sh update_time %s",
             epoch_time);
    if (system(command_buffer) != 0) {
        fprintf(stderr, "Failed to set epoch time.\n");
        //return 1;
    } else {
        printf("Successfully set Epoch time to %s\n",epoch_time);
    }

    /* Step 7: Capture default config snapshot */
    capture_defaults();
    printf("Default configuration snapshot captured.\n");

    /* Step 8: Capture factory reset backup snapshot */
    capture_factory_backup();
    printf("Default configuration snapshot captured.\n");

    /* Step 9: Delete binary after success */
    if (unlink(DEVICE_SETUP_BIN) == 0) {
        printf("Deleted binary: %s\n", DEVICE_SETUP_BIN);
    } else {
        perror("Warning: Failed to delete binary");
    }

    return 0;
}

