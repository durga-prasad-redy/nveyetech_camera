#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <ctime>
#include <cctype>
#include "fw/fw_system.h"
#include "motocam_system_api_l2.h"
#include "motocam_api_l2.h"

// Forward declarations
static bool is_leap_year(int year);
static int get_days_in_month(int month, int year);
static int validate_dob_string(const char* date_str);

int8_t set_system_shutdown_l2() {
  printf("set_system_shutdown_l2\n");
  if (shutdown_device() < 0) {
    return -1;
  }
  return 0;
}

int8_t set_ota_update_l2() {
  printf("set_ota_update_l2\n");
  if (ota_update() < 0) {
    return -1;
  }
  return 0;
}

int8_t provision_device_l2(const uint8_t provision_data_len,
                           const uint8_t *provision_data) {
  printf("provision_device_l2 provision data len=%d\n", provision_data_len);
  uint8_t macid_len_idx = 0;
  uint8_t macid_len = provision_data[macid_len_idx];
  uint8_t macid_idx = (uint8_t)(macid_len_idx + 1);
  const uint8_t *macid = &provision_data[macid_idx];
  if (provision_data_len < macid_idx + macid_len + 1) // 1 for val
    return -1;

  uint8_t serial_number_len_idx = (uint8_t)(macid_idx + 1);
  uint8_t serial_number_len = provision_data[serial_number_len_idx];
  uint8_t serial_number_idx = (uint8_t)(serial_number_len_idx + 1);
  const uint8_t *serial_number = &provision_data[serial_number_idx];
  if (provision_data_len <
      serial_number_idx + serial_number_len + 2) // 2---1 for len 1 for val
    return -1;

  uint8_t manufacture_date_len_idx =
      (uint8_t)(serial_number_idx + serial_number_len);
  uint8_t manufacture_date_len = provision_data[manufacture_date_len_idx];
  uint8_t manufacture_date_idx = (uint8_t)(manufacture_date_len_idx + 1);
  const uint8_t *manufacture_date = &provision_data[manufacture_date_idx];
  if (provision_data_len < manufacture_date_idx + manufacture_date_len +
                               2) // 2-- 1 for len, 1 for val
    return -1;

  char macid_str[100];
  uint8_t i;
  for (i = 0; i < macid_len; i++) {
    macid_str[i] = macid[i];
  }
  macid_str[macid_len] = '\0';

  if (macid_len > 0 && macid_str[macid_len - 1] == '\n') {
    macid_str[macid_len - 1] = '\0';
  }

  char serial_number_str[100];
  for (i = 0; i < serial_number_len; i++) {
    serial_number_str[i] = serial_number[i];
  }
  serial_number_str[serial_number_len] = '\0';

  char manufacture_date_str[100];
  for (i = 0; i < manufacture_date_len; i++) {
    manufacture_date_str[i] = manufacture_date[i];
  }
  manufacture_date_str[manufacture_date_len] = '\0';

  // int8_t ret = set_wifi_hotspot_config(ssid_str, encryption_type,
  // encryption_key_str, ipaddress_str, subnetmask_str);
  int8_t ret =
      provisioning_mode(macid_str, serial_number_str, manufacture_date_str);
  printf("provisioning_mode  macid_str=%s, serial_number_str=%s, "
         "manufacture_date_str=%s ret=%d\n",
         macid_str, serial_number_str, manufacture_date_str, ret);

  if (ret < 0)
    return ret;

  // writeState(motocam_wifi_state_file, &current_wifi_state);
  return 0;
}

int8_t set_system_factory_reset_l2(const uint8_t dobLength, const uint8_t *dob) {
  printf("set_system_factory_reset_l2\n");
  
  // Validate DOB length
  if (dobLength != 10) {
    printf("set_system_factory_reset_l2: invalid DOB length %d\n", dobLength);
    return -5; // Invalid data length
  }
  
  // Convert DOB to string
  char dob_str[11];
  for (uint8_t i = 0; i < dobLength; i++) {
    dob_str[i] = dob[i];
  }
  dob_str[dobLength] = '\0';
  
  // Validate DOB format
  int val_ret = validate_dob_string(dob_str);
  if (val_ret != 0) {
    printf("set_system_factory_reset_l2: invalid DOB format %d\n", val_ret);
    return val_ret; // Return validation error
  }
  
  // Call fw function with DOB validation
  int8_t ret = set_factory_reset(dob_str);
  if (ret < 0) {
    return ret; // Return -6 (DOB not set) or -7 (DOB mismatch) or -1 (other error)
  }
  
  // Reset the current config to factory defaults
  current_config = factory_config;
  
  return 0;
}

int8_t set_system_config_reset_l2(const uint8_t dobLength, const uint8_t *dob) {
  printf("set_system_config_reset_l2\n");
  
  // Validate DOB length
  if (dobLength != 10) {
    printf("set_system_config_reset_l2: invalid DOB length %d\n", dobLength);
    return -5; // Invalid data length
  }
  
  // Convert DOB to string
  char dob_str[11];
  for (uint8_t i = 0; i < dobLength; i++) {
    dob_str[i] = dob[i];
  }
  dob_str[dobLength] = '\0';
  
  // Validate DOB format
  int val_ret = validate_dob_string(dob_str);
  if (val_ret != 0) {
    printf("set_system_config_reset_l2: invalid DOB format %d\n", val_ret);
    return val_ret; // Return validation error
  }
  
  // Call fw function with DOB validation
  int8_t ret = set_config_reset(dob_str);
  if (ret < 0) {
    return ret; // Return -6 (DOB not set) or -7 (DOB mismatch) or -1 (other error)
  }
  
  return 0;
}
int8_t set_system_camera_name_l2(const uint8_t cameraNameLength,
                                 const uint8_t *cameraName) {
  printf("set_system_camera_name %d\n", cameraNameLength);
  if (cameraNameLength > 32) {
    return -1; // Name too long
  }

  char name[33]; // 32 characters + null terminator
  for (uint8_t i = 0; i < cameraNameLength; i++) {
    name[i] = cameraName[i];
  }
  name[cameraNameLength] = '\0'; // Null-terminate the string

  if (set_camera_name(name) < 0) {
    return -1; // Error setting camera name
  }

  return 0;
}

int8_t set_system_login_l2(const uint8_t loginLength, const uint8_t *loginPin, const uint8_t dobLength, const uint8_t *dob) {
  printf("set_system_login_l2 loginLength=%d, dobLength=%d\n", loginLength, dobLength);
  
  if (loginLength > 32) {
    return -1; // Login too long
  }
  
  // Validate DOB length
  if (dobLength != 10) {
    printf("set_system_login_l2: invalid DOB length %d\n", dobLength);
    return -5; // Invalid data length
  }

  char login_str[33]; // 32 characters + null terminator
  for (uint8_t i = 0; i < loginLength; i++) {
    login_str[i] = loginPin[i];
  }
  login_str[loginLength] = '\0'; // Null-terminate the string

  // Convert DOB to string
  char dob_str[11];
  for (uint8_t i = 0; i < dobLength; i++) {
    dob_str[i] = dob[i];
  }
  dob_str[dobLength] = '\0';
  
  // Validate DOB format
  int val_ret = validate_dob_string(dob_str);
  if (val_ret != 0) {
    printf("set_system_login_l2: invalid DOB format %d\n", val_ret);
    return val_ret; // Return validation error
  }

  // Call fw function with DOB validation
  int8_t ret = set_login(login_str, dob_str);
  if (ret < 0) {
    return ret; // Return -6 (DOB not set) or -7 (DOB mismatch) or -1 (other error)
  }

  return 0;
}

int8_t get_system_camera_name_l2(uint8_t **cameraName, uint8_t *length) {
  printf("get_system_camera_name_l2\n");
  char name[33]; // 32 characters + null terminator
  if (get_camera_name(name) < 0) {
    return -1; // Error getting camera name
  }

  *length = (uint8_t)strlen(name);
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  for (uint8_t i = 0; i < *length; i++) {
    buf[i] = (uint8_t)name[i];
  }
  *cameraName = buf.release();

  return 0;
}
int8_t get_system_firmware_version_l2(uint8_t **firmwareVersion,
                                      uint8_t *length) {
  printf("get_system_firmware_version_l2\n");
  char version[33]; // 32 characters + null terminator
  if (get_firmware_version(version) < 0) {
    return -1; // Error getting firmware version
  }

  *length = (uint8_t)strlen(version);
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  for (uint8_t i = 0; i < *length; i++) {
    buf[i] = (uint8_t)version[i];
  }
  *firmwareVersion = buf.release();

  return 0;
}
int8_t get_system_mac_address_l2(uint8_t **macAddress, uint8_t *length) {
  printf("get_system_mac_address_l2\n");
  char mac[18]; // 17 characters + null terminator
  if (get_mac_address(mac) < 0) {
    return -1; // Error getting MAC address
  }

  *length = (uint8_t)strlen(mac);
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  for (uint8_t i = 0; i < *length; i++) {
    buf[i] = (uint8_t)mac[i];
  }
  *macAddress = buf.release();

  return 0;
}

int8_t get_ota_update_status_l2(uint8_t **ota_status, uint8_t *length) {
  printf("get_ota_update_status_l2\n");
  char status[18]; // 17 characters + null terminator
  if (get_ota_update_status(status) < 0) {
    return -1; // Error getting MAC address
  }

  *length = (uint8_t)strlen(status);
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  for (uint8_t i = 0; i < *length; i++) {
    buf[i] = (uint8_t)status[i];
  }
  *ota_status = buf.release();

  return 0;
}

int8_t get_camera_health_l2(uint8_t **camera_health, uint8_t *length) {
  printf("get_camera_health_l2\n");

  uint8_t stream_status;
  uint8_t rtsp_status;
  uint8_t portableRtc_status;
  uint8_t cpu_usage;
  uint8_t memory_usage;
  uint8_t isp_temp;
  uint8_t ir_temp;
  uint8_t sensor_temp;

  int8_t ret = camera_health_check(
      &stream_status, &rtsp_status, &portableRtc_status, &cpu_usage,
      &memory_usage, &isp_temp, &ir_temp, &sensor_temp);
  if (ret < 0)
    return -1;



  *length = (uint8_t)(8); // 5 bytes for status + 3 bytes for temperatures
  // 1 byte for each status and 1 byte for each temperature
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  uint8_t camera_health_idx = 0;
  buf[camera_health_idx++] = stream_status;
  buf[camera_health_idx++] = rtsp_status;
  buf[camera_health_idx++] = portableRtc_status;
  buf[camera_health_idx++] = cpu_usage;
  buf[camera_health_idx++] = memory_usage;
  buf[camera_health_idx++] = isp_temp;
  buf[camera_health_idx++] = ir_temp;
  buf[camera_health_idx++] = sensor_temp;
  *camera_health = buf.release();

  return 0;
}

int8_t login_with_pin_l2(const uint8_t pinLength, const uint8_t *loginPin,
                         uint8_t **auth_data_byte,
                         uint8_t *auth_data_bytes_size) {
  // printf("get_system_mac_address_l2\n");
  char current_login_pin[18]; // 17 characters + null terminator
  if (get_login_pin(current_login_pin) < 0) {
    return -1; // Error getting MAC address
  }
  // char *user_provided_pin = (char *)malloc(pinLength + 1);
  // for (uint8_t i = 0; i < pinLength; i++)
  // {
  //     printf("user provided pin char %d: %d\n", i, loginPin[i]);
  //     user_provided_pin[i] = (char)loginPin[i];
  // }
  // user_provided_pin[pinLength] = '\0'; // Null-terminate the string

  uint8_t user_pin_len_idx = 0;
  uint8_t user_pin_len = pinLength;
  uint8_t user_pin_idx = (uint8_t)(user_pin_len_idx);
  const uint8_t *user_pin = &loginPin[user_pin_idx];

  if (user_pin_len != 4) {
    printf("user provided pin length invalid: %d\n", user_pin_len);
    return -1; // Pin length must be 4
  }

  char user_provided_pin[32];
  uint8_t i;
  for (i = 0; i < user_pin_len; i++) {

    // Check if character is not numeric (ASCII '0' to '9')
    if (user_pin[i] < 48 || user_pin[i] > 57) {
      printf("user provided pin char %d: %d\n", i, user_pin[i]);

      return -2; // Pin must be numeric
    }

    user_provided_pin[i] = user_pin[i];
  }
  user_provided_pin[user_pin_len] = '\0';

  if (user_pin_len > 0 && user_provided_pin[user_pin_len - 1] == '\n') {
    user_provided_pin[user_pin_len - 1] = '\0';
  }

  *auth_data_bytes_size = 1; // Assuming auth data size is 1 byte
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*auth_data_bytes_size]);
  if (!buf) return -1;
  if (strcmp(current_login_pin, user_provided_pin) == 0) {
    buf[0] = 0; // Indicating success
    printf("Authentication successful, current login pin: %s, user provided "
           "pin: %s\n",
           current_login_pin, user_provided_pin);
    *auth_data_byte = buf.release();
    return 0;
  } else {
    buf[0] = 3; // Indicating failure
    printf(
        "Authentication failed, current login pin: %s, user provided pin: %s\n",
        current_login_pin, user_provided_pin);
    *auth_data_byte = buf.release();
    return -3;
  }

}

static bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int get_days_in_month(int month, int year) {
    switch (month) {
        case 1: return 31;
        case 2: return is_leap_year(year) ? 29 : 28;
        case 3: return 31;
        case 4: return 30;
        case 5: return 31;
        case 6: return 30;
        case 7: return 31;
        case 8: return 31;
        case 9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 0;
    }
}

// Returns 0 if valid, negative error code if invalid
static int validate_dob_string(const char* date_str) {
    printf("validate_dob_string: %s\n", date_str);
    if (strlen(date_str) != 10) return -1;
    if (date_str[2] != '-' || date_str[5] != '-') return -1;

    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (!isdigit(date_str[i])) return -1;
    }

    int day = atoi(date_str);
    int month = atoi(date_str + 3);
    int year = atoi(date_str + 6);

    if (month < 1 || month > 12) return -2;
    if (day < 1 || day > get_days_in_month(month, year)) return -2;

    printf("validate_dob_string: day=%d, month=%d, year=%d\n", day, month, year);


    return 0;
}

int8_t set_system_user_dob_l2(const uint8_t length, const uint8_t *dob) {
    printf("set_system_user_dob_l2\n");
    
    char dob_str[11];
    if (length > 10) return -1;
    
    // Copy safely
    uint8_t copy_len = (length < 10) ? length : 10;
    memcpy(dob_str, dob, copy_len);
    dob_str[copy_len] = '\0';
    
    // Validate
    int val_ret = validate_dob_string(dob_str);
    if (val_ret != 0) {
        printf("set_system_user_dob_l2: validation failed %d\n", val_ret);
        return val_ret;
    }

    if (set_user_dob(dob_str) < 0) {
        return -1;
    }
    return 0;
}

int8_t get_system_user_dob_l2(uint8_t **dob, uint8_t *length) {
    printf("get_system_user_dob_l2\n");
    char buffer[11];
    if (get_user_dob(buffer) == 0) {
        *length = 10;
        std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
        if (!buf) return -1;
        memcpy(buf.get(), buffer, *length);
        *dob = buf.release();
        return 0;
    }
    return -1;
}

int8_t set_system_time_l2(uint8_t epoch_time_length,
                          const uint8_t *epoch_time)
{
    if (!epoch_time || epoch_time_length == 0 || epoch_time_length > 32)
        return -1;

    char epoch_str[33];

    for (uint8_t i = 0; i < epoch_time_length; i++) {
        if (epoch_time[i] > 9) {
            return -1;  // Not a digit
        }
        epoch_str[i] = (char)(epoch_time[i] + '0');
    }

    epoch_str[epoch_time_length] = '\0';

    printf("epoch_str = %s\n", epoch_str);

    return set_system_time(epoch_str) < 0 ? -1 : 0;
}

int8_t set_system_haptic_motor_l2() {
  printf("set_system_haptic_motor_l2\n");
  if (set_haptic_motor(2000, 3) < 0) {
    return -1;
  }
  return 0;
}
