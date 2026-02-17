#ifndef FW_H
#define FW_H

#include "gpio.h"
#include "log.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stddef.h>
#include <ctype.h>
#include <pthread.h>

#define CONFIG_PATH "/mnt/flash/vienna/config"
#define RES_PATH "/mnt/flash/vienna"
#define BIN_RES_PATH "/mnt/flash/vienna/bin"
#define PWM5_FILE "/dev/pwmdev-5"
#define PWM4_FILE "/dev/pwmdev-4"
#define PWM7_FILE "/dev/pwmdev-7"
#define LOCK_FILE "/tmp/pwmdev.lock"
#define M5S_CONFIG_DIR "/mnt/flash/vienna/m5s_config"

#define DEVICE_SETUP_FILE "/mnt/flash/vienna/firmware/board_setup/device_setup"

#define GET_MISC "cat " M5S_CONFIG_DIR "/misc"


#define GET_DAY_MODE "cat " M5S_CONFIG_DIR "/day_mode"
#define GET_IR_TEMP_CTL "cat " M5S_CONFIG_DIR "/ir_tmp_ctl"
#define GET_WDR      "cat " M5S_CONFIG_DIR "/wdr"
#define GET_EIS      "cat " M5S_CONFIG_DIR "/eis"
#define IR           "ir_led_brightness"

#define GET_IS_IR_TEMP      "cat " M5S_CONFIG_DIR "/is_ir_temp"
#define GET_IS_SENSOR_TEMP  "cat " M5S_CONFIG_DIR "/is_sensor_temp"

#define GET_STREAMING_STATE \
  "cat " M5S_CONFIG_DIR "/stream_state" // 1:Started, 0:Stopped

#define NIGHT_M 2
#define DAY_M 1
#define LOW_LIGHT_M 0

#define ULTRA 10
#define MAX 8
#define HIGH 6
#define MEDIUM 4
#define LOW 2
#define IR_OFF 0

#define IR_0 "1000000,1000000"
#define IR_10 "1000000,900000"
#define IR_30 "1000000,700000"
#define IR_40 "1000000,600000"
#define IR_60 "1000000,400000"
#define IR_80 "1000000,200000"
#define IR_90 "1000000,100000"

#define STREAMER_PROCESS_NAME "streamer"
#define SIGNALING_SERVER_PROCESS_NAME "signalserver"
#define PORTABLE_RTC_PROCESS_NAME "portablertc"
#define ONVIF_SERVER_PROCESS_NAME "multionvifserver"
#define CAMERA_MONITOR_PROCESS_NAME "camera_monitor"
#define RTSP_SERVER_PROCESS_NAME "rtsps"
#define STREAM_RESTART_SERVICE_PROCESS_NAME "start.sh"

#define EXEC_GET_UINT8(cmd, out_ptr)        \
do {                                       \
    char _buf[64];                         \
    if (exec_cmd(cmd, _buf, sizeof(_buf)) == 0) \
        *(out_ptr) = (uint8_t)atoi(_buf);  \
} while (0)

#define WIFI_RUNTIME_RESULT "/mnt/flash/vienna/m5s_config/wifi_runtime_result" // Replace with actual file path

typedef enum encoder_type
{
    H264 = 0,
    H265
} Encoder;

typedef enum image_resolution
{
    R640x360 = 1,
    R1280x720,
    R1920x1080,
    R3840x2160

} ImageResolution;

typedef enum ON_OFF
{
    OFF = 0,
    ON = 1
} OnOff;

typedef enum onvif_interface_state
{

    ETHER=0,
    WIFI=1
}OnvifInterfaceState;


typedef enum encryption_type
{
    WEP = 1,
    WPA,
    WPA2
} EncryptionType;
typedef enum image_zoom
{
    X1 = 1,
    X2,
    X3,
    X4
} ImageZoom;
typedef enum image_rotation
{
    R110 = 1,
    R90,
    R180,
    R270
} ImageRotation;

extern pthread_mutex_t lock;

int update_calibration_value();

/*set indication led on(0) or off(1) status returns 0-success -1-fail*/
int8_t set_indication_led(enum ON_OFF on_off);
/* Only get operation to decide system state */
int8_t get_temp(int8_t *temp);
int8_t get_mode();
int8_t get_ir_tmp_ctl();

int8_t is_ir_temp();
int8_t is_sensor_temp();
/* API's related to wifi configuration. returns 0-success -1-fail */


/* Image related API's. Should be called only when streaming is off*/
/*set image zoom returns 0-success -1-fail*/





/*get image zoom returns 0-success -1-fail*/
/*set image rotation returns 0-success -1-fail*/

// system commands


int exec_cmd(const char *cmd, char *out, size_t out_size);
int8_t outdu_update_brightness(uint8_t val);
void safe_remove(const char *path);
void safe_symlink(const char *target, const char *linkpath);

void fusion_eis_off();
void linear_eis_on();
void linear_eis_off();
void linear_lowlight_on();
void linear_4k_on();
void stream_server_config_4k_on();
void stream_server_config_2k_on();
uint8_t access_file(const char *path);
int exec_return(const char *cmd);

int8_t set_uboot_env(const char *key, uint8_t value);

int is_running(const char *process_name);
void stop_process(const char *process_name);
int8_t set_haptic_motor(int duty_cycle, int duration);

int8_t set_uboot_env_chars(const char *key, const char *value);
void kill_all_processes();

#endif // FW_H
