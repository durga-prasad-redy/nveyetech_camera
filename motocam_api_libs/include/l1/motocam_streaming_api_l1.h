#include <stdint.h>

int8_t set_streaming_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data);
int8_t set_streaming_start(const uint8_t data_length, const uint8_t *data);
int8_t start_webrtc_streaming_l1(const uint8_t data_length, const uint8_t *data);
int8_t stop_webrtc_streaming_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_streaming_stop(const uint8_t data_length, const uint8_t *data);
int8_t get_streaming_state(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);

int8_t get_streaming_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data,
                             uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);

int8_t get_webrtc_streaming_state_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);