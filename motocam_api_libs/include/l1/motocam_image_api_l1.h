#include <stdint.h>

int8_t set_image_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data);
int8_t set_image_zoom_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_rotation_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_ircutfilter_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_irbrightness_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_resolution_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_daymode_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_gyroreader_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_mirror_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_flip_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_tilt_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_wdr_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_eis_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_misc_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_mid_irbrightness_l1(const uint8_t data_length, const uint8_t *data);
int8_t set_image_side_irbrightness_l1(const uint8_t data_length, const uint8_t *data);



int8_t get_image_command(const uint8_t sub_command, const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_zoom_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_rotation_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_ircutfilter_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_irbrightness_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_daymode_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_gyroreader_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_resolution_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_mirror_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_flip_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_tilt_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_wdr_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
int8_t get_image_eis_l1(const uint8_t data_length, const uint8_t *data, uint8_t **res_data_bytes, uint8_t *res_data_bytes_size);
