//
// Created by sr on 05/06/22.
//
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <stdlib.h>
#include "fw.h"


#define SET_ZOOM "/mnt/flash/vienna/motocam/set/outdu_set_image_zoom.sh"
#define SET_IMAGE_ROTATION "/mnt/flash/vienna/motocam/set/outdu_set_image_rotation.sh"
#define SET_IRCUT_FILTER "/mnt/flash/vienna/motocam/set/outdu_set_ircut_filter.sh"
#define SET_IR "/mnt/flash/vienna/motocam/set/outdu_set_ir.sh"
#define SET_DAYMODE "/mnt/flash/vienna/motocam/set/outdu_set_day_mode.sh"
#define SET_RESOLUTION "/mnt/flash/vienna/motocam/set/outdu_set_resolution.sh"
#define SET_MIRROR "/mnt/flash/vienna/motocam/set/outdu_set_mirror.sh"
#define SET_FLIP "/mnt/flash/vienna/motocam/set/outdu_set_flip.sh"
#define SET_TILT "/mnt/flash/vienna/motocam/set/outdu_set_tilt.sh"
#define SET_WDR "/mnt/flash/vienna/motocam/set/outdu_set_wdr.sh"
#define SET_EIS "/mnt/flash/vienna/motocam/set/outdu_set_eis.sh"

#define GET_IR "/mnt/flash/vienna/motocam/get/outdu_get_ir.sh"
#define GET_IRCUT_FILTER "/mnt/flash/vienna/motocam/get/outdu_get_ircut_filter.sh"
#define GET_RESOLUTION "/mnt/flash/vienna/motocam/get/outdu_get_resolution.sh"
#define GET_DAY_MODE "/mnt/flash/vienna/motocam/get/outdu_get_day_mode.sh"
#define GET_WDR "/mnt/flash/vienna/motocam/get/outdu_get_wdr.sh"
#define GET_EIS "/mnt/flash/vienna/motocam/get/outdu_get_eis.sh"
#define START_STREAM "/mnt/flash/vienna/motocam/set/outdu_start_stream.sh"
#define STOP_STREAM "/mnt/flash/vienna/motocam/set/outdu_stop_stream.sh"
#define SHUTDOWN "/mnt/flash/vienna/motocam/set/outdu_shutdown.sh"



pthread_mutex_t lock;

std::string exec(const char* cmd) {
//    printf("$$$$$$$$$$$$$$$$$$$$$1 %s\n", cmd);
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
//    printf("$$$$$$$$$$$$$$$$$$$$$2 %s\n", cmd);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
//    printf("$$$$$$$$$$$$$$$$$$$$$3 %s\n", cmd);
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
//        printf("$$$$$$$$$$$$$$$$$$$$$4 %s\n", cmd);

        result += buffer.data();
//        printf("$$$$$$$$$$$$$$$$$$$$$5 %s\n", cmd);
    }

//    printf("$$$$$$$$$$$$$$$$$$$$$%s\n", result.c_str());
    return result;
}


int8_t set_image_zoom(uint8_t image_zoom)
{
    printf("fw set_image_zoom %d\n", image_zoom);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_ZOOM, image_zoom);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_image_rotation(uint8_t image_rotation)
{
    printf("fw set_image_rotation %d\n", image_rotation);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_IMAGE_ROTATION, image_rotation);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_ir_cutfilter(OnOff on_off)
{
    printf("fw set_ir_cutfilter %d\n", on_off);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_IRCUT_FILTER, on_off);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_ir_led_brightness(uint8_t brightness)
{
    printf("fw set_ir_led_brightness %d\n", brightness);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_IR, brightness);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_day_mode(enum ON_OFF on_off){
    printf("fw set_day_night %d\n", on_off);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_DAYMODE, on_off);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_image_resolution(uint8_t resolution)
{
    printf("fw set_image_resolution %d\n", resolution);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_RESOLUTION, resolution);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_image_mirror(uint8_t mirror)
{
    printf("fw set_image_mirror %d\n", mirror);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_MIRROR, mirror);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_image_flip(uint8_t flip)
{
    printf("fw set_image_flip %d\n", flip);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_FLIP, flip);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_image_tilt(uint8_t tilt)
{
    printf("fw set_image_tilt %d\n", tilt);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_TILT, tilt);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_image_wdr(uint8_t wdr)
{
    printf("fw set_image_wdr %d\n", wdr);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_WDR, wdr);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_image_eis(uint8_t eis)
{
    printf("fw set_image_eis %d\n", eis);
    pthread_mutex_lock(&lock);
    char cmd[100];
    sprintf(cmd, "%s %d", SET_EIS, eis);
    exec(cmd);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t set_mic(enum ON_OFF on_off){
    return 0;
}

int8_t get_ir_cutfilter(OnOff *on_off)
{
    pthread_mutex_lock(&lock);
    std::string output = exec(GET_IRCUT_FILTER);
    *on_off=(OnOff) atoi(output.c_str());
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t get_ir_led_brightness(uint8_t *brightness)
{
    pthread_mutex_lock(&lock);
    std::string output = exec(GET_IR);
    *brightness=atoi(output.c_str());
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t get_image_resolution(uint8_t *resolution)
{
    pthread_mutex_lock(&lock);
    std::string output = exec(GET_RESOLUTION);
    *resolution=atoi(output.c_str());
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t get_wdr(uint8_t *wdr)
{
    pthread_mutex_lock(&lock);
    std::string output = exec(GET_WDR);
    *wdr=atoi(output.c_str());
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t get_eis(uint8_t *eis)
{
    pthread_mutex_lock(&lock);
    std::string output = exec(GET_EIS);
    *eis=atoi(output.c_str());
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t get_day_mode(uint8_t *day_mode){
    pthread_mutex_lock(&lock);
    std::string output = exec(GET_DAY_MODE);
    *day_mode=atoi(output.c_str());
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t get_mic(enum ON_OFF *on_off){
    return 0;
}

int8_t start_stream() {
    pthread_mutex_lock(&lock);
    exec(START_STREAM);
    pthread_mutex_unlock(&lock);
    return 0;
}
int8_t stop_stream() {
    pthread_mutex_lock(&lock);
    exec(STOP_STREAM);
    pthread_mutex_unlock(&lock);
    return 0;
}

int8_t set_wifi_hotspot_config(const char *ssid, const uint8_t encryption_type, const char *encryption_key, const char *ip_address, const char *subnetmask)
{
    return 0;
}
int8_t get_wifi_hotspot_config(char *ssid, uint8_t *encryption_type, char *encryption_key, char *ip_address, char *subnetmask)
{
    return 0;
}
int8_t set_wifi_client_config(const char *ssid, const uint8_t encryption_type, const char *encryption_key, const char *ip_address, const char *subnetmask)
{
    return 0;
}
int8_t get_wifi_client_config(char *ssid, uint8_t *encryption_type, char *encryption_key, char *ip_address, char *subnetmask)
{
    return 0;
}

int8_t shutdown_device()
{
    pthread_mutex_lock(&lock);
    exec(SHUTDOWN);
    pthread_mutex_unlock(&lock);
    return 0;
}



