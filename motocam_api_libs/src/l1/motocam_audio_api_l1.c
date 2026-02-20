#include "motocam_audio_api_l1.h"
#include "motocam_audio_api_l2.h"

int8_t set_audio_command(const uint8_t sub_command, const uint8_t data_length,
                         const uint8_t *data) {
  if (sub_command == MIC) {
    printf("data_length: %u\n", data_length);

    if (data != NULL) {
      printf("data: ");
      for (uint8_t i = 0; i < data_length; ++i) {
        printf("%02X ", data[i]);
      }
      printf("\n");
    } else {
      printf("data is NULL\n");
    }

    return 0;
  }

  printf("invalid sub command\n");
  return -4;
}

int8_t get_audio_command(const uint8_t sub_command, const uint8_t data_length,
                         const uint8_t *data, uint8_t **res_data_bytes,
                         uint8_t *res_data_bytes_size) {

  if (sub_command == MIC) {
    return get_audio_mic_l1(data_length, data, res_data_bytes,
                            res_data_bytes_size);
  }

  // Fallback for any unhandled sub_command
  printf("invalid sub command\n");
  return -4;
}

int8_t get_audio_mic_l1(const uint8_t data_length, const uint8_t *data,
                        uint8_t **res_data_bytes,
                        uint8_t *res_data_bytes_size) {
  (void)data;
  if (data_length == 0) {
    int8_t ret = get_audio_mic_l2(res_data_bytes, res_data_bytes_size);
    if (ret == 0) {
      return 0;
    }
    printf("error in executing the command\n");
    return -1;
  }
  printf("invalid data/data length\n");
  return -5;
}
