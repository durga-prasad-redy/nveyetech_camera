#include "motocam_streaming_api_l2.h"
#include "fw/fw_streaming.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t current_streaming_state; // 1-running, 2-not running

int8_t start_stream_l2() {
  printf("start_stream_l2 %d\n", current_streaming_state);

  start_stream();

  return 0;
}

int8_t stop_stream_l2() {
  printf("stop_stream_l2 %d\n", current_streaming_state);

  return 0;
}

int8_t get_stream_state_l2() {
  printf("get_stream_state_l2\n");
  return get_stream_state();
}

int8_t get_webrtc_streaming_state_l2(uint8_t **webrtc_state,
                                     uint8_t *webrtc_state_size) {
  printf("get_webrtc_streaming_state_l2\n");

  webrtc_state_size[0] = 1;
  webrtc_state[0] = (uint8_t *)malloc(webrtc_state_size[0]);
  int8_t ret = get_webrtc_streaming_state(&webrtc_state[0][0]);
  if (ret < 0) {
    printf("get_webrtc_streaming_state_l2: Failed to get webrtc streaming "
           "state\n");
    return -1;
  }
  return 0;
}

int8_t start_webrtc_streaming_state_l2() {

  printf("set_webrtc_enabled_l2 \n");
  int ret = start_webrtc_stream();
  if (ret < 0) {
    printf("set_webrtc_enabled_l2: Failed to start webrtc stream as misc is "
           "set to 4k mode\n");
    return -1;
  }
  return 0;
}

int8_t stop_webrtc_streaming_state_l2() {

  printf("set_webrtc_enabled_l2 \n");
  int ret = stop_webrtc_stream();
  if (ret < 0) {
    printf("set_webrtc_enabled_l2: Failed to start webrtc stream as misc is "
           "set to 4k mode\n");
    return -1;
  }
  return 0;
}
