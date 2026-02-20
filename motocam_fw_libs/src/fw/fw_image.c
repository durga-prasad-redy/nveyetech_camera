#include "fw/fw_image.h"
#include "fw/fw_state_machine.h"
#include "fw/fw_system.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#define GET_IR_TEMP_STATE "cat " M5S_CONFIG_DIR "/ir_temp_state"
#define GET_ISP_TEMP_STATE "cat " M5S_CONFIG_DIR "/isp_temp_state"
#define GET_IR "cat " M5S_CONFIG_DIR "/ir_led_brightness"
#define GET_IRCUT_FILTER "cat " M5S_CONFIG_DIR "/ircut_filter"
#define GET_RESOLUTION "cat " M5S_CONFIG_DIR "/resolution"
#define GET_DAY_MODE "cat " M5S_CONFIG_DIR "/day_mode"
#define GET_GYRO_READER "cat " M5S_CONFIG_DIR "/gyro_reader"
#define GET_WDR "cat " M5S_CONFIG_DIR "/wdr"
#define GET_STREAM1_RESOLUTION "cat " M5S_CONFIG_DIR "/stream1_resolution"

#define GET_EIS "cat " M5S_CONFIG_DIR "/eis"
#define GET_FLIP "cat " M5S_CONFIG_DIR "/flip"
#define GET_MIRROR "cat " M5S_CONFIG_DIR "/mirror"
#define GET_ZOOM "cat " M5S_CONFIG_DIR "/image_zoom"
#define SET_MISC "misc"
#define IR "ir_led_brightness"
#define IR_TEMP_STATE "ir_temp_state"
#define ISP_TEMP_STATE "isp_temp_state"
#define IR_FILTER "ircut_filter"
#define RESOLUTION "resolution"
#define DAY_MODE "day_mode"
#define GYRO_READER "gyro_reader"
#define WDR "wdr"
#define EIS "eis"
#define STREAM1_RESOLUTION "stream1_resolution"
#define FLIP "flip"
#define MIRROR "mirror"
#define ZOOM "image_zoom"
#define TILT "tilt"
#define MISC_SOCK_PATH "/tmp/misc_change.sock"
#define IR_SOCK_PATH "/tmp/ir_change.sock"

static int ir_sock = -1;

static int misc_sock = -1;
static uint8_t last_misc = 0;
static int first_set_done = 0;

typedef struct {
  uint8_t old_misc;
  uint8_t new_misc;
} misc_event_t;

typedef struct {
  uint8_t old_ir;
  uint8_t new_ir;
} ir_event_t;

static void init_ir_socket_if_needed(void) {
  if (ir_sock >= 0)
    return;

  ir_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (ir_sock < 0) {
    LOG_ERROR("ir socket create failed errno=%d\n", errno);
  }
}

static void send_ir_event(uint8_t old_ir, uint8_t new_ir) {
  init_ir_socket_if_needed();

  if (ir_sock < 0)
    return;

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;

  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", IR_SOCK_PATH);
  ir_event_t evt;
  evt.old_ir = old_ir;
  evt.new_ir = new_ir;

  if (sendto(ir_sock, &evt, sizeof(evt), 0, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {

    if (errno == ENOENT || errno == ECONNREFUSED)
      LOG_DEBUG("ir event dropped (receiver not ready)\n");
    else
      LOG_ERROR("ir event send failed errno=%d\n", errno);
  }
}

static void handle_linear_mode(uint8_t day_mode, uint8_t eis,
                               uint8_t stream1_resolution) {
  if (day_mode == 0) {
    safe_symlink(RES_PATH "/Resource.Lowlight", RES_PATH "/Resource");
    linear_lowlight_on();
    return;
  }

  safe_symlink(RES_PATH "/Resource.Linear", RES_PATH "/Resource");

  if (stream1_resolution == 3) {
    linear_4k_on();
    return;
  }

  if (eis) {
    linear_eis_on();
  } else {
    linear_eis_off();
  }
}

void start_wdr_eis_mode(uint8_t day_mode) {
  uint8_t eis = 0;
  uint8_t wdr = 0;
  uint8_t stream1_resolution = 0;
  char output[64];

  if (exec_cmd(GET_EIS, output, sizeof(output)) == 0)
    eis = atoi(output);

  if (exec_cmd(GET_WDR, output, sizeof(output)) == 0)
    wdr = atoi(output);

  if (exec_cmd(GET_STREAM1_RESOLUTION, output, sizeof(output)) == 0)
    stream1_resolution = atoi(output);

  safe_remove(RES_PATH "/Resource");

  if (wdr) {
    safe_symlink(RES_PATH "/Resource.Fusion", RES_PATH "/Resource");
    safe_remove(RES_PATH "/Resource/AutoScene/autoscene_conf.cfg");

    if (day_mode == DAY_M) {
      safe_symlink(RES_PATH "/Resource/AutoScene/autoscene_conf.cfg.day",
                   RES_PATH "/Resource/AutoScene/autoscene_conf.cfg");
    } else if (day_mode == NIGHT_M) {
      safe_symlink(RES_PATH "/Resource/AutoScene/autoscene_conf.cfg.night",
                   RES_PATH "/Resource/AutoScene/autoscene_conf.cfg");
    }

    fusion_eis_off();
    return;
  }

  handle_linear_mode(day_mode, eis, stream1_resolution);
}

void do_action_for_1() {
  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 2);
  set_uboot_env(IR_FILTER, 0);

  outdu_update_brightness(0);
  update_ir_cut_filter_off();
  stream_server_config_2k_on();
  start_wdr_eis_mode(DAY_M);
}

void do_action_for_2() {
  set_uboot_env(EIS, 1);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 2);

  set_uboot_env(IR_FILTER, 0);
  outdu_update_brightness(0);
  update_ir_cut_filter_off();
  stream_server_config_2k_on();

  start_wdr_eis_mode(DAY_M);
}

void do_action_for_3() {
  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 1);
  set_uboot_env(STREAM1_RESOLUTION, 2);

  set_uboot_env(IR_FILTER, 0);
  outdu_update_brightness(0);
  update_ir_cut_filter_off();
  stream_server_config_2k_on();

  start_wdr_eis_mode(DAY_M);
}

void do_action_for_4() {

  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 3);

  set_uboot_env(IR_FILTER, 0);
  outdu_update_brightness(0);
  update_ir_cut_filter_off();
  stream_server_config_4k_on();

  start_wdr_eis_mode(DAY_M);
}

void do_action_for_5() {
  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 2);

  set_uboot_env(IR_FILTER, 1);
  outdu_update_brightness(0);
  update_ir_cut_filter_on();
  stream_server_config_2k_on();

  start_wdr_eis_mode(LOW_LIGHT_M);
}

void do_action_for_6() {
  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 2);

  set_uboot_env(IR_FILTER, 1);
  outdu_update_brightness(0);
  update_ir_cut_filter_on();
  stream_server_config_2k_on();

  start_wdr_eis_mode(LOW_LIGHT_M);
}

void do_action_for_7() {
  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 2);

  set_uboot_env(IR_FILTER, 1);
  outdu_update_brightness(0);
  update_ir_cut_filter_on();
  stream_server_config_2k_on();

  start_wdr_eis_mode(LOW_LIGHT_M);
}

void do_action_for_8() {
  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 2);

  set_uboot_env(IR_FILTER, 1);
  outdu_update_brightness(0);
  update_ir_cut_filter_on();
  stream_server_config_2k_on();

  start_wdr_eis_mode(LOW_LIGHT_M);
}

void do_action_for_9() {
  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 2);

  set_uboot_env(IR_FILTER, 1);
  outdu_update_brightness(MAX);
  update_ir_cut_filter_on();
  stream_server_config_2k_on();

  start_wdr_eis_mode(NIGHT_M);
}

void do_action_for_10() {
  set_uboot_env(EIS, 1);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 2);

  set_uboot_env(IR_FILTER, 1);
  outdu_update_brightness(MAX);
  update_ir_cut_filter_on();
  stream_server_config_2k_on();

  start_wdr_eis_mode(NIGHT_M);
}

void do_action_for_11() {
  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 1);
  set_uboot_env(STREAM1_RESOLUTION, 2);

  set_uboot_env(IR_FILTER, 1);
  outdu_update_brightness(MAX);
  update_ir_cut_filter_on();
  stream_server_config_2k_on();

  start_wdr_eis_mode(NIGHT_M);
}

void do_action_for_12() {
  set_uboot_env(EIS, 0);
  set_uboot_env(WDR, 0);
  set_uboot_env(STREAM1_RESOLUTION, 3);

  set_uboot_env(IR_FILTER, 1);
  outdu_update_brightness(MAX);
  update_ir_cut_filter_on();
  stream_server_config_4k_on();

  start_wdr_eis_mode(NIGHT_M);
}

void set_misc(uint8_t misc) {

  printf("fw set_misc %d\n", misc);
  switch (misc) {
  case DAY_EIS_OFF_WDR_OFF:
    do_action_for_1();
    break;
  case DAY_EIS_ON_WDR_OFF:
    do_action_for_2();
    break;
  case DAY_EIS_OFF_WDR_ON:
    do_action_for_3();
    break;
  case DAY_EIS_ON_WDR_ON:
    do_action_for_4();
    break;
  case LOWLIGHT_EIS_OFF_WDR_OFF:
    do_action_for_5();
    break;
  case LOWLIGHT_EIS_ON_WDR_OFF:
    do_action_for_6();
    break;
  case LOWLIGHT_EIS_OFF_WDR_ON:
    do_action_for_7();
    break;
  case LOWLIGHT_EIS_ON_WDR_ON:
    do_action_for_8();
    break;
  case NIGHT_EIS_OFF_WDR_OFF:
    do_action_for_9();
    break;
  case NIGHT_EIS_ON_WDR_OFF:
    do_action_for_10();
    break;
  case NIGHT_EIS_OFF_WDR_ON:
    do_action_for_11();
    break;
  case NIGHT_EIS_ON_WDR_ON:
    do_action_for_12();
    break;
  default:
    fprintf(stderr, "Invalid input\n");

    return;
  }

  if (misc == DAY_EIS_ON_WDR_ON || misc == NIGHT_EIS_ON_WDR_ON) {
    if (is_running(SIGNALING_SERVER_PROCESS_NAME))
      stop_process(SIGNALING_SERVER_PROCESS_NAME);
    if (is_running(PORTABLE_RTC_PROCESS_NAME))
      stop_process(PORTABLE_RTC_PROCESS_NAME);
    set_uboot_env(WEBRTC_ENABLED, 0);
  }

  if (is_running(STREAMER_PROCESS_NAME)) {
    stop_process(STREAMER_PROCESS_NAME);

    /* Non-blocking wait for streamer restart */
    misc_sm_ctx_t sm;
    misc_sm_init(&sm);

    fw_sm_status_t sm_status;
    do {
      sm_status = misc_sm_step(&sm);
      if (sm_status == FW_SM_RUNNING)
        fw_sm_yield();
    } while (sm_status == FW_SM_RUNNING);
  }
}

int8_t set_image_zoom(uint8_t image_zoom) {
  char command[256];
  const char *value = NULL;
  int status;

  LOG_DEBUG("fw set_image_zoom %d\n", image_zoom);
  pthread_mutex_lock(&lock);

  switch (image_zoom) {
  case 1:
    value = "1";
    break;
  case 2:
    value = "0.5";
    break;
  case 3:
    value = "0.4";
    break;
  case 4:
    value = "0.1";
    break;
  default:
    LOG_ERROR("Invalid option\n");
    pthread_mutex_unlock(&lock);
    return -1;
  }

  set_uboot_env(ZOOM, image_zoom);

  snprintf(command, sizeof(command), "streamer_msg_sender -C 0 -s 0 -z %s",
           value);
  status = system(command);
  if (status != 0) {
    LOG_ERROR("Command execution failed with status %d\n", status);
  }

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_image_rotation(uint8_t image_rotation) {
  LOG_DEBUG("fw set_image_rotation %d\n", image_rotation);
  (void)image_rotation;
  pthread_mutex_lock(&lock);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_ir_cutfilter(OnOff on_off) {
  LOG_DEBUG("fw set_ir_cutfilter %d\n", on_off);
  pthread_mutex_lock(&lock);

  if (on_off == ON) {
    update_ir_cut_filter_on();
  } else {
    update_ir_cut_filter_off();
  }

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_ir_led_brightness(uint8_t brightness) {
  LOG_INFO("fw set_ir_led_brightness %d\n", brightness);
  uint8_t last_ir = 0;
  get_ir_led_brightness(&last_ir);

  pthread_mutex_lock(&lock);

  outdu_update_brightness(brightness);

  /* Notify after change */
  LOG_INFO("ir brightness changed: %u -> %u\n", last_ir, brightness);
  send_ir_event(last_ir, brightness);

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_ir_temp_state(uint8_t state) {
  LOG_DEBUG("fw set_ir_temp_state %d\n", state);
  pthread_mutex_lock(&lock);

  set_uboot_env(IR_TEMP_STATE, state);

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_isp_temp_state(uint8_t state) {
  LOG_DEBUG("fw set_isp_temp_state %d\n", state);
  pthread_mutex_lock(&lock);

  set_uboot_env(ISP_TEMP_STATE, state);

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_day_mode(enum ON_OFF on_off) {
  LOG_DEBUG("fw set_day_night %d\n", on_off);
  pthread_mutex_lock(&lock);

  set_uboot_env(DAY_MODE, on_off);

  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_gyro_reader(enum ON_OFF on_off)

{

  LOG_DEBUG("fw set_gyro_reader %d\n", on_off);

  pthread_mutex_lock(&lock);

  set_uboot_env(GYRO_READER, on_off);

  pthread_mutex_unlock(&lock);

  return 0;
}
int8_t set_image_resolution(uint8_t resolution) {
  LOG_DEBUG("fw set_image_resolution %d\n", resolution);
  pthread_mutex_lock(&lock);

  if (resolution == 0 || resolution == 1) {
    set_uboot_env(RESOLUTION, resolution);
  } else {
    LOG_ERROR("Invalid resolution value is selected Valid range: 0 to 1\n");
  }

  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t set_image_mirror(uint8_t mirror) {
  int result = 1;
  char cmd[64];

  LOG_DEBUG("fw set_image_mirror %d\n", mirror);
  pthread_mutex_lock(&lock);

  if (mirror == 0 || mirror == 1) {
    set_uboot_env(MIRROR, mirror);

    snprintf(cmd, sizeof(cmd), "streamer_msg_sender -C 2 --mirror %d", mirror);
    result = system(cmd);
    if (result != 0) {
      LOG_ERROR("Command failed with exit code %d\n", result);
    }
  } else {
    LOG_ERROR("Invalid mirror value is selected Valid range: 0 to 1\n");
  }

  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t set_image_flip(uint8_t flip) {
  int result = 1;
  char cmd[64];

  LOG_DEBUG("fw set_image_flip %d\n", flip);
  pthread_mutex_lock(&lock);

  if (flip == 0 || flip == 1) {

    set_uboot_env(FLIP, flip);

    snprintf(cmd, sizeof(cmd), "streamer_msg_sender -C 2 --flip %d", flip);
    result = system(cmd);
    if (result != 0) {
      LOG_ERROR("Command failed with exit code %d\n", result);
    }
  } else {
    LOG_ERROR("Invalid flip value is selected Valid range: 0 to 1\n");
  }

  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t set_image_tilt(uint8_t tilt) {
  LOG_DEBUG("fw set_image_tilt %d\n", tilt);
  pthread_mutex_lock(&lock);

  if (tilt <= 5) {
    set_uboot_env(TILT, tilt);
  } else {
    LOG_ERROR("Invalid tilt value is selected Valid range: 0 to 5\n");
  }
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_image_wdr(uint8_t wdr) {
  LOG_DEBUG("fw set_image_wdr %d\n", wdr);
  pthread_mutex_lock(&lock);

  if (wdr == 1 || wdr == 0) {
    set_uboot_env(WDR, wdr);
  } else {
    LOG_ERROR("Invalid wdr value\n");
  }

  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t set_image_eis(uint8_t eis) {
  LOG_DEBUG("fw set_image_eis %d\n", eis);
  pthread_mutex_lock(&lock);

  set_uboot_env(EIS, eis);

  pthread_mutex_unlock(&lock);
  return 0;
}

static void init_last_misc(void) {
  uint8_t misc = 0;
  if (get_image_misc(&misc) == 0) {
    last_misc = misc;
    LOG_DEBUG("misc init: current misc = %u\n", last_misc);
  } else {
    LOG_ERROR("misc init failed, defaulting to 0\n");
    last_misc = 0;
  }
}

static void init_misc_socket_if_needed(void) {
  if (misc_sock >= 0)
    return;

  misc_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (misc_sock < 0) {
    LOG_ERROR("misc socket create failed errno=%d\n", errno);
  }
}

static void send_misc_event(uint8_t old_misc, uint8_t new_misc) {
  init_misc_socket_if_needed();

  if (misc_sock < 0)
    return;

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;

  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", MISC_SOCK_PATH);
  misc_event_t evt;
  evt.old_misc = old_misc;
  evt.new_misc = new_misc;

  if (sendto(misc_sock, &evt, sizeof(evt), 0, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {

    if (errno == ENOENT || errno == ECONNREFUSED)
      LOG_DEBUG("misc event dropped (receiver not ready)\n");
    else
      LOG_ERROR("misc event send failed errno=%d\n", errno);
  }
}

int8_t set_image_misc(uint8_t misc) {
  init_last_misc();

  /* No-op if misc is unchanged */
  if (first_set_done && misc == last_misc) {
    LOG_DEBUG("set_image_misc: misc %u unchanged, skipping\n", misc);
    return 0;
  }

  LOG_DEBUG("fw set_image_misc %d\n", misc);
  pthread_mutex_lock(&lock);

  char webrtc_enabled[64];
  uint8_t webrtc_en = 0;

  if (exec_cmd(GET_WEBRTC_ENABLED, webrtc_enabled, sizeof(webrtc_enabled)) ==
      0) {
    webrtc_en = (uint8_t)atoi(webrtc_enabled);
  }
  if (webrtc_en == 1 &&
      (misc == DAY_EIS_ON_WDR_ON || misc == NIGHT_EIS_ON_WDR_ON)) {
    printf("[INFO] WebRTC is enabled. Cannot set misc to %d\n", misc);
    pthread_mutex_unlock(&lock);
    return -1;
  }

  /* Notify what will change change */
  LOG_INFO("misc changed: %u -> %u\n", last_misc, misc);
  send_misc_event(last_misc, misc);

  set_uboot_env(SET_MISC, misc);
  set_misc(misc);
  pthread_mutex_unlock(&lock);

  /* Notify AFTER successful change */
  LOG_INFO("misc changed: %u -> %u\n", last_misc, misc);
  send_misc_event(0, 0);

  first_set_done = 1;
  return 0;
}

int8_t get_ir_led_brightness(uint8_t *brightness) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_IR, brightness);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_ir_temp_state(uint8_t *ir_temp_state) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_IR_TEMP_STATE, ir_temp_state);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_isp_temp_state(uint8_t *isp_temp_state) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_ISP_TEMP_STATE, isp_temp_state);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_image_resolution(uint8_t *resolution) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_RESOLUTION, resolution);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_wdr(uint8_t *wdr) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_WDR, wdr);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_eis(uint8_t *eis) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_EIS, eis);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_flip(uint8_t *flip) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_FLIP, flip);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_mirror(uint8_t *mirror) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_MIRROR, mirror);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_image_zoom(uint8_t *zoom) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_ZOOM, zoom);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_day_mode(uint8_t *day_mode) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_DAY_MODE, day_mode);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_gyro_reader(uint8_t *gyro_reader) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_GYRO_READER, gyro_reader);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_image_misc(uint8_t *misc) {
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_MISC, misc);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_ir_cutfilter(OnOff *on_off) {
  uint8_t value;
  pthread_mutex_lock(&lock);

  value = get_ir_cut_filter();

  if (value == 1) {
    *on_off = OFF;
  } else {
    *on_off = ON;
  }

  pthread_mutex_unlock(&lock);
  return 0;
}
