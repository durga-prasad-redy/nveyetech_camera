#include "motocam_audio_api_l1.h"
#include "motocam_audio_api_l2.h"


/*
 *
 * Audio
 * */

int8_t set_audio_mic_l1(const uint8_t data_length, const uint8_t *data)
{
    if (data_length == 1)
    {
        if (set_audio_mic_l2(data[0]) == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_audio_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data)
{
    switch (sub_command)
    {
    case MIC:
        return set_audio_mic_l1(data_length, data);
    default:
        printf("invalid sub command\n");
        return -4;
    }
    return 0;
}

int8_t get_audio_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data,
                         uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    switch (sub_command)
    {
    case MIC:
        return get_audio_mic_l1(data_length, data, res_data_bytes, res_data_bytes_size);
    default:
        printf("invalid sub command\n");
        return -4;
    }
    return 0;
}

int8_t get_audio_mic_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_audio_mic_l2(res_data_bytes, res_data_bytes_size);
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
