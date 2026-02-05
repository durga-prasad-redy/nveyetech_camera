#include <cstdio>
#include <cstdlib>
#include "motocam_config_api_l1.h"
#include "motocam_config_api_l2.h"
#include "motocam_command_enums.h"
#include "motocam_system_api_l1.h"
#include "fw/fw_streaming.h"
/*
 *
 * Set CONFIG
 * */

int8_t set_config_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data)
{
    switch (sub_command)
    {
    case DefaultToFactory:
        return set_config_defaulttofactory(data_length, data);
    case DefaultToCurrent:
        return set_config_defaulttocurrent(data_length, data);
    case CurrentToFactory:
        return set_config_currenttofactory(data_length, data);
    case CurrentToDefault:
        return set_config_currenttodefault(data_length, data);
    default:
        printf("invalid sub command\n");
        return -4;
    }
    return 0;
}


int8_t set_config_defaulttofactory(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (set_config_defaulttofactory_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_config_defaulttocurrent(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (set_config_defaulttocurrent_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_config_currenttofactory(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (set_config_currenttofactory_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_config_currenttodefault(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (set_config_currenttodefault_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}


/*
*
* Get Config
* */

int8_t get_config_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data,
                          uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    switch (sub_command)
    {
    case Factory:
        return get_config_factory(data_length, data, res_data_bytes, res_data_bytes_size);
    case Default:
        return get_config_default(data_length, data, res_data_bytes, res_data_bytes_size);
    case Current:
        return get_config_current(data_length, data, res_data_bytes, res_data_bytes_size);
    case StreamingConfig:
        return get_config_streaming_config(data_length, data, res_data_bytes, res_data_bytes_size);
    default:
        printf("invalid sub command\n");
        return -4;
    }
    return 0;
}

int8_t get_config_factory(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_config_factory_l2(res_data_bytes, res_data_bytes_size);
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

int8_t get_config_default(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_config_default_l2(res_data_bytes, res_data_bytes_size);
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

int8_t get_config_current(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_config_current_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_config_streaming_config(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_config_streaming_config_l2(res_data_bytes, res_data_bytes_size);
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
