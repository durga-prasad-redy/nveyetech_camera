#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <errno.h>
#include "gpio.h"
#include "log.h"

#define GPIO_EXPORT_PATH "/sys/class/gpio/export"
#define GPIO_DIRECTION_PATH "/sys/class/gpio/gpio%d/direction"
#define GPIO_VALUE_PATH "/sys/class/gpio/gpio%d/value"

#define OTA_WATCH_DIR   "/mnt/flash/vienna/firmware/ota/"
#define OTA_STATUS_FILE "/mnt/flash/vienna/m5s_config/ota_status"
#define WIFI_STATE_PATH "/sys/class/net/wlan0/operstate"
#define IRCUT_FILTER "ircut_filter"
#define IRCUT_STATE_FILE "/mnt/flash/vienna/m5s_config/ircut_filter"
#define WIFI_STATE_FILE "/mnt/flash/vienna/m5s_config/wifi_state"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef enum {
    SUBSYS_OK = 0,
    SUBSYS_FAIL,
    SUBSYS_IN_PROGRESS
} subsys_state_t;

typedef struct {
    subsys_state_t wifi;
    subsys_state_t ota;
    subsys_state_t onvif;
    int ota_passed_not_rebooted;
    int wifi_ap_no_client;
} system_state_t;

typedef struct {
    int red;
    int green;
    int blue;
    int duration_ms;
} led_step_t;


// define patterns for RGB LED
static const led_step_t PATTERN_ALL_OK[] = {
    {0, 1, 0, 1000},
};

static const led_step_t PATTERN_OTA_PROGRESS[] = {
    {0, 0, 1, 1000},
    {0, 0, 0, 1000},
};

static const led_step_t PATTERN_OTA_PASS_NO_REBOOT[] = {
    {0, 1, 0, 800},
    {0, 0, 1, 800},
};

static const led_step_t PATTERN_OTA_FAIL[] = {
    {0, 0, 1, 300},
    {0, 0, 0, 300},
};

static const led_step_t PATTERN_WIFI_FAIL[] = {
    {1, 0, 0, 800},
    {0, 0, 0, 800},
};

static const led_step_t PATTERN_WIFI_AP_NO_CLIENT[] = {
    {1, 0, 0, 1000},
};

static const led_step_t PATTERN_ONVIF_FAIL[] = {
    {1, 1, 0, 800},
    {0, 0, 0, 800},
};

static const led_step_t PATTERN_WIFI_OTA_FAIL[] = {
    {1, 0, 0, 700},
    {0, 0, 1, 700},
};

static const led_step_t PATTERN_WIFI_ONVIF_FAIL[] = {
    {1, 0, 0, 700},
    {1, 1, 0, 800},
};

static const led_step_t PATTERN_OTA_ONVIF_FAIL[] = {
    {1, 1, 0, 800},
    {0, 0, 1, 700},
};

static const led_step_t PATTERN_ALL_FAIL[] = {
    {1, 0, 0, 600},
    {1, 1, 0, 600},
    {0, 0, 1, 600},
};

static const led_step_t PATTERN_WIFI_FAIL_OTA_PASSED[] = {
    {1, 0, 0, 500},
    {0, 0, 1, 500},
    {0, 0, 1, 500},
};


static int is_ota_failed_state(const char *status)
{
    /* Explicit known failures */
    if (!strcmp(status, "compatible-mismatch"))
        return 1;

    /* Generic failure detection */
    if (strstr(status, "fail") || strstr(status, "err"))
        return 1;

    return 0;
}

static void export_gpio(int gpio_pin)
{
    FILE *export_file = fopen(GPIO_EXPORT_PATH, "w");
    
    if (export_file == NULL) {
        perror("Failed to export GPIO pin");
        exit(EXIT_FAILURE);
    }
    fprintf(export_file, "%d", gpio_pin);
    fclose(export_file);
}

static void set_gpio_direction(int gpio_pin, const char *direction)
{
    char direction_path[64];
    snprintf(direction_path, sizeof(direction_path), GPIO_DIRECTION_PATH, gpio_pin);
    FILE *direction_file = fopen(direction_path, "w");

    if (direction_file == NULL) {
        perror("Failed to set GPIO direction");
        exit(EXIT_FAILURE);
    }
    fprintf(direction_file, "%s", direction);
    fclose(direction_file);
}

static void set_gpio_value(int gpio_pin, int value)
{
    char value_path[64];
    snprintf(value_path, sizeof(value_path), GPIO_VALUE_PATH, gpio_pin);
    FILE *value_file = fopen(value_path, "w");
    
    if (value_file == NULL) {
        perror("Failed to set GPIO value");
        exit(EXIT_FAILURE);
    }
    fprintf(value_file, "%d", value);
    fclose(value_file);
}

static uint8_t get_gpio_value(int gpio_pin)
{
    char value_path[64];
    snprintf(value_path, sizeof(value_path), GPIO_VALUE_PATH, gpio_pin);

    FILE *fp = fopen(value_path, "r");
    if (!fp) {
        perror("Failed to open GPIO value file");
        return 2;
    }

    char value[4];
    if (fgets(value, sizeof(value), fp) == NULL) {
        perror("Failed to read GPIO value");
        fclose(fp);
        return 2;
    }
    fclose(fp);

    // Remove newline if present
    value[strcspn(value, "\n")] = '\0';

    if (strcmp(value, "1") == 0) {
        return 1;
    } else {
        return 0;
    }
}

static void apply_rgb(int r, int g, int b)
{
    set_gpio_value(GPIO_PIN_R_LED, r ? SET_GPIO : CLEAR_GPIO);
    set_gpio_value(GPIO_PIN_G_LED, g ? SET_GPIO : CLEAR_GPIO);
    set_gpio_value(GPIO_PIN_B_LED, b ? SET_GPIO : CLEAR_GPIO);
}

void update_ir_cut_filter_on()
{

    set_gpio_value(GPIO_PIN_IR_CUT_59, SET_GPIO);
    set_gpio_value(GPIO_PIN_IR_CUT_60, CLEAR_GPIO);
    sleep(2);
    set_gpio_value(GPIO_PIN_IR_CUT_59, CLEAR_GPIO);
}

void update_ir_cut_filter_off()
{

    set_gpio_value(GPIO_PIN_IR_CUT_59, SET_GPIO);
    set_gpio_value(GPIO_PIN_IR_CUT_60, SET_GPIO);
    sleep(2);
    set_gpio_value(GPIO_PIN_IR_CUT_59, CLEAR_GPIO);
}

uint8_t get_ir_cut_filter()
{
    return get_gpio_value(GPIO_PIN_IR_CUT_60);
}

static int read_ota_status(char *buf, size_t len)
{
    FILE *fp = fopen(OTA_STATUS_FILE, "r");
    if (!fp)
        return -1;

    if (!fgets(buf, len, fp)) {
        fclose(fp);
        return -1;
    }

    buf[strcspn(buf, "\n")] = '\0';
    fclose(fp);
    return 0;
}

static void run_led_pattern(const led_step_t *pattern, int steps)
{
    for (int i = 0; i < steps; i++) {

        apply_rgb(
            pattern[i].red,
            pattern[i].green,
            pattern[i].blue
        );

        usleep(pattern[i].duration_ms * 1000);
    }
}

static int read_ircut_state(void)
{
    FILE *fp;
    int state = 0;

    fp = fopen(IRCUT_STATE_FILE, "r");
    if (!fp)
        return 0;   // default to GREEN on error

    fscanf(fp, "%d", &state);
    fclose(fp);

    return state;
}

static void decide_and_run_led(system_state_t *st)
{
    if (st->ota == SUBSYS_IN_PROGRESS) {
        run_led_pattern(PATTERN_OTA_PROGRESS,
                        ARRAY_SIZE(PATTERN_OTA_PROGRESS));
        return;
    }

    if (st->ota == SUBSYS_FAIL &&
        st->wifi == SUBSYS_FAIL &&
        st->onvif == SUBSYS_FAIL) {
        run_led_pattern(PATTERN_ALL_FAIL,
                        ARRAY_SIZE(PATTERN_ALL_FAIL));
        return;
    }

    if (st->wifi == SUBSYS_FAIL && st->ota == SUBSYS_FAIL) {
        run_led_pattern(PATTERN_WIFI_OTA_FAIL,
                        ARRAY_SIZE(PATTERN_WIFI_OTA_FAIL));
        return;
    }

    if (st->wifi == SUBSYS_FAIL && st->ota_passed_not_rebooted) {
        run_led_pattern(PATTERN_WIFI_FAIL_OTA_PASSED,
                        ARRAY_SIZE(PATTERN_WIFI_FAIL_OTA_PASSED));
        return;
    }

    if (st->ota_passed_not_rebooted &&
            st->wifi == SUBSYS_OK &&
            st->onvif == SUBSYS_OK) {
        run_led_pattern(PATTERN_OTA_PASS_NO_REBOOT,
                ARRAY_SIZE(PATTERN_OTA_PASS_NO_REBOOT));
        return;
    }

    if (st->ota == SUBSYS_FAIL &&
            st->onvif == SUBSYS_FAIL) {
        run_led_pattern(PATTERN_OTA_ONVIF_FAIL,
                        ARRAY_SIZE(PATTERN_OTA_ONVIF_FAIL));
        return;
    }

    if (st->wifi == SUBSYS_FAIL &&
            st->onvif == SUBSYS_FAIL) {
        run_led_pattern(PATTERN_WIFI_ONVIF_FAIL,
                        ARRAY_SIZE(PATTERN_WIFI_ONVIF_FAIL));
        return;
    }

    if (st->ota == SUBSYS_FAIL) {
        run_led_pattern(PATTERN_OTA_FAIL,
                        ARRAY_SIZE(PATTERN_OTA_FAIL));
        return;
    }

    if (st->onvif == SUBSYS_FAIL) {
        run_led_pattern(PATTERN_ONVIF_FAIL,
                        ARRAY_SIZE(PATTERN_ONVIF_FAIL));
        return;
    }

    if (st->wifi_ap_no_client) {
        run_led_pattern(PATTERN_WIFI_AP_NO_CLIENT,
                ARRAY_SIZE(PATTERN_WIFI_AP_NO_CLIENT));
        return;
    }

    if (st->wifi == SUBSYS_FAIL) {
        run_led_pattern(PATTERN_WIFI_FAIL,
                        ARRAY_SIZE(PATTERN_WIFI_FAIL));
        return;
    }

    int ircut = read_ircut_state();
    if (ircut == 1) {
        /*IR ON : Magenta */
        apply_rgb(1, 0, 1);
    } else {
        /* Default: all good: green */
        apply_rgb(0, 1, 0);
    }

    sleep(1);
}

static int read_wifi_state(void)
{
    FILE *fp;
    int state = 0;

    fp = fopen(WIFI_STATE_FILE, "r");
    if (!fp)
        return 0;   // default: STA mode / disabled

    fscanf(fp, "%d", &state);
    fclose(fp);

    return state;
}

static int ap_has_connected_sta(void)
{
    FILE *fp;
    char buf[256];

    fp = popen("hostapd_cli -i wlan0 all_sta 2>/dev/null", "r");
    if (!fp)
        return 0;

    while (fgets(buf, sizeof(buf), fp)) {
        /* MAC address line starts with hex and ':' */
        if (strchr(buf, ':')) {
            pclose(fp);
            return 1;   // at least one client connected
        }
    }

    pclose(fp);
    return 0;   // no client
}

static int wifi_has_ip(void)
{
    FILE *fp;
    char buf[256];

    fp = popen("ifconfig wlan0", "r");
    if (!fp)
        return 0;

    while (fgets(buf, sizeof(buf), fp)) {
        if (strstr(buf, "inet addr:") || strstr(buf, "inet ")) {
            pclose(fp);
            return 1;
        }
    }

    pclose(fp);
    return 0;
}

static subsys_state_t check_wifi(void)
{
    FILE *fp;
    char buf[16];

    fp = fopen(WIFI_STATE_PATH, "r");
    if (!fp)
        return SUBSYS_FAIL;

    fgets(buf, sizeof(buf), fp);
    fclose(fp);

    if (strncmp(buf, "up", 2) != 0)
        return SUBSYS_FAIL;

    /* IP check */
    if (!wifi_has_ip())
        return SUBSYS_FAIL;

    return SUBSYS_OK;
}

static subsys_state_t check_onvif(void)
{
    if (system("pidof multionvifserver > /dev/null") == 0)
        return SUBSYS_OK;

    return SUBSYS_FAIL;
}

static subsys_state_t check_ota(char *status_buf, size_t len)
{
    if (read_ota_status(status_buf, len) < 0)
        return SUBSYS_OK;   // treat as idle

    if (strstr(status_buf, "in-progress"))
        return SUBSYS_IN_PROGRESS;

    if (!strcmp(status_buf, "ota-successful"))
        return SUBSYS_OK;

    if (is_ota_failed_state(status_buf))
        return SUBSYS_FAIL;

    return SUBSYS_OK;
}

static void *system_led_watcher(void *arg)
{
    system_state_t st;
    char ota_status[64];

    while (1) {
        memset(&st, 0, sizeof(st));

        st.wifi  = check_wifi();
        st.onvif = check_onvif();
        st.ota   = check_ota(ota_status, sizeof(ota_status));

        if (st.wifi == SUBSYS_OK && read_wifi_state() == 1 && !ap_has_connected_sta()) {
           st.wifi_ap_no_client = 1;
        }
        if (!strcmp(ota_status, "ota-successful"))
            st.ota_passed_not_rebooted = 1;

        decide_and_run_led(&st);
    }
}

int start_led_watcher()
{
    pthread_t tid;

    if (pthread_create(&tid, NULL, system_led_watcher, NULL) != 0) {
        perror("pthread_create");
        return -1;
    }

    pthread_detach(tid);
    return 0;
}

int gpio_init()
{

    LOG_DEBUG("gpio_init Called");

    // Initialize IR CUT FILTER pins
    export_gpio(GPIO_PIN_IR_CUT_59);
    set_gpio_direction(GPIO_PIN_IR_CUT_59, GPIO_DIRECTION_OUT);
    set_gpio_value(GPIO_PIN_IR_CUT_59, SET_GPIO);

    export_gpio(GPIO_PIN_IR_CUT_60);
    set_gpio_direction(GPIO_PIN_IR_CUT_60, GPIO_DIRECTION_OUT);
    set_gpio_value(GPIO_PIN_IR_CUT_60, SET_GPIO);

    set_gpio_value(GPIO_PIN_IR_CUT_59, CLEAR_GPIO);

    // Initialize B LED
    export_gpio(GPIO_PIN_B_LED);
    set_gpio_direction(GPIO_PIN_B_LED, GPIO_DIRECTION_OUT);
    set_gpio_value(GPIO_PIN_B_LED, CLEAR_GPIO);

    // Initialize G LED
    export_gpio(GPIO_PIN_G_LED);
    set_gpio_direction(GPIO_PIN_G_LED, GPIO_DIRECTION_OUT);
    set_gpio_value(GPIO_PIN_G_LED, SET_GPIO);

    // Initialize R LED
    export_gpio(GPIO_PIN_R_LED);
    set_gpio_direction(GPIO_PIN_R_LED, GPIO_DIRECTION_OUT);
    set_gpio_value(GPIO_PIN_R_LED, CLEAR_GPIO);

    start_led_watcher();

    LOG_DEBUG("Completed successfully");

    return 0;
}
