#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <signal.h>
#include "gpio.h"
#include "timer.h"
#include "log.h"

#define CONFIG_PATH "/mnt/flash/vienna/config"
#define GYRO_PATH "/sys/bus/iio/devices/iio:device1"

static float read_calibration_value(const char *filepath)
{
    float value = 0.0f;

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 0.0f;
    }

    if (fscanf(file, "%f", &value) != 1) {
        perror("Failed to read float from file\n");
    }

    fclose(file);
    return value;
}

int update_calibration_value()
{
    char output_file_path[256];
    float x_cali;
    float y_cali;
    float z_cali;
    FILE *outfile;

    outfile = fopen("/sys/bus/iio/devices/iio:device1/misc_self_test", "r");
    if (outfile == NULL) {
        LOG_ERROR("Failed to open file");
        return 1;
    }

    while (fgets(output_file_path, sizeof(output_file_path), outfile));

    fclose(outfile);

    snprintf(output_file_path, sizeof(output_file_path), "%s/gyro_calib_bias.txt", CONFIG_PATH);

    outfile = fopen(output_file_path, "w");
    if (outfile == NULL) {
        LOG_ERROR("Error opening output file");
        return 1;
    }

    x_cali = read_calibration_value(GYRO_PATH "/in_anglvel_x_st_calibbias");
    y_cali = read_calibration_value(GYRO_PATH "/in_anglvel_y_st_calibbias");
    z_cali = read_calibration_value(GYRO_PATH "/in_anglvel_z_st_calibbias");

    fprintf(outfile, "[Gyro_Calibration_Value]\n");
    fprintf(outfile, "in_anglvel_x_st_cali = %f\n", x_cali);
    fprintf(outfile, "in_anglvel_y_st_cali = %f\n", y_cali);
    fprintf(outfile, "in_anglvel_z_st_cali = %f\n", z_cali);

    fclose(outfile);
    return 0;
}

int main() {

    gpio_init();

    update_calibration_value();

    timer_init();

    while (1) {
        pause(); // Wait for signals
    }
}
