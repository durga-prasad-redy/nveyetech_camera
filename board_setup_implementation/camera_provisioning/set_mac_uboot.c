/**
 * @file        set_mac_uboot.c
 * @author      Shah Rikin R.
 * @date        2025-06-24
 * @brief       Program to set the MAC address in U-Boot environment using fw_setenv.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

int validate_mac(const char *mac) {
    regex_t regex;
    int ret = regcomp(&regex, "^([0-9A-Fa-f]{2}:){5}([0-9A-Fa-f]{2})$", REG_EXTENDED);
    if (ret) return 0;
    ret = regexec(&regex, mac, 0, NULL, 0);
    regfree(&regex);
    return ret == 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <MAC_ADDRESS>\n", argv[0]);
        return 1;
    }

    const char *mac = argv[1];

    if (!validate_mac(mac)) {
        fprintf(stderr, "Invalid MAC address format.\n");
        return 1;
    }

    char command[128];
    snprintf(command, sizeof(command), "fw_setenv ethaddr %s", mac);

    int status = system(command);

    if (status == 0) {
        printf("Successfully set ethaddr to %s\n", mac);
        // Delete the binary after success
        if (unlink("/mnt/flash/vienna/firmware/board_setup/set_mac_uboot") == 0) {
            printf("Deleted binary: /mnt/flash/vienna/firmware/board_setup/set_mac_uboot\n");
        } else {
            perror("Failed to delete binary");
        }
    } else {
        fprintf(stderr, "Failed to set ethaddr.\n");
        return 1;
    }

    return 0;
}

