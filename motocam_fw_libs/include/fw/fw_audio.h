#ifndef FW_AUDIO_H
#define FW_AUDIO_H
#include "fw.h"

int8_t set_mic(enum ON_OFF on_off);
int8_t get_mic(enum ON_OFF *on_off);

#endif
