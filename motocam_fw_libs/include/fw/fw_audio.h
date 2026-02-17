#ifndef FW_AUDIO_H
#define FW_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fw.h"

int8_t set_mic(enum ON_OFF on_off);
int8_t get_mic(enum ON_OFF *on_off);

#ifdef __cplusplus
}
#endif

#endif
