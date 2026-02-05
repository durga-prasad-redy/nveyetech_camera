//
// Created by sr on 2/6/25.
//
#include "motocam_api_l2.h"
#include "l2/motocam_config_api_l2.h"   // for initialize_config()
#include "l2/motocam_image_api_l2.h"    // for set_image_ircutfilter_l2()
#include "fw/fw_image.h"                 // for set_image_zoom, rotation, tilt, misc, day_mode
#include "fw/fw_audio.h"                 // for set_mic()
#include "fw/fw_streaming.h"             // for stop_webrtc_stream()
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
  //    set_ir_led_brightness(current_config.irledbrightness);
  // set_image_irbrightness_l2(current_config.irledbrightness);
  set_image_rotation(current_config.rotation);
  //    if(current_config.ircutfilter==1)
  //        set_ir_cutfilter(ON);
  //    if(current_config.ircutfilter==0)
  set_image_ircutfilter_l2(OFF);
  if (current_config.daymode == 1)
    set_day_mode(ON);
  if (current_config.daymode == 0)
    set_day_mode(OFF);
  //    set_image_mirror(current_config.mirror);
  //    set_image_mirror(0);
  //    set_image_flip(current_config.flip);
  //    set_image_flip(0);
  set_image_tilt(current_config.tilt);
  if (current_config.mic == 1)
    set_mic(ON);
  if (current_config.mic == 0)
    set_mic(OFF);
  set_image_misc(current_config.misc);
  stop_webrtc_stream();

   
  return 0;
}
