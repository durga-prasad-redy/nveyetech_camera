#ifndef MOCK_FW_EXTRA_H
#define MOCK_FW_EXTRA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Video frequency types used by motocam_image_api_l2 (not in current fw_image.h) */
typedef enum {
  F25 = 1,
  F30 = 2,
  F50 = 3,
  F60 = 4
} VideoFrequency;

int8_t apply_video_frequency_change(VideoFrequency *freq);
int8_t get_video_frequency(VideoFrequency *freq);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_FW_EXTRA_H */
