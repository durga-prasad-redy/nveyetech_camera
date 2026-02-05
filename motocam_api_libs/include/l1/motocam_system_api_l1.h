#include <stdint.h>

int8_t set_system_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data);
int8_t set_system_shutdown_l1(const uint8_t data_length, const uint8_t *data);
int8_t get_system_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data,
                           uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);

//system commands
int8_t set_system_factory_reset_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_system_config_reset_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_system_camera_name_l1(const uint8_t data_length, const uint8_t *data) ;
int8_t set_system_login_l1(const uint8_t data_length, const uint8_t *data);
int8_t provision_device_l1(const uint8_t data_length, const uint8_t *data);

int8_t get_system_camera_name_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_system_firmware_version_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_system_mac_address_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t login_with_pin_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size) ;

int8_t set_system_user_dob_l1(const uint8_t data_length, const uint8_t *data);
int8_t get_system_user_dob_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t set_system_time_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_system_haptic_motor_l1(const uint8_t data_length, const uint8_t *data);


int8_t get_config_streaming_config(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size) ;


int8_t set_ota_update_l1(const uint8_t data_length, const uint8_t *data);
int8_t get_ota_update_status_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_camera_health_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);