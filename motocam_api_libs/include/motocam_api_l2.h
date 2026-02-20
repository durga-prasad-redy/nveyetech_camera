//
// Created by sr on 2/6/25.
//

#ifndef TEST_GEN_MOTOCAM_HW_API_H
#define TEST_GEN_MOTOCAM_HW_API_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MotocamConfig {
  uint8_t zoom;
  uint8_t rotation;
  uint8_t ircutfilter;
  uint8_t irledbrightness;
  uint8_t daymode;
  uint8_t resolution;
  uint8_t mirror;
  uint8_t flip;
  uint8_t tilt;
  uint8_t wdr;
  uint8_t eis;
  uint8_t gyroreader;
  uint8_t misc;
  uint8_t mic;

} MotocamConfig;

int8_t init_motocam_configs();
struct MotocamConfig factory_config;
struct MotocamConfig default_config;
struct MotocamConfig current_config;

#ifdef __cplusplus
}
#endif

#endif // TEST_GEN_MOTOCAM_HW_API_H
