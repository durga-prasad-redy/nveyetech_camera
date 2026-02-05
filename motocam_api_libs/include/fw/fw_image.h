#ifndef FW_IMAGE_H
#define FW_IMAGE_H

#include <stdint.h>
#include "fw.h"

#define DAY_EIS_OFF_WDR_OFF 1
#define DAY_EIS_ON_WDR_OFF 2
#define DAY_EIS_OFF_WDR_ON 3
#define DAY_EIS_ON_WDR_ON 4 // using for 4k resolution
#define LOWLIGHT_EIS_OFF_WDR_OFF 5
#define LOWLIGHT_EIS_ON_WDR_OFF 6
#define LOWLIGHT_EIS_OFF_WDR_ON 7
#define LOWLIGHT_EIS_ON_WDR_ON 8
#define NIGHT_EIS_OFF_WDR_OFF 9
#define NIGHT_EIS_ON_WDR_OFF 10
#define NIGHT_EIS_OFF_WDR_ON 11
#define NIGHT_EIS_ON_WDR_ON 12 // using for 4k resolution

#define WEBRTC_ENABLED "webrtc_enabled"

#define GET_WEBRTC_ENABLED "cat " M5S_CONFIG_DIR "/webrtc_enabled"

int8_t set_image_zoom(uint8_t image_zoom);
int8_t set_image_rotation(uint8_t image_rotation);
int8_t set_image_resolution(uint8_t image_resolution);
int8_t set_image_mirror(uint8_t mirror);
int8_t set_image_flip(uint8_t flip);
int8_t set_image_tilt(uint8_t tilt);
int8_t set_image_wdr(uint8_t wdr);
int8_t set_image_eis(uint8_t eis);
int8_t set_image_misc(uint8_t misc);
int8_t get_image_zoom(uint8_t *zoom);
int8_t get_image_resolution(uint8_t *resolution);
int8_t get_wdr(uint8_t *wdr);
int8_t get_eis(uint8_t *eis);
int8_t get_flip(uint8_t *flip);
int8_t get_mirror(uint8_t *mirror);

int8_t set_day_mode(enum ON_OFF on_off);
int8_t get_day_mode(uint8_t *day_mode);
int8_t get_image_misc(uint8_t *misc);
/*set ir cut filter on(0) or off(1) returns 0-success -1-fail*/
int8_t set_ir_cutfilter(enum ON_OFF on_off);
/*get ir cut filter on(0) or off(1) status returns 0-success -1-fail*/
int8_t get_ir_cutfilter(enum ON_OFF *on_off);
/*set ir led brightness returns 0-success -1-fail*/
int8_t set_ir_led_brightness(uint8_t brightness);
int8_t get_ir_led_brightness(uint8_t *brightness);
/*set haptic on(0) or off(1) returns 0-success -1-fail*/
int8_t set_haptic(enum ON_OFF on_off);
/*set heater on(0) or off(1) returns 0-success -1-fail*/
int8_t set_heater(enum ON_OFF on_off);
/*get heater on(0) or off(1) status returns 0-success -1-fail*/
int8_t get_heater(enum ON_OFF *on_off);
int8_t set_haptic_motor(int duty_cycle, int duration);
int8_t debug_pwm4_set(uint8_t val);
int8_t debug_pwm5_set(uint8_t val);
int8_t set_ir_temp_state(uint8_t state);
int8_t get_ir_temp_state(uint8_t *state);
int8_t set_isp_temp_state(uint8_t state);
int8_t get_isp_temp_state(uint8_t *state);



int8_t start_webrtc_stream();
int8_t stop_webrtc_stream();



int8_t start_webrtc_stream();
int8_t stop_webrtc_stream();


#endif // FW_IMAGE_H
