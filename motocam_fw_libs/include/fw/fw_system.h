#ifndef FW_SYSTEM_H
#define FW_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fw.h"
#include "fw/fw_sensor.h"

int8_t set_camera_name(const char *camera_name);
int8_t set_login(const char *login_pin, const char *dob);
int8_t get_camera_name(char *camera_name);
int8_t get_firmware_version(char *firmware_version);
int8_t get_mac_address(char *mac_address);
int8_t get_ota_update_status(char *ota_status);
int8_t get_factory_reset_status(char *factory_reset_status);
int8_t get_login_pin(char *loginPin);
int8_t get_stream_resolution(enum image_resolution *resolution,
                             uint8_t stream_number);

int8_t shutdown_device();
int8_t ota_update();

int8_t provisioning_mode(const char *mac_address,const char *serial_number,const char *manufacture_date);
int8_t remove_ota_files();
int8_t set_factory_reset(const char *dob);
int8_t set_config_reset(const char *dob);
int8_t set_user_dob(const char *dob);
int8_t get_user_dob(char *dob);
int8_t validate_user_dob(const char *input_dob);
int8_t set_system_time(const char *epoch_time);

#ifdef __cplusplus
}
#endif

#endif
