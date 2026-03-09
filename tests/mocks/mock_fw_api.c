/*
 * Mock implementations of fw (firmware) layer symbols required by motocam_api_libs.
 * Used only for unit testing the API libs without hardware or full fw stack.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "mock_fw_control.h"
#include "mock_fw_extra.h"

/* Configurable mock state (set by tests via mock_fw_control.h) */
static uint8_t mock_day_mode = 1;
static uint8_t mock_image_misc = 1;
static int8_t mock_stream_state = 1;
static int mock_set_image_zoom_fail = 0;
static int mock_set_image_misc_fail = 0;
static int mock_get_webrtc_fail = 0;
static int mock_get_ir_cutfilter_fail = 0;
static int mock_get_gyro_reader_fail = 0;
static int mock_get_image_resolution_fail = 0;
static int mock_get_day_mode_fail = 0;
static int mock_start_stream_fail = 0;
static int mock_stop_stream_fail = 0;
static int mock_start_webrtc_stream_fail = 0;
static int mock_stop_webrtc_stream_fail = 0;
static uint8_t mock_wifi_state_value = 0;
static int mock_fail_image_set = 0;
static int mock_fail_image_get = 0;
static int mock_fail_network_fw = 0;
static int mock_fail_system_fw = 0;
static int8_t mock_set_factory_reset_ret = 0;
static int8_t mock_set_config_reset_ret = 0;
static int8_t mock_set_login_ret = 0;
static uint8_t mock_login_pin_mode = 0;
static uint8_t mock_wifi_country_code_mode = 0;
static uint8_t mock_network_config_nonempty = 0;

void set_mock_day_mode(uint8_t value) { mock_day_mode = value; }
void set_mock_get_day_mode_fail(int fail) { mock_get_day_mode_fail = fail; }
void set_mock_image_misc(uint8_t value) { mock_image_misc = value; }
void set_mock_stream_state(int8_t value) { mock_stream_state = value; }
void set_mock_set_image_zoom_fail(int fail) { mock_set_image_zoom_fail = fail; }
void set_mock_set_image_misc_fail(int fail) { mock_set_image_misc_fail = fail; }
void set_mock_get_webrtc_fail(int fail) { mock_get_webrtc_fail = fail; }
void set_mock_get_ir_cutfilter_fail(int fail) { mock_get_ir_cutfilter_fail = fail; }
void set_mock_get_gyro_reader_fail(int fail) { mock_get_gyro_reader_fail = fail; }
void set_mock_get_image_resolution_fail(int fail) { mock_get_image_resolution_fail = fail; }
void set_mock_wifi_state(uint8_t value) { mock_wifi_state_value = value; }
void set_mock_start_stream_fail(int fail) { mock_start_stream_fail = fail; }
void set_mock_stop_stream_fail(int fail) { mock_stop_stream_fail = fail; }
void set_mock_start_webrtc_stream_fail(int fail) { mock_start_webrtc_stream_fail = fail; }
void set_mock_stop_webrtc_stream_fail(int fail) { mock_stop_webrtc_stream_fail = fail; }
void set_mock_fail_image_set(int fail) { mock_fail_image_set = fail; }
void set_mock_fail_image_get(int fail) { mock_fail_image_get = fail; }
void set_mock_fail_network_fw(int fail) { mock_fail_network_fw = fail; }
void set_mock_fail_system_fw(int fail) { mock_fail_system_fw = fail; }
void set_mock_set_factory_reset_ret(int8_t ret) { mock_set_factory_reset_ret = ret; }
void set_mock_set_config_reset_ret(int8_t ret) { mock_set_config_reset_ret = ret; }
void set_mock_set_login_ret(int8_t ret) { mock_set_login_ret = ret; }
void set_mock_login_pin_mode(uint8_t mode) { mock_login_pin_mode = mode; }
void set_mock_wifi_country_code_mode(uint8_t mode) { mock_wifi_country_code_mode = mode; }
void set_mock_network_config_nonempty(uint8_t mode) { mock_network_config_nonempty = mode; }
void reset_mock_fw_control(void) {
  mock_day_mode = 1;
  mock_image_misc = 1;
  mock_stream_state = 1;
  mock_set_image_zoom_fail = 0;
  mock_set_image_misc_fail = 0;
  mock_get_webrtc_fail = 0;
  mock_get_ir_cutfilter_fail = 0;
  mock_get_gyro_reader_fail = 0;
  mock_get_image_resolution_fail = 0;
  mock_get_day_mode_fail = 0;
  mock_start_stream_fail = 0;
  mock_stop_stream_fail = 0;
  mock_start_webrtc_stream_fail = 0;
  mock_stop_webrtc_stream_fail = 0;
  mock_wifi_state_value = 0;
  mock_fail_image_set = 0;
  mock_fail_image_get = 0;
  mock_fail_network_fw = 0;
  mock_fail_system_fw = 0;
  mock_set_factory_reset_ret = 0;
  mock_set_config_reset_ret = 0;
  mock_set_login_ret = 0;
  mock_login_pin_mode = 0;
  mock_wifi_country_code_mode = 0;
  mock_network_config_nonempty = 0;
}

/* Define paths before including fw.h so test paths are used */
#ifndef CONFIG_PATH
#define CONFIG_PATH "/tmp/test_api/config"
#endif
#ifndef M5S_CONFIG_DIR
#define M5S_CONFIG_DIR "/tmp/test_api/m5s_config"
#endif
#ifndef RES_PATH
#define RES_PATH "/tmp/test_api"
#endif

#include "fw.h"
#include "fw/fw_image.h"
#include "fw/fw_sensor.h"
#include "fw/fw_streaming.h"
#include "fw/fw_network.h"
#include "fw/fw_system.h"
#include "fw/fw_audio.h"
#include "log.h"

pthread_mutex_t lock;

/* ---- mock_fw_extra ---- */
int8_t apply_video_frequency_change(VideoFrequency *freq) {
  (void)freq;
  return 0;
}
int8_t get_video_frequency(VideoFrequency *freq) {
  if (freq)
    *freq = F50;
  return 0;
}

/* ---- gpio.h ---- */
int gpio_init(void) { return 0; }
void update_ir_cut_filter_off(void) {}
void update_ir_cut_filter_on(void) {}
uint8_t get_ir_cut_filter(void) { return 0; }

/* ---- fw.h ---- */
int exec_cmd(const char *cmd, char *out, size_t out_size) {
  (void)cmd;
  if (out && out_size)
    out[0] = '\0';
  return 0;
}
int8_t outdu_update_brightness(uint8_t val) { (void)val; return 0; }
void safe_remove(const char *path) { (void)path; }
void safe_symlink(const char *target, const char *linkpath) {
  (void)target;
  (void)linkpath;
}
uint8_t access_file(const char *path) { (void)path; return 1; }
int exec_return(const char *cmd) { (void)cmd; return 0; }
int8_t set_uboot_env(const char *key, uint8_t value) {
  (void)key;
  (void)value;
  return 0;
}
int is_running(const char *process_name) { (void)process_name; return 0; }
void stop_process(const char *process_name) { (void)process_name; }
int8_t set_haptic_motor(int duty_cycle, int duration) {
  (void)duty_cycle;
  (void)duration;
  return mock_fail_system_fw ? -1 : 0;
}
int8_t set_uboot_env_chars(const char *key, const char *value) {
  (void)key;
  (void)value;
  return 0;
}
void kill_all_processes(void) {}
void fusion_eis_on(void) {}
void fusion_eis_off(void) {}
void linear_eis_on(void) {}
void linear_eis_off(void) {}
void linear_lowlight_on(void) {}
void linear_4k_on(void) {}
void stream_server_config_4k_on(void) {}
void stream_server_config_2k_on(void) {}

/* ---- fw_image.h ---- */
int8_t set_image_zoom(uint8_t image_zoom) {
  (void)image_zoom;
  return mock_set_image_zoom_fail ? -1 : 0;
}
int8_t set_image_rotation(uint8_t image_rotation) {
  (void)image_rotation;
  return mock_fail_image_set ? -1 : 0;
}
int8_t set_image_resolution(uint8_t image_resolution) {
  (void)image_resolution;
  return mock_fail_image_set ? -1 : 0;
}
int8_t set_image_mirror(uint8_t mirror) { (void)mirror; return mock_fail_image_set ? -1 : 0; }
int8_t set_image_flip(uint8_t flip) { (void)flip; return mock_fail_image_set ? -1 : 0; }
int8_t set_image_tilt(uint8_t tilt) { (void)tilt; return mock_fail_image_set ? -1 : 0; }
int8_t set_image_wdr(uint8_t wdr) { (void)wdr; return mock_fail_image_set ? -1 : 0; }
int8_t set_image_eis(uint8_t eis) { (void)eis; return mock_fail_image_set ? -1 : 0; }
int8_t set_image_misc(uint8_t misc) {
  (void)misc;
  return mock_set_image_misc_fail ? -1 : 0;
}
int8_t get_image_zoom(uint8_t *zoom) {
  if (mock_fail_image_get) return -1;
  if (zoom)
    *zoom = 1;
  return 0;
}
int8_t get_image_resolution(uint8_t *resolution) {
  if (mock_get_image_resolution_fail || mock_fail_image_get) return -1;
  if (resolution)
    *resolution = 2; /* R1280x720 */
  return 0;
}
int8_t get_wdr(uint8_t *wdr) { if (mock_fail_image_get) return -1; if (wdr) *wdr = 0; return 0; }
int8_t get_eis(uint8_t *eis) { if (mock_fail_image_get) return -1; if (eis) *eis = 0; return 0; }
int8_t get_flip(uint8_t *flip) { if (mock_fail_image_get) return -1; if (flip) *flip = 0; return 0; }
int8_t get_mirror(uint8_t *mirror) { if (mock_fail_image_get) return -1; if (mirror) *mirror = 0; return 0; }
int8_t set_day_mode(enum ON_OFF on_off) { (void)on_off; return mock_fail_image_set ? -1 : 0; }
int8_t get_day_mode(uint8_t *day_mode) {
  if (mock_get_day_mode_fail) return -1;
  if (day_mode)
    *day_mode = mock_day_mode;
  return 0;
}
int8_t get_image_misc(uint8_t *misc) { if (misc) *misc = mock_image_misc; return 0; }
int8_t set_ir_cutfilter(enum ON_OFF on_off) { (void)on_off; return 0; }
int8_t get_ir_cutfilter(enum ON_OFF *on_off) {
  if (mock_get_ir_cutfilter_fail || mock_fail_image_get) return -1;
  if (on_off)
    *on_off = OFF;
  return 0;
}
int8_t set_ir_led_brightness(uint8_t brightness) {
  (void)brightness;
  return mock_fail_image_set ? -1 : 0;
}
int8_t get_ir_led_brightness(uint8_t *brightness) {
  if (mock_fail_image_get) return -1;
  if (brightness)
    *brightness = 0;
  return 0;
}
int8_t set_haptic(enum ON_OFF on_off) { (void)on_off; return 0; }
int8_t set_heater(enum ON_OFF on_off) { (void)on_off; return 0; }
int8_t get_heater(enum ON_OFF *on_off) { if (on_off) *on_off = OFF; return 0; }
int8_t debug_pwm4_set(uint8_t val) { (void)val; return mock_fail_image_set ? -1 : 0; }
int8_t debug_pwm5_set(uint8_t val) { (void)val; return mock_fail_image_set ? -1 : 0; }
int8_t set_ir_temp_state(uint8_t state) { (void)state; return 0; }
int8_t get_ir_temp_state(uint8_t *state) { if (state) *state = 0; return 0; }
int8_t set_isp_temp_state(uint8_t state) { (void)state; return 0; }
int8_t get_isp_temp_state(uint8_t *state) { if (state) *state = 0; return 0; }
int8_t start_webrtc_stream(void) {
  return mock_start_webrtc_stream_fail ? -1 : 0;
}
int8_t stop_webrtc_stream(void) {
  return mock_stop_webrtc_stream_fail ? -1 : 0;
}

/* ---- fw_sensor.h ---- */
int8_t set_gyro_reader(enum ON_OFF on_off) { (void)on_off; return mock_fail_image_set ? -1 : 0; }
int8_t get_gyro_reader(uint8_t *gyro_reader) {
  if (mock_get_gyro_reader_fail || mock_fail_image_get) return -1;
  if (gyro_reader)
    *gyro_reader = 0;
  return 0;
}
int8_t get_motion_data(int16_t *mag_x, int16_t *mag_y, int16_t *mag_z) {
  if (mag_x) *mag_x = 0;
  if (mag_y) *mag_y = 0;
  if (mag_z) *mag_z = 0;
  return 0;
}
int8_t get_sensor_temp(uint8_t *temp) { if (temp) *temp = 0; return 0; }
int8_t get_isp_temp(uint8_t *temp) { if (temp) *temp = 0; return 0; }
int8_t get_ir_temp(uint8_t *temp) { if (temp) *temp = 0; return 0; }
int8_t camera_health_check(uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d,
                            uint8_t *e, uint8_t *f, uint8_t *g, uint8_t *h) {
  if (mock_fail_system_fw) return -1;
  if (a) *a = 0;
  if (b) *b = 0;
  if (c) *c = 0;
  if (d) *d = 0;
  if (e) *e = 0;
  if (f) *f = 0;
  if (g) *g = 0;
  if (h) *h = 0;
  return 0;
}

/* ---- fw_streaming.h ---- */
int get_ini_value(const char *filename, const char *section, const char *key,
                  int default_value) {
  (void)filename;
  (void)section;
  (void)key;
  return default_value;
}
ImageResolution map_resolution(int width, int height) {
  if (width == 640 && height == 360)
    return R640x360;
  if (width == 1280 && height == 720)
    return R1280x720;
  if (width == 1920 && height == 1080)
    return R1920x1080;
  if (width == 3840 && height == 2160)
    return R3840x2160;
  return R1920x1080;
}
Encoder map_encoder(int codec) {
  return (codec == 1) ? H265 : H264;
}
int8_t get_stream_state(void) { return mock_stream_state; }
int8_t get_webrtc_streaming_state(uint8_t *webrtc_state) {
  if (mock_get_webrtc_fail) return -1;
  if (webrtc_state)
    *webrtc_state = 0;
  return 0;
}
int8_t start_stream(void) { return mock_start_stream_fail ? -1 : 0; }
int8_t stop_stream(void) { return mock_stop_stream_fail ? -1 : 0; }
int8_t get_stream_resolution(enum image_resolution *r, uint8_t n) {
  (void)n;
  if (r)
    *r = R1920x1080;
  return 0;
}
int8_t get_stream_fps(uint8_t *fps, uint8_t n) {
  (void)n;
  if (fps)
    *fps = 25;
  return 0;
}
int8_t get_stream_bitrate(uint8_t *bitrate, uint8_t n) {
  (void)n;
  if (bitrate)
    *bitrate = 2;
  return 0;
}
int8_t get_stream_encoder(enum encoder_type *e, uint8_t n) {
  (void)n;
  if (e)
    *e = H264;
  return 0;
}
char *trim(char *str) { return str; }

/* ---- fw_network.h ---- */
int8_t get_wifi_hotspot_ipaddress(char *ip_address) {
  if (ip_address)
    strcpy(ip_address, "192.168.1.1");
  return 0;
}
int8_t get_wifi_client_ipaddress(char *ip_address) {
  if (ip_address)
    ip_address[0] = '\0';
  return 0;
}
int8_t get_eth_ipaddress(char *ip_address) {
  if (mock_fail_network_fw) return -1;
  if (ip_address)
    strcpy(ip_address, "192.168.1.100");
  return 0;
}
int8_t set_ethernet_ip_address(const char *ip, const char *subnet) {
  (void)ip;
  (void)subnet;
  return mock_fail_network_fw ? -1 : 0;
}
int8_t set_ethernet_dhcp_config(void) { return mock_fail_network_fw ? -1 : 0; }
int8_t set_wifi_hotspot_config(const char *ssid, const uint8_t enc,
                               const char *key, const char *ip,
                               const char *subnet) {
  (void)ssid;
  (void)enc;
  (void)key;
  (void)ip;
  (void)subnet;
  return mock_fail_network_fw ? -1 : 0;
}
int8_t get_wifi_hotspot_config(char *ssid, uint8_t *enc, char *key,
                                char *ip, char *subnet) {
  if (mock_fail_network_fw) return -1;
  if (mock_network_config_nonempty) {
    if (ssid) strcpy(ssid, "Test");
    if (enc) *enc = 1;
    if (key) strcpy(key, "12345678");
    if (ip) strcpy(ip, "192.168.1.1");
    if (subnet) strcpy(subnet, "255.255.0.0");
  } else {
    if (ssid) ssid[0] = '\0';
    if (enc) *enc = 0;
    if (key) key[0] = '\0';
    if (ip) ip[0] = '\0';
    if (subnet) subnet[0] = '\0';
  }
  return 0;
}
int8_t set_wifi_client_config(const char *s, const uint8_t e, const char *k,
                              const char *i, const char *n) {
  (void)s;
  (void)e;
  (void)k;
  (void)i;
  (void)n;
  return mock_fail_network_fw ? -1 : 0;
}
int8_t set_wifi_dhcp_client_config(const char *s, const uint8_t e,
                                    const char *k) {
  (void)s;
  (void)e;
  (void)k;
  return mock_fail_network_fw ? -1 : 0;
}
int8_t get_wifi_client_config(char *s, uint8_t *e, char *k, char *i, char *n) {
  if (mock_fail_network_fw) return -1;
  if (mock_network_config_nonempty) {
    if (s) strcpy(s, "Test");
    if (e) *e = 1;
    if (k) strcpy(k, "12345678");
    if (i) strcpy(i, "192.168.1.2");
    if (n) strcpy(n, "255.255.0.0");
  } else {
    if (s) s[0] = '\0';
    if (e) *e = 0;
    if (k) k[0] = '\0';
    if (i) i[0] = '\0';
    if (n) n[0] = '\0';
  }
  return 0;
}
int8_t get_wifi_state(uint8_t *wifi_state) {
  if (mock_fail_network_fw) return -1;
  if (wifi_state)
    *wifi_state = mock_wifi_state_value;
  return 0;
}
int8_t set_onvif_interface(const uint8_t interface) {
  (void)interface;
  return mock_fail_network_fw ? -1 : 0;
}
int8_t get_onvif_interface_state(uint8_t *interface) {
  if (mock_fail_network_fw) return -1;
  if (interface)
    *interface = 0;
  return 0;
}
int8_t set_wifi_country_code(const char *code) {
  (void)code;
  return mock_fail_network_fw ? -1 : 0;
}
int8_t get_wifi_country_code(char *code) {
  if (mock_fail_network_fw) return -1;
  if (code) {
    if (mock_wifi_country_code_mode == 1) {
      strcpy(code, "U");
    } else if (mock_wifi_country_code_mode == 2) {
      code[0] = '\0';
    } else {
      strcpy(code, "US");
    }
  }
  return 0;
}

/* ---- fw_system.h ---- */
int8_t set_camera_name(const char *camera_name) { (void)camera_name; return mock_fail_system_fw ? -1 : 0; }
int8_t set_login(const char *pin, const char *dob) {
  (void)pin;
  (void)dob;
  if (mock_set_login_ret != 0) return mock_set_login_ret;
  return mock_fail_system_fw ? -1 : 0;
}
int8_t get_camera_name(char *name) {
  if (mock_fail_system_fw) return -1;
  if (name)
    strcpy(name, "TestCam");
  return 0;
}
int8_t get_firmware_version(char *ver) {
  if (mock_fail_system_fw) return -1;
  if (ver)
    strcpy(ver, "1.0.0");
  return 0;
}
int8_t get_mac_address(char *mac) {
  if (mock_fail_system_fw) return -1;
  if (mac)
    strcpy(mac, "00:00:00:00:00:00");
  return 0;
}
int8_t get_ota_update_status(char *s) { if (mock_fail_system_fw) return -1; if (s) strcpy(s, "OK"); return 0; }
int8_t get_factory_reset_status(char *s) { if (s) s[0] = '\0'; return 0; }
int8_t get_login_pin(char *pin) {
  if (mock_fail_system_fw) return -1;
  if (pin) {
    if (mock_login_pin_mode == 1) strcpy(pin, "1234");
    else pin[0] = '\0';
  }
  return 0;
}
int8_t shutdown_device(void) { return mock_fail_system_fw ? -1 : 0; }
int8_t ota_update(void) { return mock_fail_system_fw ? -1 : 0; }
int8_t provisioning_mode(const char *mac, const char *serial,
                         const char *mfg) {
  (void)mac;
  (void)serial;
  (void)mfg;
  return mock_fail_system_fw ? -1 : 0;
}
int8_t remove_ota_files(void) { return 0; }
int8_t set_factory_reset(const char *dob) { (void)dob; if (mock_set_factory_reset_ret != 0) return mock_set_factory_reset_ret; return mock_fail_system_fw ? -1 : 0; }
int8_t set_config_reset(const char *dob) { (void)dob; if (mock_set_config_reset_ret != 0) return mock_set_config_reset_ret; return mock_fail_system_fw ? -1 : 0; }
int8_t set_user_dob(const char *dob) { (void)dob; return mock_fail_system_fw ? -1 : 0; }
int8_t get_user_dob(char *dob) { if (mock_fail_system_fw) return -1; if (dob) strcpy(dob, "01-01-2024"); return 0; }
int8_t validate_user_dob(const char *dob) { (void)dob; return 0; }
int8_t set_system_time(const char *epoch) { (void)epoch; return mock_fail_system_fw ? -1 : 0; }

/* ---- fw_audio.h ---- */
int8_t set_mic(enum ON_OFF on_off) { (void)on_off; return 0; }
int8_t get_mic(const enum ON_OFF *on_off) {
  (void)on_off;
  return 0;
}

/* set_indication_led may be referenced from fw_image */
int8_t set_indication_led(enum ON_OFF on_off) { (void)on_off; return 0; }
