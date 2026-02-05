#include <stdint.h>
#include "motocam_api_l2.h"
#include "fw/fw_streaming.h"

int8_t set_config_defaulttofactory_l2();
int8_t set_config_defaulttocurrent_l2();
int8_t set_config_currenttofactory_l2();
int8_t set_config_currenttodefault_l2();

int8_t get_config_factory_l2(uint8_t **config, uint8_t *length);
int8_t get_config_default_l2(uint8_t **config, uint8_t *length);
int8_t get_config_current_l2(uint8_t **config, uint8_t *length);




void initialize_config(MotocamConfig *config);
int8_t get_config_streaming_config_l2(uint8_t **config, uint8_t *length);
