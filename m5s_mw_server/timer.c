#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include "fw.h"
#include "fw/fw_image.h"
#include "fw/fw_sensor.h"
#include "gpio.h"
#include "log.h"
#include "timer.h"

#define TEMP_TIMER_IN_SEC    10 // timer calling interval

#define IR_HIGH_TEMP_THRESHOLD 90  // When temp > this, set IR to 15
#define IR_LOW_TEMP_THRESHOLD 70 // When temp < this, restore original IR

#define SENSOR_HIGH_TEMP_THRESHOLD 70  // When temp > this, set IR to Off
#define SENSOR_LOW_TEMP_THRESHOLD 60 // When temp < this, restore original IR

#define ISP_HIGH_TEMP_THRESHOLD 80
#define ISP_LOW_TEMP_THRESHOLD 70

#define GAIN_FILE "/mnt/flash/vienna/tmp/asc_ae_log.txt"

#define DAY_MODE_MISC 2
#define LL_MODE_MISC 8
#define NIGHT_MODE_MISC 11

#define DAY_LL 8
#define LOW_LIGHT_NIGHT 12
#define LOW_LIGHT_D 3
#define NIGHT_DAY 2
#define MAX_COUNT 8
#define MAX_COUNT_TEST 5

#define DAY 0
#define LOW_LIGHT 1
#define NIGHT 2
#define LOW_LIGHT_DAY 3

float mode[4] = {DAY_LL, LOW_LIGHT_NIGHT, NIGHT_DAY, LOW_LIGHT_D};

int8_t is_ir_temp_state, is_sensor_temp_state;

static int minute_counter_ir = 0;

static volatile sig_atomic_t timer_tick = 0;
static pthread_t tid;

static int read_float_from_file(const char *path, float *val)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;

    float tmp;
    if (fscanf(fp, "%f", &tmp) != 1) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    *val = tmp;
    return 0;
}

static void load_mode_thresholds_from_config(void)
{
    struct {
        const char *file;
        int row;
    } cfg[] = {
        { "day_ll",     DAY},
        { "ll_night",   LOW_LIGHT},
        { "night_day",  NIGHT},
        { "ll_day",   LOW_LIGHT_DAY},
    };

    char path[128];
    float val;

    for (size_t i = 0; i < sizeof(cfg)/sizeof(cfg[0]); i++) {
        snprintf(path, sizeof(path),
                 "/mnt/flash/vienna/m5s_config/%s", cfg[i].file);

        if (read_float_from_file(path, &val) == 0) {
            LOG_INFO("AutoDN cfg: %s=%.2f (override)\n",
                     cfg[i].file, val);
            mode[cfg[i].row] = val;
        } else {
            LOG_DEBUG("AutoDN cfg: %s not found, using default %.2f\n",
                      cfg[i].file,
                      mode[cfg[i].row]);
        }
    }
}

void control_ir(void)
{
    static uint8_t isp_state = 0;
    static uint8_t ir_val = 0;
    int8_t ir_tmp_ctl;

    uint8_t isp_temp, ir_temp, sensor_temp;

    if (is_sensor_temp_state==1)
        get_sensor_temp(&sensor_temp);
    else
        sensor_temp = 0;

    LOG_DEBUG("Sensor temp value  is %d\n", sensor_temp);

    /* Get ISP temp details */
    get_isp_temp(&isp_temp);
    
    LOG_DEBUG("ISP Temp Value  is %d\n", isp_temp);

    if (is_ir_temp_state==1)
        get_ir_temp(&ir_temp);
    else
        ir_temp = 0;

    ir_tmp_ctl = get_ir_tmp_ctl();
    if (ir_tmp_ctl)
    {
        LOG_DEBUG("IR temp is %d\n",ir_temp);
        if (isp_temp > ISP_HIGH_TEMP_THRESHOLD || ir_temp > IR_HIGH_TEMP_THRESHOLD || sensor_temp > SENSOR_HIGH_TEMP_THRESHOLD) {
            get_ir_led_brightness(&ir_val);
            if (ir_val > HIGH) {
                set_ir_led_brightness(HIGH);
                LOG_INFO("ISP: Reduce the power of IR\n");
            }
            set_isp_temp_state(1);
            isp_state = 1;
        } else if ((isp_temp < ISP_LOW_TEMP_THRESHOLD && ir_temp < IR_LOW_TEMP_THRESHOLD && sensor_temp < SENSOR_LOW_TEMP_THRESHOLD) && (isp_state == 1)) {
            set_ir_led_brightness(ir_val);
            set_isp_temp_state(0);
            isp_state = 0;
            LOG_DEBUG("ISP: Restore threshold\n");
        }
    }
}

void get_gain(float *gain_value)
{
    float gain = 0;
    
    int status;
    status = system("asc_msg_sender -e 12 -g");
    if (status != 0)
        LOG_ERROR("Command execution failed with status %d\n", status);

    FILE *fp = fopen(GAIN_FILE, "r");
    if (!fp)
        return;

    if (fscanf(fp, "%f", &gain) != 1) {
        fclose(fp);
        return;
    }
    *gain_value = gain;
    fclose(fp);
}

void process_auto_day_night(void)
{
    static int sample_count = 0;
    static int day_count = 0;
    static int low_light_count = 0;
    static int night_count = 0;
    uint8_t misc_val = 0;
    float gain_value = 0;

    int8_t day_mode;
    float gain_thr, gain_thr_1;

    day_mode = get_mode();
    if (!day_mode)
        return;

    get_gain(&gain_value);
    LOG_INFO("Gain value: %f\n", gain_value);

    get_image_misc(&misc_val);
    if (misc_val > 0 && misc_val < 5) {
        gain_thr = mode[DAY]; 
        if (gain_value > gain_thr)
            low_light_count++;
    } else if (misc_val > 4 && misc_val < 9) {
        gain_thr = mode[LOW_LIGHT];
        gain_thr_1 = mode[LOW_LIGHT_DAY];
        if (gain_value > gain_thr)
            night_count++;
        else if (gain_value < gain_thr_1)
            day_count++;
    } else { 
        gain_thr = mode[NIGHT]; 
        if (gain_value < gain_thr)
            day_count++;
    }

    sample_count++;
    if (sample_count < MAX_COUNT){
        return;
    }    

    if (day_count >= MAX_COUNT_TEST) {
            LOG_INFO("Auto DN: Day detected, IR OFF\n");
            set_image_misc(DAY_MODE_MISC);
    } else if (low_light_count >= MAX_COUNT_TEST) {
            LOG_INFO("Auto DN: Switch to LL MONO\n");
            set_image_misc(LL_MODE_MISC);
    } else if (night_count >= MAX_COUNT_TEST) {
            LOG_INFO("Auto DN: Switch to IR ON\n");
            set_image_misc(NIGHT_MODE_MISC);
    }

    day_count = 0;
    low_light_count = 0;
    night_count = 0;
    sample_count = 0;

    return;
}

static void *timer_worker(void *arg)
{
    (void)arg;

    while (1) {
        if (timer_tick) {
            timer_tick = 0;

            if (++minute_counter_ir >= 6) {
                minute_counter_ir = 0;
                control_ir();
            }
            process_auto_day_night();
        }
        usleep(10000); // 10 ms
    }
    return NULL;
}

void timer_handler(int sig)
{
    (void)sig;
    timer_tick = 1;
}

void timer_init()
{
    struct sigaction sa;
    struct itimerval timer;

    load_mode_thresholds_from_config();
    // checking for ir and sensor support is there or not
    is_ir_temp_state = is_ir_temp();
    is_sensor_temp_state = is_sensor_temp();

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    // Configure timer to go off every 3 seconds
    timer.it_value.tv_sec = TEMP_TIMER_IN_SEC;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = TEMP_TIMER_IN_SEC;
    timer.it_interval.tv_usec = 0;
    
    setitimer(ITIMER_REAL, &timer, NULL);
    pthread_create(&tid, NULL, timer_worker, NULL);
}
