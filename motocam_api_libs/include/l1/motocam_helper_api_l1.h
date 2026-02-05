#include <stdint.h>

uint8_t get2sComplement(uint64_t num);
int8_t validate_req_bytes_crc(const uint8_t *req_bytes, const uint8_t req_bytes_size);
uint8_t calc_crc(const uint8_t *res_bytes, const uint8_t res_bytes_size);
int8_t set_command(const uint8_t command, const uint8_t sub_command, const uint8_t data_length, const uint8_t *data);
int8_t get_command(const uint8_t command, const uint8_t sub_command, const uint8_t data_length, const uint8_t *data, uint8_t **res_bytes, uint8_t *res_bytes_size);
