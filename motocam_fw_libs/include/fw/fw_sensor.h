#ifndef FW_SENSOR_H
#define FW_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h> 
#include "fw.h"

int8_t set_gyro_reader(enum ON_OFF on_off);
int8_t get_gyro_reader(uint8_t *gyro_reader);
int8_t get_motion_data(int16_t *mag_x, int16_t *mag_y, int16_t *mag_z);
int8_t get_sensor_temp(uint8_t *temp);
int8_t get_isp_temp(uint8_t *temp);
int8_t get_ir_temp(uint8_t *temp);
int8_t camera_health_check(uint8_t *streamer, uint8_t *rtsp, uint8_t *portable_rtc, uint8_t *cpu_usage, uint8_t *memory_usage, uint8_t *isp_temp, uint8_t *ir_temp, uint8_t *sensor_temp);

#ifdef __cplusplus
}
#endif

#endif
