#include <stdint.h>

// system commands
int8_t get_system_camera_name_l2(uint8_t **camera_name, uint8_t *length);
int8_t get_system_firmware_version_l2(uint8_t **firmware_version,
                                      uint8_t *length);
int8_t get_system_mac_address_l2(uint8_t **mac_address, uint8_t *length);
int8_t get_ota_update_status_l2(uint8_t **mac_address, uint8_t *length);
int8_t get_camera_health_l2(uint8_t **mac_address, uint8_t *length);
int8_t get_system_user_dob_l2(uint8_t **dob, uint8_t *length);

int8_t set_system_login_l2(const uint8_t pinLength, const uint8_t *loginPin, const uint8_t dobLength, const uint8_t *dob);
int8_t set_system_camera_name_l2(const uint8_t cameraNameLength,
                                 const uint8_t *cameraName);
int8_t set_system_factory_reset_l2(const uint8_t dobLength, const uint8_t *dob);
int8_t set_system_config_reset_l2(const uint8_t dobLength, const uint8_t *dob);
int8_t set_system_user_dob_l2(const uint8_t length, const uint8_t *dob);
// int8_t login_with_pin_l2(uint8_t **loginPin, uint8_t *length);
int8_t login_with_pin_l2(const uint8_t loginPin, const uint8_t *pinLength,
                         uint8_t **auth_data_byte,
                         uint8_t *auth_data_bytes_size);


int8_t set_system_shutdown_l2();
int8_t set_ota_update_l2();
int8_t provision_device_l2(const uint8_t provision_data_len,
                           const uint8_t *provision_data);
int8_t set_system_time_l2(const uint8_t epoch_time_length, const uint8_t *epoch_time);
int8_t set_system_haptic_motor_l2();
