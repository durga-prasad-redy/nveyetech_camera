#include "motocam_helper_api_l1.h"
#include "motocam_audio_api_l1.h"
#include "motocam_command_enums.h"
#include "motocam_config_api_l1.h"
#include "motocam_image_api_l1.h"
#include "motocam_network_api_l1.h"
#include "motocam_streaming_api_l1.h"
#include "motocam_system_api_l1.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t get2sComplement(uint64_t num) { return (uint8_t)((num ^ 255) + 1); }

int8_t validate_req_bytes_crc(const uint8_t *req_bytes,
                              const uint8_t req_bytes_size) {
  printf("validate_req_bytes\n");
  uint64_t sum_req_bytes = 0;
  int i;
  for (i = 0; i < req_bytes_size; i++) {
    sum_req_bytes += req_bytes[i];
  }
  uint8_t lower_8_bits = (uint8_t)sum_req_bytes;
  if (lower_8_bits == 0)
    return 0;
  return -6; // validation error
}

uint8_t calc_crc(const uint8_t *res_bytes, const uint8_t res_bytes_size) {
  printf("calc_crc\n");
  uint64_t sum_res_bytes_except_crc = 0;
  int i;
  for (i = 0; i < res_bytes_size - 1; i++) {
    sum_res_bytes_except_crc += res_bytes[i];
  }
  return get2sComplement(sum_res_bytes_except_crc);
}

int8_t set_command(const uint8_t command, const uint8_t sub_command,
                   const uint8_t data_length, const uint8_t *data) {
  switch (command) {
  case STREAMING:
    return set_streaming_command(sub_command, data_length, data);
  case NETWORK:
    return set_network_command(sub_command, data_length, data);
  case CONFIG:
    return set_config_command(sub_command, data_length, data);
  case IMAGE:
    return set_image_command(sub_command, data_length, data);
  case AUDIO:
    return set_audio_command(sub_command, data_length, data);
  case SYSTEM:
    return set_system_command(sub_command, data_length, data);
  default:
    printf("invalid command\n");
    return -3;
  }
}

int8_t get_command(const uint8_t command, const uint8_t sub_command,
                   const uint8_t data_length, const uint8_t *data,
                   uint8_t **res_data_bytes, uint8_t *res_data_bytes_size) {
  switch (command) {
  case STREAMING:
    return get_streaming_command(sub_command, data_length, data, res_data_bytes,
                                 res_data_bytes_size);
  case NETWORK:
    return get_network_command(sub_command, data_length, data, res_data_bytes,
                               res_data_bytes_size);
  case CONFIG:
    return get_config_command(sub_command, data_length, data, res_data_bytes,
                              res_data_bytes_size);
  case IMAGE:
    return get_image_command(sub_command, data_length, data, res_data_bytes,
                             res_data_bytes_size);
  case AUDIO:
    return get_audio_command(sub_command, data_length, data, res_data_bytes,
                             res_data_bytes_size);
  case SYSTEM:
    return get_system_command(sub_command, data_length, data, res_data_bytes,
                              res_data_bytes_size);
  default:
    printf("invalid command\n");
    return -3;
  }
}
