#include <stdint.h>

int8_t set_image_zoom_l2(const uint8_t zoom);
int8_t set_image_rotation_l2(const uint8_t rotation);
int8_t set_image_ircutfilter_l2(const uint8_t ircutfilter);
int8_t set_image_irbrightness_l2(const uint8_t irbrightness);
int8_t set_image_mid_irbrightness_l2(const uint8_t irbrightness);
int8_t set_image_side_irbrightness_l2(const uint8_t irbrightness);

int8_t set_image_daymode_l2(const uint8_t daymode);
int8_t set_gyroreader_l2(const uint8_t gyroreader);
int8_t set_image_resolution_l2(const uint8_t resolution);
int8_t set_image_mirror_l2(const uint8_t mirror);
int8_t set_image_flip_l2(const uint8_t flip);
int8_t set_image_tilt_l2(const uint8_t tilt);
int8_t set_image_wdr_l2(const uint8_t wdr);
int8_t set_image_eis_l2(const uint8_t eis);
int8_t set_image_misc_l2(const uint8_t misc);

int8_t get_image_zoom_l2(uint8_t **zoom, uint8_t *length);
int8_t get_image_rotation_l2(uint8_t **rotation, uint8_t *length);
int8_t get_image_ircutfilter_l2(uint8_t **ircutfilter, uint8_t *length);
int8_t get_image_irbrightness_l2(uint8_t **irbrightness, uint8_t *length);
int8_t get_image_daymode_l2(uint8_t **daymode, uint8_t *length);
int8_t get_gyroreader_l2(uint8_t **daymode, uint8_t *length);
int8_t get_image_resolution_l2(uint8_t **mirror, uint8_t *length);
int8_t get_wdr_l2(uint8_t **mirror, uint8_t *length);
int8_t get_eis_l2(uint8_t **mirror, uint8_t *length);
int8_t get_image_mirror_l2(uint8_t **mirror, uint8_t *length);
int8_t get_image_flip_l2(uint8_t **flip, uint8_t *length);
int8_t get_image_tilt_l2(uint8_t **tilt, uint8_t *length);