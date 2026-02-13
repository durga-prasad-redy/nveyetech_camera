#include <stdint.h>
#include "fw/fw_audio.h"

int8_t set_mic(enum ON_OFF on_off) { (void)on_off; return 0; }
int8_t get_mic(enum ON_OFF *on_off) { (void)on_off; return 0; }
