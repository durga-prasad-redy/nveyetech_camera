#ifndef FW_STREAMING_H
#define FW_STREAMING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fw.h"
#include "fw/fw_image.h"
#include <ctype.h>

int8_t start_stream();
int8_t stop_stream();
int8_t get_stream_state();
int8_t get_stream_resolution(enum image_resolution *resolution, uint8_t stream_number);

int8_t get_stream_fps(uint8_t *fps, uint8_t stream_number);
int8_t get_stream_bitrate(uint8_t *bitrate, uint8_t stream_number);
int8_t get_stream_encoder(enum encoder_type *encoder1, uint8_t stream_number);


int8_t get_webrtc_streaming_state(uint8_t *webrtc_state);


char *trim(char *str);
int get_ini_value(const char *filename,
                  const char *section,
                  const char *key,
                  int default_value);
ImageResolution map_resolution(int width, int height);
Encoder map_encoder(int codec);

#ifdef __cplusplus
}
#endif

#endif
