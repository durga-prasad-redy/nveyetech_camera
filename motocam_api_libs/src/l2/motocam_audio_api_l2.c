#include "motocam_audio_api_l2.h"
#include "fw/fw_audio.h"
#include "motocam_api_l2.h"
#include "motocam_command_enums.h"
#include <stdio.h>

// int8_t set_audio_mic_l2(const uint8_t mic) {
//   printf("set_mic_l2 %d\n", mic);
//   if (mic == 1) {
//     if (set_mic(ON) < 0) {
//       return -1;
//     }
//     current_config.mic = mic;
//     // writeConfigFile(motocam_current_config_file, &current_config);
//     return 0;
//   }
//   if (mic == 0) {
//     if (set_mic(OFF) < 0) {
//       return -1;
//     }
//     current_config.mic = mic;
//     // writeConfigFile(motocam_current_config_file, &current_config);
//     return 0;
//   }
//   return -1;
// }
int8_t get_audio_mic_l2(uint8_t **mic, uint8_t *length) {
  printf("get_mic_l2\n");
  enum ON_OFF mic_e;
  if (get_mic(&mic_e) < 0) {
    return -1;
  }
  *length = 1;
  *mic = (uint8_t *)malloc(*length);
  (*mic)[0] = mic_e;
  return 0;
}
