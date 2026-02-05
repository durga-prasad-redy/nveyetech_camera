#include <stdint.h>

int8_t set_config_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data);
int8_t set_config_defaulttofactory(const uint8_t data_length, const uint8_t *data);
int8_t set_config_defaulttocurrent(const uint8_t data_length, const uint8_t *data);
int8_t set_config_currenttofactory(const uint8_t data_length, const uint8_t *data);
int8_t set_config_currenttodefault(const uint8_t data_length, const uint8_t *data);
int8_t get_config_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_config_factory(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_config_default(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_config_current(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
