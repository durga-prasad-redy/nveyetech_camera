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



void set_misc_actions_on_boot(uint8_t misc)
{

  printf("fw set_misc %d\n", misc);
  switch (misc)
  {
  case DAY_EIS_OFF_WDR_OFF:
    printf("misc: DAY_EIS_OFF_WDR_OFF\n");
    outdu_update_brightness(0);
    update_ir_cut_filter_off();
    break;
  case DAY_EIS_ON_WDR_OFF:
    printf("misc: DAY_EIS_ON_WDR_OFF\n");
    outdu_update_brightness(0);
    update_ir_cut_filter_off();
    break;
  case DAY_EIS_OFF_WDR_ON:
    printf("misc: DAY_EIS_OFF_WDR_ON\n");
    outdu_update_brightness(0);
    update_ir_cut_filter_off();
    break;
  case DAY_EIS_ON_WDR_ON:
    printf("misc: DAY_EIS_ON_WDR_ON\n");
    outdu_update_brightness(0);
    update_ir_cut_filter_off();
    break;
  case LOWLIGHT_EIS_OFF_WDR_OFF:
      printf("misc: LOWLIGHT_EIS_OFF_WDR_OFF\n");
    outdu_update_brightness(0);
    update_ir_cut_filter_on();
    break;
  case LOWLIGHT_EIS_ON_WDR_OFF:
    printf("misc: LOWLIGHT_EIS_ON_WDR_OFF\n");
    outdu_update_brightness(0);
    update_ir_cut_filter_on();
    break;
  case LOWLIGHT_EIS_OFF_WDR_ON:
    printf("misc: LOWLIGHT_EIS_OFF_WDR_ON\n");
    outdu_update_brightness(0);
    update_ir_cut_filter_on();
    break;
  case LOWLIGHT_EIS_ON_WDR_ON:
    printf("misc: LOWLIGHT_EIS_ON_WDR_ON\n");
    outdu_update_brightness(0);
    update_ir_cut_filter_on();
    break;
  case NIGHT_EIS_OFF_WDR_OFF:
    printf("misc: NIGHT_EIS_OFF_WDR_OFF\n");
    outdu_update_brightness(MAX);
    update_ir_cut_filter_on();
    break;
  case NIGHT_EIS_ON_WDR_OFF:
    printf("misc: NIGHT_EIS_ON_WDR_OFF\n");
    outdu_update_brightness(MAX);
    update_ir_cut_filter_on();
    break;
  case NIGHT_EIS_OFF_WDR_ON:
    printf("misc: NIGHT_EIS_OFF_WDR_ON\n");
    outdu_update_brightness(MAX);
    update_ir_cut_filter_on();
    break;
  case NIGHT_EIS_ON_WDR_ON:
    printf("misc: NIGHT_EIS_ON_WDR_ON\n");
    outdu_update_brightness(MAX);
    update_ir_cut_filter_on();
    break;
  default:
    fprintf(stderr, "Invalid input\n");

    return;
  }
}

int8_t init_motocam_configs() {
  printf("init_motocam_configs\n");

  initialize_config(&current_config);

  if (current_config.daymode == 1)
    set_day_mode(ON);
  if (current_config.daymode == 0)
    set_day_mode(OFF);

  set_misc_actions_on_boot(current_config.misc);

  stop_webrtc_stream();

  return 0;
}
