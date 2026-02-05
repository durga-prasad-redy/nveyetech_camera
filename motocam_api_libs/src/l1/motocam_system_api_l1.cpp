#include <cstdlib>               
#include <cstdio> 
#include "motocam_system_api_l1.h"
#include "motocam_system_api_l2.h"
#include "motocam_command_enums.h"


/*
 *
 * System Set
 * */

 int8_t set_system_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data)
{
    switch (sub_command)
    {
    case SETCAMERANAME:
        return set_system_camera_name_l1(data_length, data);
    case SET_LOGIN:
        return set_system_login_l1(data_length, data);
    case FACTORY_RESET:
        return set_system_factory_reset_l1(data_length, data);
    case SHUTDOWN:
        return set_system_shutdown_l1(data_length, data);
    case OTA_UPDATE:
        return set_ota_update_l1(data_length, data);
    case PROVISION_DEVICE:
        return provision_device_l1(data_length, data);
    case SET_USER_DOB:
        return set_system_user_dob_l1(data_length, data);
    case CONFIG_RESET:
        return set_system_config_reset_l1(data_length, data);
    case SET_TIME:
        return set_system_time_l1(data_length, data);
    case HAPTIC_MOTOR:
        return set_system_haptic_motor_l1(data_length, data);

    default:
        printf("invalid sub command\n");
        return -4;
    }
    return 0;
}

int8_t set_system_shutdown_l1(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (set_system_shutdown_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_ota_update_l1(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    printf("set_ota_update_l2\n");

    if (data_length == 0)
    {
        printf("set_ota_update_l2\n");

        if (set_ota_update_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t provision_device_l1(const uint8_t data_length, const uint8_t *data)
{
    printf("provision_device_l1\n");

    if (provision_device_l2(data_length,data) == 0)
    {
        return 0;
    }
    printf("error in executing the command\n");
    return -1;

    
}

int8_t set_system_factory_reset_l1(const uint8_t data_length, const uint8_t *data)
{
    // Expected format: DOB (10 bytes, dd-mm-yyyy)
    if (data_length != 10)
    {
        printf("set_system_factory_reset_l1: invalid data length %d, expected 10 (DOB)\n", data_length);
        return -5;
    }
    
    // Extract DOB
    const uint8_t *dob = data;
    uint8_t dob_length = 10;
    
    // Call L2 with DOB
    int8_t ret = set_system_factory_reset_l2(dob_length, dob);
    if (ret == 0)
    {
        return 0;
    }
    else if (ret == -6)
    {
        printf("set_system_factory_reset_l1: DOB not set in system\n");
        return -6;
    }
    else if (ret == -7)
    {
        printf("set_system_factory_reset_l1: DOB validation failed\n");
        return -7;
    }
    
    printf("error in executing the command\n");
    return ret;
}

int8_t set_system_config_reset_l1(const uint8_t data_length, const uint8_t *data)
{
    // Expected format: DOB (10 bytes, dd-mm-yyyy)
    if (data_length != 10)
    {
        printf("set_system_config_reset_l1: invalid data length %d, expected 10 (DOB)\n", data_length);
        return -5;
    }
    
    // Extract DOB
    const uint8_t *dob = data;
    uint8_t dob_length = 10;
    
    // Call L2 with DOB
    int8_t ret = set_system_config_reset_l2(dob_length, dob);
    if (ret == 0)
    {
        return 0;
    }
    else if (ret == -6)
    {
        printf("set_system_config_reset_l1: DOB not set in system\n");
        return -6;
    }
    else if (ret == -7)
    {
        printf("set_system_config_reset_l1: DOB validation failed\n");
        return -7;
    }
    
    printf("error in executing the command\n");
    return ret;
}
int8_t set_system_camera_name_l1(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length >= 1 && data_length <= 32)
    {
        if (set_system_camera_name_l2(data_length, data) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_system_login_l1(const uint8_t data_length, const uint8_t *data)
{
    // Expected format: login_pin_length (1 byte) + login_pin (variable) + DOB (10 bytes)
    // Minimum: 1 + 1 + 10 = 12 bytes
    // Maximum: 1 + 32 + 10 = 43 bytes
    if (data_length < 12 || data_length > 43)
    {
        printf("set_system_login_l1: invalid data length %d\n", data_length);
        return -5;
    }
    
    // Parse login pin length
    uint8_t login_pin_length = data[0];
    
    // Validate login pin length
    if (login_pin_length < 1 || login_pin_length > 32)
    {
        printf("set_system_login_l1: invalid login pin length %d\n", login_pin_length);
        return -5;
    }
    
    // Validate total length matches expected
    uint8_t expected_length = 1 + login_pin_length + 10; // 1 (length) + pin + DOB
    if (data_length != expected_length)
    {
        printf("set_system_login_l1: data length mismatch. Expected %d, got %d\n", expected_length, data_length);
        return -5;
    }
    
    // Extract login pin
    const uint8_t *login_pin = &data[1];
    
    // Extract DOB (last 10 bytes)
    const uint8_t *dob = &data[1 + login_pin_length];
    uint8_t dob_length = 10;
    
    // Call L2 with login pin and DOB
    int8_t ret = set_system_login_l2(login_pin_length, login_pin, dob_length, dob);
    if (ret == 0)
    {
        return 0;
    }
    else if (ret == -6)
    {
        printf("set_system_login_l1: DOB not set in system\n");
        return -6;
    }
    else if (ret == -7)
    {
        printf("set_system_login_l1: DOB validation failed\n");
        return -7;
    }
    
    printf("error in executing the command\n");
    return ret;
}


/*
 *
 * System Get
 * */
int8_t get_system_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data,
                          uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    switch (sub_command)
    {
        //        case IPAddressSubnetMask:
        //            return get_network_IPAddressSubnetMask(data_length, data, res_data_bytes, res_data_bytes_size);
    case GETCAMERANAME:
        return get_system_camera_name_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case FIRMWAREVERSION:
        return get_system_firmware_version_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case MACADDRESS:
        return get_system_mac_address_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case LOGIN:
        return login_with_pin_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case OTA_UPDATE_STATUS:
        return get_ota_update_status_l1(data_length, data, res_data_bytes, res_data_bytes_size);

    case HEALTH_CHECK:
        return get_camera_health_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case GET_USER_DOB:
        return get_system_user_dob_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    default:
        printf("invalid system sub command\n");
        return -4;
    }
    return 0;
}

int8_t get_system_camera_name_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_system_camera_name_l2(res_data_bytes, res_data_bytes_size);
        if (ret == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t get_system_firmware_version_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_system_firmware_version_l2(res_data_bytes, res_data_bytes_size);
        if (ret == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t get_system_mac_address_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_system_mac_address_l2(res_data_bytes, res_data_bytes_size);
        if (ret == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t get_ota_update_status_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_ota_update_status_l2(res_data_bytes, res_data_bytes_size);
        if (ret == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t get_camera_health_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_camera_health_l2(res_data_bytes, res_data_bytes_size);

        if (ret == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t login_with_pin_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{

    // print the data as a string

    int8_t ret = login_with_pin_l2(data_length, data, res_data_bytes, res_data_bytes_size);
    if (ret == 0)
    {
        printf("login successful\n");
        return 0;
    }
    else if (ret == -1)
    {
        /* code */
        printf("login pin length should be 4");
        return -1;
    }

    else if (ret == -2)
    {
        printf("pin must be numeric\n");
        return -2;
    }
    else if (ret == -3)
    {
        printf("Authentication failed\n");
        return -3;
    }

    printf("error in executing the command\n");
    return -5;
}

int8_t set_system_user_dob_l1(const uint8_t data_length, const uint8_t *data) {
    if (data_length != 10) {
        printf("set_system_user_dob_l1: invalid length %d\n", data_length);
        return -5; 
    }

    if (set_system_user_dob_l2(data_length, data) == 0) {
        return 0;
    }
    
    printf("set_system_user_dob_l1: l2 failed\n");
    return -1;
}

int8_t get_system_user_dob_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size) {
    (void)data;
    if (data_length != 0) {
         printf("get_system_user_dob_l1: invalid length %d\n", data_length);
         return -5;
    }

    uint8_t *dob = NULL;
    uint8_t length = 0;
    if (get_system_user_dob_l2(&dob, &length) == 0) {
        *res_data_bytes = dob;
        *res_data_bytes_size = length;
        return 0;
    }

    printf("get_system_user_dob_l1: l2 failed\n");
    return -1;
}

int8_t set_system_time_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length >= 1 && data_length <= 32)
    {
        if (set_system_time_l2(data_length, data) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_system_haptic_motor_l1(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (set_system_haptic_motor_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
