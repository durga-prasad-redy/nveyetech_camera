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
  uint8_t video_frequency;
} MotocamConfig;

// Function to initialize configs to zero or default values
int8_t init_motocam_configs();

extern struct MotocamConfig factory_config;
extern struct MotocamConfig default_config;
extern struct MotocamConfig current_config;

#ifdef __cplusplus
}
#endif

#endif // TEST_GEN_MOTOCAM_HW_API_H