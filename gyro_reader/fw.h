#ifndef FW_H

#include <stdint.h>
#define FW_H

typedef enum ON_OFF{
    OFF=0,
    ON=1
}OnOff;
typedef enum encryption_type{
    WEP=1,
    WPA,
    WPA2
}EncryptionType;
typedef enum image_zoom{
    X1=1,
    X2,
    X3,
    X4
}ImageZoom;
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
/*set indication led on(0) or off(1) status returns 0-success -1-fail*/
int8_t set_indication_led(enum ON_OFF on_off);
/* Only get operation to decide system state */
int8_t get_temp(int8_t *temp);
int8_t set_mic(enum ON_OFF on_off);
int8_t get_mic(enum ON_OFF *on_off);
int8_t set_day_mode(enum ON_OFF on_off);
int8_t get_day_mode(uint8_t *day_mode);
int8_t get_motion_data(int16_t *mag_x, int16_t *mag_y, int16_t *mag_z);
/* API's related to wifi configuration. returns 0-success -1-fail */
int8_t set_wifi_hotspot_config(const char *ssid, const uint8_t encryption_type, const char *encryption_key, const char *ip_address, const char *subnetmask);
int8_t get_wifi_hotspot_config(char *ssid, uint8_t *encryption_type, char *encryption_key, char *ip_address, char *subnetmask);
int8_t set_wifi_client_config(const char *ssid, const uint8_t encryption_type, const char *encryption_key, const char *ip_address, const char *subnetmask);
int8_t get_wifi_client_config(char *ssid, uint8_t *encryption_type, char *encryption_key, char *ip_address, char *subnetmask);
/* Image related API's. Should be called only when streaming is off*/
/*set image zoom returns 0-success -1-fail*/

int8_t set_image_zoom(uint8_t image_zoom);
int8_t set_image_rotation(uint8_t image_rotation);
int8_t set_image_resolution(uint8_t image_resolution);
int8_t set_image_mirror(uint8_t mirror);
int8_t set_image_flip(uint8_t flip);
int8_t set_image_tilt(uint8_t tilt);
int8_t set_image_wdr(uint8_t wdr);
int8_t set_image_eis(uint8_t eis);

int8_t get_image_resolution(uint8_t *resolution);
int8_t get_wdr(uint8_t *wdr);
int8_t get_eis(uint8_t *eis);

int8_t start_stream();
int8_t stop_stream();
/*get image zoom returns 0-success -1-fail*/
/*set image rotation returns 0-success -1-fail*/

int8_t shutdown_device();
#endif
