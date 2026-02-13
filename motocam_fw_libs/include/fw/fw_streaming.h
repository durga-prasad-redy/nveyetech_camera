#ifndef FW_STREAMING_H
#define FW_STREAMING_H
#include "fw.h"
#include "fw/fw_image.h"
#include <cstring>
#include <string>
#include <fstream>



int8_t start_stream();
int8_t stop_stream();
int8_t get_stream_state();
int8_t get_stream_resolution(enum image_resolution *resolution, uint8_t stream_number);

int8_t get_stream_fps(uint8_t *fps, uint8_t stream_number);
int8_t get_stream_bitrate(uint8_t *bitrate, uint8_t stream_number);
int8_t get_stream_encoder(enum encoder_type *encoder1, uint8_t stream_number);


int8_t get_webrtc_streaming_state(uint8_t *webrtc_state);


 std::string trim(const std::string& str) ;
 int get_ini_value(const std::string& filename, const std::string& section, const std::string& key, int default_value);
     Encoder map_encoder(int codec);
     ImageResolution map_resolution(int width, int height);


#endif
