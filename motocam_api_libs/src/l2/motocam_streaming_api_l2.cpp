#include <cstdio>
#include <memory>
#include <new>
#include "motocam_streaming_api_l2.h"
#include "fw/fw_streaming.h"

static uint8_t current_streaming_state; // 1-running, 2-not running

int8_t start_stream_l2()
{
  printf("start_stream_l2 %d\n", current_streaming_state);

  start_stream();

  return 0;
}

int8_t stop_stream_l2()
{
  printf("stop_stream_l2 %d\n", current_streaming_state);

  return 0;
}

int8_t get_stream_state_l2()
{
  printf("get_stream_state_l2\n");
  return get_stream_state();
}

int8_t get_webrtc_streaming_state_l2(uint8_t **webrtc_state, uint8_t *webrtc_state_size) {
    printf("get_webrtc_streaming_state_l2\n");

    // 1. Use std::string to manage the temporary buffer
    // Initialize with a dummy size or the expected size
    std::string state_buffer(1, '\0'); 

    // 2. Call the underlying function
    // We cast the string's internal data pointer to uint8_t*
    int8_t ret = get_webrtc_streaming_state(reinterpret_cast<uint8_t*>(&state_buffer[0]));

    if (ret < 0) {
        printf("get_webrtc_streaming_state_l2: Failed to get webrtc streaming state\n");
        return -1;
    }

    // 3. Update the size output
    webrtc_state_size[0] = static_cast<uint8_t>(state_buffer.size());

    // 4. Transfer ownership to the caller
    // Since the caller expects a raw pointer they can own, we must allocate 
    // a buffer that persists after this function returns.
    uint8_t* final_out = new (std::nothrow) uint8_t[state_buffer.size()];
    if (!final_out) return -1;

    std::copy(state_buffer.begin(), state_buffer.end(), final_out);
    webrtc_state[0] = final_out;

    return 0;
}
int8_t start_webrtc_streaming_state_l2()
{

  printf("set_webrtc_enabled_l2 \n");
  auto ret=start_webrtc_stream();
  if(ret<0) {
      printf("set_webrtc_enabled_l2: Failed to start webrtc stream as misc is set to 4k mode\n");
      return -1;
  }
  return 0;
}

int8_t stop_webrtc_streaming_state_l2()
{

  printf("set_webrtc_enabled_l2 \n");
  int ret=stop_webrtc_stream();
  if(ret<0) {
      printf("set_webrtc_enabled_l2: Failed to start webrtc stream as misc is set to 4k mode\n");
      return -1;
  }
  return 0;
}

