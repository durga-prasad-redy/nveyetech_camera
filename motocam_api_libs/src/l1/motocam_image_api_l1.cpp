#include <cstdlib>
#include <cstdio> 
#include "fw/fw_image.h"         
#include "fw/fw_sensor.h"
#include "motocam_image_api_l1.h"
#include "motocam_image_api_l2.h"
#include "motocam_command_enums.h"

/*
 *
 * IMAGE SET
 * */

 int8_t set_image_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data)
{
    switch (sub_command)
    {
    case ZOOM:
        return set_image_zoom_l1(data_length, data);
    case ROTATION:
        return set_image_rotation_l1(data_length, data);
    case IRCUTFILTER:
        return set_image_ircutfilter_l1(data_length, data);
    case IRBRIGHTNESS:
        return set_image_irbrightness_l1(data_length, data);
    case DAYMODE:
        return set_image_daymode_l1(data_length, data);
    case RESOLUTION:
        return set_image_resolution_l1(data_length, data);
    case MIRROR:
        return set_image_mirror_l1(data_length, data);
    case FLIP:
        return set_image_flip_l1(data_length, data);
    case TILT:
        return set_image_tilt_l1(data_length, data);
    case WDR:
        return set_image_wdr_l1(data_length, data);
    case EIS:
        return set_image_eis_l1(data_length, data);
    case GYROREADER:
        return set_image_gyroreader_l1(data_length, data);
    case MISC:
        return set_image_misc_l1(data_length, data);
    case MID_IRBRIGHTNESS:
        return set_image_mid_irbrightness_l1(data_length, data);
    case SIDE_IRBRIGHTNESS:
        return set_image_side_irbrightness_l1(data_length, data);
    default:
        printf("invalid sub command\n");
        return -4;
    }
    return 0;
}

int8_t set_image_zoom_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_zoom_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_rotation_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_rotation_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_ircutfilter_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_ircutfilter_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_irbrightness_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_irbrightness_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_image_mid_irbrightness_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_mid_irbrightness_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_side_irbrightness_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_side_irbrightness_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_daymode_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_daymode_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_gyroreader_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_gyroreader_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_resolution_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_resolution_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_mirror_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_mirror_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_flip_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_flip_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_tilt_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_tilt_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_wdr_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_wdr_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
int8_t set_image_eis_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_image_eis_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_image_misc_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        int8_t ret=set_image_misc_l2(data[0]);
        if (ret == 0)
        {
            return 0;
        }
        else if(ret == -1)
        {
            printf("Cannot set misc while in auto day/night mode\n");
            return -1;
        }    
        else if(ret == -2)
        {
            LOG_ERROR("error in setting misc\n");
            return -2;
        }
        
        LOG_ERROR("error in executing the command\n");
        return -3;
    }
    LOG_ERROR("invalid data/data length\n");
    return -5;
}




/*
 *
 * IMAGE GET
 * */

int8_t get_image_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data,
                         uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    switch (sub_command)
    {
    case ZOOM:
        return get_image_zoom_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case ROTATION:
        return get_image_rotation_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case IRCUTFILTER:
        return get_image_ircutfilter_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case IRBRIGHTNESS:
        return get_image_irbrightness_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case DAYMODE:
        return get_image_daymode_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case RESOLUTION:
        return get_image_resolution_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case MIRROR:
        return get_image_mirror_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case FLIP:
        return get_image_flip_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case TILT:
        return get_image_tilt_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case WDR:
        return get_image_wdr_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case EIS:
        return get_image_eis_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    case GYROREADER:
        return get_image_gyroreader_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    default:
        printf("invalid sub command\n");
        return -4;
    }
    return 0;
}

int8_t get_image_zoom_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_image_zoom_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_rotation_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_image_rotation_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_ircutfilter_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_image_ircutfilter_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_irbrightness_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_image_irbrightness_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_daymode_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_image_daymode_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_gyroreader_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_gyroreader_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_resolution_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_image_resolution_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_mirror_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_image_mirror_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_flip_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_image_flip_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_tilt_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_image_tilt_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_wdr_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_wdr_l2(res_data_bytes, res_data_bytes_size);
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
int8_t get_image_eis_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_eis_l2(res_data_bytes, res_data_bytes_size);
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
