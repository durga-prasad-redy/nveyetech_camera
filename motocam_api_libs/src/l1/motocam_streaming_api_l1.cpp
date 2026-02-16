#include <cstdlib>
#include <cstdio>
#include "motocam_streaming_api_l1.h"
#include "motocam_streaming_api_l2.h"
#include "motocam_command_enums.h"

int8_t set_streaming_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data)
{
    switch (sub_command)
    {
    case START_STREAMING:
        return set_streaming_start(data_length, data);
    case STOP_STREAMING:
        return set_streaming_stop(data_length, data);
        
    case START_WEBRTC_STREAMING:
        return start_webrtc_streaming_l1(data_length, data);
    case STOP_WEBRTC_STREAMING:
        return stop_webrtc_streaming_l1(data_length, data);
    default:
        printf("invalid sub command\n");
        return -4;
    }
}

int8_t get_streaming_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data,
                             uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    switch (sub_command)
    {
    case STREAM_STATE:
        return get_streaming_state(data_length, data, res_data_bytes, res_data_bytes_size);
    case WEBRTC_STREAMING_STATUS:
        return get_webrtc_streaming_state_l1(data_length, data, res_data_bytes, res_data_bytes_size);

    default:
        printf("invalid sub command\n");
        return -4;
    }
}
int8_t set_streaming_start(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (start_stream_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t start_webrtc_streaming_l1(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (start_webrtc_streaming_state_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command in the l1\n");
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t stop_webrtc_streaming_l1(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (stop_webrtc_streaming_state_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}

int8_t set_streaming_stop(const uint8_t data_length, const uint8_t *data)
{
    (void)data;
    if (data_length == 0)
    {
        if (stop_stream_l2() == 0)
        {
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}




int8_t get_streaming_state(const uint8_t data_length,
                           const uint8_t* data,
                           uint8_t** res_data_bytes,
                           uint8_t* res_data_bytes_size)
{
    (void)data;

    if (data_length != 0) {
        printf("invalid data/data length\n");
        return -5;
    }

    int8_t streaming_state = get_stream_state_l2();
    if (streaming_state <= 0) {
        printf("error in executing the command\n");
        return -1;
    }

    *res_data_bytes_size = 1;

    // Allocate using unique_ptr (no malloc)
    std::unique_ptr<uint8_t[]> buffer(new (std::nothrow) uint8_t[*res_data_bytes_size]);
    if (!buffer) {
        return -2;  // allocation failure
    }

    buffer[0] = static_cast<uint8_t>(streaming_state);

    // Transfer ownership to caller
    *res_data_bytes = buffer.release();

    return 0;
}




int8_t get_webrtc_streaming_state_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size)
{
    (void)data;
    if (data_length == 0)
    {
        int8_t ret = get_webrtc_streaming_state_l2(res_data_bytes, res_data_bytes_size);
        if (ret == 0)
        {
            printf("get_webrtc_streaming_state_l1: success\n");
            return 0;
        }
        printf("error in executing the command\n");
        return -1;
    }
    printf("invalid data/data length\n");
    return -5;
}
