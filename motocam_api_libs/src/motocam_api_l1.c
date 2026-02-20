//
// Created by sr on 2/6/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "motocam_command_enums.h"
#include "motocam_api_l1.h"
#include "motocam_helper_api_l1.h"

int8_t do_processing(const uint8_t *req_bytes, const uint8_t req_bytes_size, uint8_t **res_bytes, uint8_t *res_bytes_size)
{
    printf("do_processing req_bytes=");
    int i;
    for (i = 0; i < req_bytes_size; i++)
    {
        printf("%d", req_bytes[i]);
    }
    printf("\n");

    uint8_t header = req_bytes[0];
    uint8_t command = req_bytes[1];
    uint8_t sub_command = req_bytes[2];
    uint8_t data_length = req_bytes[3];

    const uint8_t *data = NULL;
    if (data_length > 0)
    {
        data = &req_bytes[4];
    }

    uint8_t packet_length_except_data = 5;
    uint8_t data_success_flag_size = 1;

    if (validate_req_bytes_crc(req_bytes, req_bytes_size) != 0)
    {
        uint8_t res_data_bytes_size = 1;
        *res_bytes_size = packet_length_except_data + data_success_flag_size + res_data_bytes_size;
        *res_bytes = (uint8_t *)malloc(*res_bytes_size);
        (*res_bytes)[0] = header;
        (*res_bytes)[1] = command;
        (*res_bytes)[2] = sub_command;
        (*res_bytes)[3] = data_success_flag_size + res_data_bytes_size;            // DataLength
        (*res_bytes)[4] = 1;                                                       // failed
        (*res_bytes)[5] = (uint8_t)-6;                                             // err invalid CRC
        (*res_bytes)[*res_bytes_size - 1] = calc_crc(*res_bytes, *res_bytes_size); // last byte CRC
    }
    else if (header == SET)
    {
        int8_t ret = set_command(command, sub_command, data_length, data);
        uint8_t res_data_bytes_size = 1;
        *res_bytes_size = packet_length_except_data + data_success_flag_size + res_data_bytes_size;
        *res_bytes = (uint8_t *)malloc(*res_bytes_size);
        (*res_bytes)[0] = ACK;
        (*res_bytes)[1] = command;
        (*res_bytes)[2] = sub_command;
        (*res_bytes)[3] = data_success_flag_size + res_data_bytes_size; // DataLength
        printf("do_processing: set_command returned %d\n", ret);
        if (ret >= 0)
        {
            (*res_bytes)[4] = 0; // success
        }
        else
        {
            (*res_bytes)[4] = 1; // failed
        }
        (*res_bytes)[5] = (uint8_t)ret;                                            // success / err value
        (*res_bytes)[*res_bytes_size - 1] = calc_crc(*res_bytes, *res_bytes_size); // last byte CRC
    }
    else if (header == GET)
    {
        uint8_t res_data_bytes_size = 0;
        uint8_t *res_data_bytes;
        int8_t ret = get_command(command, sub_command, data_length, data, &res_data_bytes, &res_data_bytes_size);
        if (ret == 0)
        {
            *res_bytes_size = packet_length_except_data + data_success_flag_size + res_data_bytes_size;
            *res_bytes = (uint8_t *)malloc(*res_bytes_size);
            (*res_bytes)[0] = RESPONSE;
            (*res_bytes)[1] = command;
            (*res_bytes)[2] = sub_command;
            (*res_bytes)[3] = data_success_flag_size + res_data_bytes_size; // DataLength
            (*res_bytes)[4] = 0;                                            // success
            memcpy(&((*res_bytes)[5]), res_data_bytes, res_data_bytes_size);
            (*res_bytes)[*res_bytes_size - 1] = calc_crc(*res_bytes, *res_bytes_size); // last byte CRC

            free(res_data_bytes);
        }
        else
        {
            res_data_bytes_size = 1;
            *res_bytes_size = packet_length_except_data + data_success_flag_size + res_data_bytes_size;
            *res_bytes = (uint8_t *)malloc(*res_bytes_size);
            (*res_bytes)[0] = RESPONSE;
            (*res_bytes)[1] = command;
            (*res_bytes)[2] = sub_command;
            (*res_bytes)[3] = data_success_flag_size + res_data_bytes_size;            // DataLength
            (*res_bytes)[4] = 1;                                                       // failed
            (*res_bytes)[5] = (uint8_t)ret;                                            // err
            (*res_bytes)[*res_bytes_size - 1] = calc_crc(*res_bytes, *res_bytes_size); // last byte CRC
        }
    }
    else
    {
        uint8_t res_data_bytes_size = 1;
        *res_bytes_size = packet_length_except_data + data_success_flag_size + res_data_bytes_size;
        *res_bytes = (uint8_t *)malloc(*res_bytes_size);
        (*res_bytes)[0] = header;
        (*res_bytes)[1] = command;
        (*res_bytes)[2] = sub_command;
        (*res_bytes)[3] = data_success_flag_size + res_data_bytes_size;            // DataLength
        (*res_bytes)[4] = 1;                                                       // failed
        (*res_bytes)[5] = (uint8_t)-2;                                             // err invalid header
        (*res_bytes)[*res_bytes_size - 1] = calc_crc(*res_bytes, *res_bytes_size); // last byte CRC
    }
    return 0;
}
