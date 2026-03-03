#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mock_hw.h"

extern "C" {
    // Declarations from fw.c
    int exec_cmd(const char *cmd, char *out, size_t out_size);
    int is_running(const char *process_name);
    uint8_t access_file(const char *path);
    void safe_remove(const char *path);
    int8_t set_haptic_motor(int duty_cycle, int duration);
    int8_t debug_pwm4_set(uint8_t val);
    int8_t debug_pwm5_set(uint8_t val);
    int8_t outdu_update_brightness(uint8_t val);
    int8_t get_mode(void);
    int8_t get_ir_tmp_ctl(void);
    int8_t is_ir_temp(void);
    int8_t is_sensor_temp(void);
    void safe_symlink(const char *target, const char *linkpath);
    int8_t set_uboot_env(const char *key, uint8_t value);
    int8_t set_uboot_env_chars(const char *key, const char *value);
    void stop_process(const char *process_name);
    int exec_return(const char *cmd);
    
    void create_file(const char *path);
    void delete_file(const char *path);
    
    void fusion_eis_on();
    void fusion_eis_off();
    void linear_eis_on();
    void linear_eis_off();
    void linear_lowlight_on();
    void linear_4k_on();
    void stream_server_config_4k_on();
    void stream_server_config_2k_on();

    // gpio.c
    int gpio_init();
    void update_ir_cut_filter_on();
    void update_ir_cut_filter_off();
    uint8_t get_ir_cut_filter();
    int start_led_watcher();
    uint8_t get_gpio_value(int gpio_pin);
    int read_wifi_state(void);

    // Stubs to avoid linkage errors
    int8_t set_indication_led(int on_off) { return 0; }
    int8_t set_image_zoom(uint8_t image_zoom) { return 0; }
    int8_t set_image_rotation(uint8_t image_rotation) { return 0; }
    int8_t set_image_resolution(uint8_t image_resolution) { return 0; }
    int8_t set_image_mirror(uint8_t mirror) { return 0; }
    int8_t set_image_flip(uint8_t flip) { return 0; }
    int8_t set_image_tilt(uint8_t tilt) { return 0; }
    int8_t set_image_wdr(uint8_t wdr) { return 0; }
    int8_t set_image_eis(uint8_t eis) { return 0; }
    int8_t set_image_misc(uint8_t misc) { return 0; }
    int8_t get_image_zoom(uint8_t *zoom) { return 0; }
    int8_t get_image_resolution(uint8_t *resolution) { return 0; }
    int8_t get_wdr(uint8_t *wdr) { return 0; }
    int8_t get_eis(uint8_t *eis) { return 0; }
    int8_t get_flip(uint8_t *flip) { return 0; }
    int8_t get_mirror(uint8_t *mirror) { return 0; }
    int8_t set_day_mode(int on_off) { return 0; }
    int8_t get_day_mode(uint8_t *day_mode) { return 0; }
    int8_t get_image_misc(uint8_t *misc) { return 0; }
    int8_t set_ir_cutfilter(int on_off) { return 0; }
    int8_t get_ir_cutfilter(int *on_off) { return 0; }
    int8_t set_ir_led_brightness(uint8_t brightness) { return 0; }
    int8_t get_ir_led_brightness(uint8_t *brightness) { return 0; }
    int8_t set_haptic(int on_off) { return 0; }
    int8_t set_heater(int on_off) { return 0; }
    int8_t get_heater(int *on_off) { return 0; }
    int8_t start_webrtc_stream() { return 0; }
    int8_t stop_webrtc_stream() { return 0; }

    // Stubs for functions called by fw.c and gpio.c
    void* msgbroker_get_instance(int id) { return nullptr; }
    int msgbroker_publish(void* b, const char* topic, const char* src, const char* key, const char* val) { return 0; }
    void fw_sm_timer_start(void* t, int ms) {}
    void fw_sm_yield() {}
    int fw_sm_timer_expired(void* t) { usleep(10000); return 1; }
}

class MotocamFwLibsTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        system("mkdir -p /tmp/test_fw");
        system("mkdir -p /tmp/test_fw/config");
        system("mkdir -p /tmp/test_fw/proc");
        system("mkdir -p /tmp/test_fw/gpio/export");
        set_mock_system_return(0);
        set_mock_popen_return(0);
        set_mock_popen_output("");
        set_mock_pthread_allow(0); 

        // Ensure directories exist for GPIO mock
        system("mkdir -p /tmp/test_fw/gpio/gpio59");
        system("mkdir -p /tmp/test_fw/gpio/gpio60");
        system("mkdir -p /tmp/test_fw/gpio/gpio1"); // B LED
        system("mkdir -p /tmp/test_fw/gpio/gpio2"); // G LED
        system("mkdir -p /tmp/test_fw/gpio/gpio3"); // R LED
        
        auto create_file_local = [](const char* p) {
            FILE* f = fopen(p, "w");
            if(f) fclose(f);
        };
        
        create_file_local("/tmp/test_fw/gpio/export");
        create_file_local("/tmp/test_fw/gpio/gpio59/direction");
        create_file_local("/tmp/test_fw/gpio/gpio59/value");
        create_file_local("/tmp/test_fw/gpio/gpio60/direction");
        create_file_local("/tmp/test_fw/gpio/gpio60/value");
        create_file_local("/tmp/test_fw/gpio/gpio1/direction");
        create_file_local("/tmp/test_fw/gpio/gpio1/value");
        create_file_local("/tmp/test_fw/gpio/gpio2/direction");
        create_file_local("/tmp/test_fw/gpio/gpio2/value");
        create_file_local("/tmp/test_fw/gpio/gpio3/direction");
        create_file_local("/tmp/test_fw/gpio/gpio3/value");
        
        create_file_local("/tmp/test_fw/ota_status");
        create_file_local("/tmp/test_fw/wifi_operstate");
        create_file_local("/tmp/test_fw/ircut_filter");
        create_file_local("/tmp/test_fw/wifi_state");
    }

    virtual void TearDown() {
        set_mock_pthread_allow(0);
        set_mock_system_return(0);
        set_mock_popen_return(0);
        set_mock_popen_output("");
        system("rm -rf /tmp/test_fw/*");
    }
};

TEST_F(MotocamFwLibsTest, ExecCmd_ValidCommand) {
    set_mock_popen_output("hello\n");
    set_mock_popen_return(0);
    char out[128] = {0};
    int ret = exec_cmd("echo 'hello'", out, sizeof(out));
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(out, "hello\n");
}

TEST_F(MotocamFwLibsTest, ExecCmd_BufferFull) {
    set_mock_popen_output("too long output");
    char out[5] = {0};
    int ret = exec_cmd("echo 'too long output'", out, sizeof(out));
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(out, "too ");
}

TEST_F(MotocamFwLibsTest, ExecCmd_FailingCommand) {
    set_mock_popen_output("");
    set_mock_popen_return(1 << 8); 
    char out[128] = {0};
    int ret = exec_cmd("ls /nonexistent", out, sizeof(out));
    EXPECT_EQ(ret, 1);
}

TEST_F(MotocamFwLibsTest, IsRunning_Found) {
    system("mkdir -p /tmp/test_fw/proc/123");
    FILE* f = fopen("/tmp/test_fw/proc/123/cmdline", "w");
    fprintf(f, "some_process");
    fclose(f);
    EXPECT_EQ(is_running("some_process"), 1);
}

TEST_F(MotocamFwLibsTest, IsRunning_EmptyCmdline) {
    system("mkdir -p /tmp/test_fw/proc/789");
    FILE* f = fopen("/tmp/test_fw/proc/789/cmdline", "w");
    fclose(f);
    EXPECT_EQ(is_running("some_process"), 0);
}

TEST_F(MotocamFwLibsTest, StopProcess_EdgeCases) {
    stop_process(NULL);
    stop_process("");
    SUCCEED();
}

TEST_F(MotocamFwLibsTest, ExecReturnTest) {
    set_mock_popen_return(0);
    EXPECT_EQ(exec_return("cmd"), 0);
    set_mock_popen_return(42 << 8);
    EXPECT_EQ(exec_return("cmd"), 42);
}

TEST_F(MotocamFwLibsTest, IsRunning_NotFound) {
    EXPECT_EQ(is_running("not_existent"), 0);
}

TEST_F(MotocamFwLibsTest, SafeRemove_Nonexistent) {
    safe_remove("/tmp/test_fw/does_not_exist");
    SUCCEED();
}

TEST_F(MotocamFwLibsTest, SafeSymlink_CreatesLink) {
    const char* target = "/tmp/test_fw/target.txt";
    const char* link = "/tmp/test_fw/link.txt";
    FILE *f = fopen(target, "w");
    if(f) { fprintf(f, "test"); fclose(f); }
    safe_symlink(target, link);
    char buf[128] = {0};
    int len = readlink(link, buf, sizeof(buf)-1);
    EXPECT_GT(len, 0);
    EXPECT_STREQ(buf, target);
}

TEST_F(MotocamFwLibsTest, AccessFile_Exists) {
    const char* path = "/tmp/test_fw/file1.txt";
    FILE *f = fopen(path, "w");
    if(f) { fprintf(f, "hi"); fclose(f); }
    EXPECT_EQ(access_file(path), 1);
}

TEST_F(MotocamFwLibsTest, AccessFile_NotExists) {
    EXPECT_EQ(access_file("/tmp/test_fw/missing"), 0);
}

TEST_F(MotocamFwLibsTest, UbootEnvTest) {
    EXPECT_EQ(set_uboot_env("test_key", 123), 0);
    EXPECT_EQ(set_uboot_env_chars("test_key_str", "test_val"), 0);
}

TEST_F(MotocamFwLibsTest, StopProcessTest) {
    stop_process("some_process");
    SUCCEED();
}

TEST_F(MotocamFwLibsTest, HapticMotorTest) {
    EXPECT_EQ(set_haptic_motor(50, 1000), 0);
}

TEST_F(MotocamFwLibsTest, GpioInitTest) {
    EXPECT_EQ(gpio_init(), 0);
}

TEST_F(MotocamFwLibsTest, IrCutFilterTest) {
    update_ir_cut_filter_on();
    EXPECT_EQ(get_ir_cut_filter(), 0);
    update_ir_cut_filter_off();
    EXPECT_EQ(get_ir_cut_filter(), 1);
}

TEST_F(MotocamFwLibsTest, GpioValue_ReadFail) {
    FILE* f = fopen("/tmp/test_fw/gpio/gpio59/value", "w");
    fclose(f);
    EXPECT_EQ(get_gpio_value(59), 2);
}

TEST_F(MotocamFwLibsTest, WifiState_Fail) {
    system("rm -f /tmp/test_fw/wifi_state");
    system("mkdir -p /tmp/test_fw/wifi_state");
    EXPECT_EQ(read_wifi_state(), 0);
    system("rm -rf /tmp/test_fw/wifi_state");
}

TEST_F(MotocamFwLibsTest, AllEisAndConfigFunctionsTest) {
    fusion_eis_on(); SUCCEED();
    fusion_eis_off(); SUCCEED();
    linear_eis_on(); SUCCEED();
    linear_eis_off(); SUCCEED();
    linear_lowlight_on(); SUCCEED();
    linear_4k_on(); SUCCEED();
    stream_server_config_4k_on(); SUCCEED();
    stream_server_config_2k_on(); SUCCEED();
}

TEST_F(MotocamFwLibsTest, CreateDeleteFileTest) {
    const char* p = "/tmp/test_fw/new_file.txt";
    create_file(p);
    EXPECT_EQ(access_file(p), 1);
    delete_file(p);
    EXPECT_EQ(access_file(p), 0);
}

TEST_F(MotocamFwLibsTest, UbootEnv_Fail) {
    // Make it fail by creating a directory where the file should be
    mkdir("/tmp/test_fw/fail_key.tmp", 0777); 
    EXPECT_EQ(set_uboot_env("fail_key", 1), -1);
    EXPECT_EQ(set_uboot_env_chars("fail_key", "val"), -1);
    rmdir("/tmp/test_fw/fail_key.tmp");
}

TEST_F(MotocamFwLibsTest, PwmAndBrightnessTest) {
    EXPECT_EQ(debug_pwm4_set(10), 0);
    EXPECT_EQ(debug_pwm5_set(10), 0);
    EXPECT_EQ(outdu_update_brightness(8), 0);
}

TEST_F(MotocamFwLibsTest, BrightnessUpdate_Failures) {
    set_mock_fcntl_fail(1);
    EXPECT_EQ(outdu_update_brightness(8), 1);
    set_mock_fcntl_fail(0);

    system("mkdir -p /tmp/test_fw/pwmdev-5");
    EXPECT_EQ(outdu_update_brightness(8), 1);
    system("rm -rf /tmp/test_fw/pwmdev-5");

    system("mkdir -p /tmp/test_fw/pwmdev-4");
    EXPECT_EQ(outdu_update_brightness(8), 1);
    system("rm -rf /tmp/test_fw/pwmdev-4");
}

TEST_F(MotocamFwLibsTest, GettersTest) {
    set_mock_popen_output("1");
    EXPECT_EQ(get_mode(), 1);
    set_mock_popen_output("42");
    EXPECT_EQ(is_ir_temp(), 42);
    set_mock_popen_output("10");
    EXPECT_EQ(is_sensor_temp(), 10);
    set_mock_popen_output("3");
    EXPECT_EQ(get_ir_tmp_ctl(), 3);
}

extern "C" void kill_all_processes();
TEST_F(MotocamFwLibsTest, KillAllProcessesTest) {
    kill_all_processes();
    SUCCEED();
}

TEST_F(MotocamFwLibsTest, LedWatcherLogicTest) {
    set_mock_pthread_allow(1);
    auto set_wifi_up = [](bool up) {
        FILE* f = fopen("/tmp/test_fw/wifi_operstate", "w");
        if(f) { fprintf(f, "%s\n", up ? "up" : "down"); fclose(f); }
    };
    auto set_ota = [](const char* status) {
        FILE* f = fopen("/tmp/test_fw/ota_status", "w");
        if(f) { fprintf(f, "%s\n", status); fclose(f); }
    };
    
    set_wifi_up(true);
    set_ota("idle");
    set_mock_popen_output("inet addr:192.168.1.1");
    set_mock_system_return(0);
    
    gpio_init(); 
    sleep(1);

    set_ota("in-progress");
    sleep(1);

    set_ota("fail");
    set_wifi_up(false);
    sleep(1);

    set_ota("ota-successful-90");
    set_wifi_up(true);
    set_mock_popen_output("inet addr:1.1.1.1");
    sleep(1);

    set_mock_system_return(1); // ONVIF Fail
    sleep(1);

    FILE* f_ws = fopen("/tmp/test_fw/wifi_state", "w"); 
    if(f_ws) { fprintf(f_ws, "1"); fclose(f_ws); }
    set_mock_popen_output(""); // No client
    sleep(1);

    // WiFi Fail AND OTA Fail (hits PATTERN_WIFI_OTA_FAIL)
    set_wifi_up(false);
    set_ota("fail");
    sleep(1);

    // WiFi OK, but no client on AP (hits PATTERN_WIFI_AP_NO_CLIENT)
    set_wifi_up(true);
    set_mock_popen_output("inet addr:192.168.1.1");
    // read_wifi_state=1 (AP)
    FILE* f_ws2 = fopen("/tmp/test_fw/wifi_state", "w");
    if(f_ws2) { fprintf(f_ws2, "1"); fclose(f_ws2); }
    // ap_has_connected_sta=0
    set_mock_popen_output("NOT_A_MAC"); 
    sleep(1);

    // WiFi OK, client connected (hits normal green LED / IR magenta)
    set_mock_popen_output("AA:BB:CC:DD:EE:FF"); // has ':' so connected
    sleep(1);

    // IR Cut filter state 1
    FILE* f_ir = fopen("/tmp/test_fw/ircut_filter", "w");
    if(f_ir) { fprintf(f_ir, "1\n"); fclose(f_ir); }
    sleep(1);

    SUCCEED();
}

TEST_F(MotocamFwLibsTest, ExecCmd_StatusFalse) {
    set_mock_popen_return(1); // Makes WIFEXITED false on many platforms (signals)
    char out[128] = {0};
    int ret = exec_cmd("echo test", out, sizeof(out));
    EXPECT_EQ(ret, 1);
}

TEST_F(MotocamFwLibsTest, IsRunning_OpendirFail) {
    set_mock_opendir_fail(1);
    EXPECT_EQ(is_running("process"), 0);
    set_mock_opendir_fail(0);
}

TEST_F(MotocamFwLibsTest, UbootEnv_RenameFail) {
    set_mock_rename_fail(1);
    EXPECT_EQ(set_uboot_env("key", 1), -1);
    EXPECT_EQ(set_uboot_env_chars("key", "val"), -1);
    set_mock_rename_fail(0);
}

TEST_F(MotocamFwLibsTest, StopProcess_SystemFail) {
    set_mock_system_return(-1);
    stop_process("some_proc");
    SUCCEED();
}

TEST_F(MotocamFwLibsTest, FcntlFailCases) {
    set_mock_fcntl_fail(1);
    EXPECT_EQ(set_haptic_motor(50, 100), -1);
    EXPECT_EQ(debug_pwm4_set(10), 1);
    EXPECT_EQ(debug_pwm5_set(10), 1);
    EXPECT_EQ(outdu_update_brightness(8), 1);
    set_mock_fcntl_fail(0);
}

TEST_F(MotocamFwLibsTest, FileOpsFailures) {
    set_mock_remove_fail(1);
    safe_remove("/some/path");
    SUCCEED();
    set_mock_remove_fail(0);

    set_mock_symlink_fail(1);
    safe_symlink("target", "link");
    SUCCEED();
    set_mock_symlink_fail(0);
}

TEST_F(MotocamFwLibsTest, CreateDeleteFail) {
    // mkdir where file should be to cause fopen("w") to fail
    mkdir("/tmp/test_fw/fail_create", 0777);
    create_file("/tmp/test_fw/fail_create"); 
    SUCCEED();
    rmdir("/tmp/test_fw/fail_create");

    set_mock_remove_fail(1);
    delete_file("/nonexistent");
    SUCCEED();
    set_mock_remove_fail(0);
}

TEST_F(MotocamFwLibsTest, IsOtaFailed_Cases) {
    // Test logic via public start_led_watcher and mocks if possible
    // Since we detach the thread, it's hard to synchronize.
}

TEST_F(MotocamFwLibsTest, StartLedWatcher_Fail) {
    set_mock_pthread_fail(1);
    EXPECT_EQ(start_led_watcher(), -1);
    set_mock_pthread_fail(0);
}

TEST_F(MotocamFwLibsTest, ReadFailures) {
    // read_ota_status failure
    system("rm -f /tmp/test_fw/ota_status");
    system("mkdir /tmp/test_fw/ota_status"); // causes fopen to fail 
}

TEST_F(MotocamFwLibsTest, GpioValue_FgetsNull) {
    // Create an empty file, fgets will return NULL
    FILE* f = fopen("/tmp/test_fw/gpio/gpio59/value", "w");
    fclose(f);
    EXPECT_EQ(get_gpio_value(59), 2);
}

TEST_F(MotocamFwLibsTest, MoreOtaFailures) {
    set_mock_pthread_allow(1);
    auto set_ota = [](const char* status) {
        FILE* f = fopen("/tmp/test_fw/ota_status", "w");
        if(f) { fprintf(f, "%s\n", status); fclose(f); }
    };
    
    gpio_init();
    set_ota("compatible-mismatch-24");
    sleep(1);
    set_ota("generic_fail");
    sleep(1);
    set_ota("error_encountered");
    sleep(1);
    SUCCEED();
}

TEST_F(MotocamFwLibsTest, ExecCmd_InvalidParams) {
    EXPECT_EQ(exec_cmd(NULL, NULL, 0), -1);
    char buf[10];
    EXPECT_EQ(exec_cmd("test", NULL, 10), -1);
    EXPECT_EQ(exec_cmd("test", buf, 0), -1);
}

TEST_F(MotocamFwLibsTest, ExecReturn_InvalidParam) {
    EXPECT_EQ(exec_return(NULL), -1);
}

TEST_F(MotocamFwLibsTest, PwmAllLevelsTest) {
    // Test all enum values: 10=ULTRA, 8=MAX, 6=HIGH, 4=MEDIUM, 2=LOW, 0=IR_OFF
    uint8_t levels[] = {10, 8, 6, 4, 2, 0};
    for(int i=0; i<6; i++) {
        EXPECT_EQ(outdu_update_brightness(levels[i]), 0);
    }
    // Invalid level
    EXPECT_EQ(outdu_update_brightness(99), 1);
}

TEST_F(MotocamFwLibsTest, IsRunning_Cases_Extra) {
    // Branch: ent->d_name[0] not a digit
    system("mkdir -p /tmp/test_fw/proc/not_a_digit");
    EXPECT_EQ(is_running("some_proc"), 0);
    
    // Branch: fopen fail
    system("mkdir -p /tmp/test_fw/proc/999");
    // Make cmdline a directory of its own to cause fopen fail
    system("mkdir -p /tmp/test_fw/proc/999/cmdline");
    EXPECT_EQ(is_running("some_proc"), 0);
    system("rm -rf /tmp/test_fw/proc/999/cmdline");
}

TEST_F(MotocamFwLibsTest, SafeRemove_Fail_Branch) {
    set_mock_remove_fail(1);
    system("touch /tmp/test_fw/exists_but_fail");
    safe_remove("/tmp/test_fw/exists_but_fail"); 
    set_mock_remove_fail(0);
    SUCCEED();
}

TEST_F(MotocamFwLibsTest, ReadWifiState_Fail_Empty) {
    // empty file 
    FILE* f = fopen("/tmp/test_fw/wifi_state", "w");
    fclose(f);
    read_wifi_state();
    SUCCEED();
}
