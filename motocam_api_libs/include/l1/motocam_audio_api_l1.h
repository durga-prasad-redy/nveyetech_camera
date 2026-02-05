#include <stdint.h>
#include <cstdio>
#include <motocam_command_enums.h>

int8_t set_audio_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data);
int8_t set_audio_mic_l1(const uint8_t data_length, const uint8_t *data);
int8_t get_audio_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_audio_mic_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
