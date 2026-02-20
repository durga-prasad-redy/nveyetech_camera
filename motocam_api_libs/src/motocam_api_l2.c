//
// Created by sr on 2/6/25.
//
#include "motocam_api_l2.h"
#include "fw/fw_audio.h" // for set_mic()
#include "fw/fw_image.h" // for set_image_zoom, rotation, tilt, misc, day_mode
#include "fw/fw_streaming.h"          // for stop_webrtc_stream()
#include "l2/motocam_config_api_l2.h" // for initialize_config()
#include "l2/motocam_image_api_l2.h"  // for set_image_ircutfilter_l2()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fw.h"

struct MotocamConfig factory_config;
struct MotocamConfig default_config;
struct MotocamConfig current_config;

int8_t init_motocam_configs() {
  printf("init_motocam_configs\n");

  initialize_config(&current_config);
  set_image_zoom(current_config.zoom);

  set_image_rotation(current_config.rotation);

  set_image_ircutfilter_l2(OFF);
  if (current_config.daymode == 1)
    set_day_mode(ON);
  if (current_config.daymode == 0)
    set_day_mode(OFF);

  set_image_tilt(current_config.tilt);

  set_image_misc(current_config.misc);
  stop_webrtc_stream();

  return 0;
}
