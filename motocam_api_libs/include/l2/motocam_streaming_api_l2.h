#include <stdint.h>

int8_t start_stream_l2();
int8_t stop_stream_l2();
int8_t get_stream_state_l2(); // Started=1, Stopped=2
int8_t start_webrtc_streaming_state_l2();
int8_t stop_webrtc_streaming_state_l2();

int8_t get_webrtc_streaming_state_l2(uint8_t **webrtc_state,uint8_t *webrtc_state_size);
