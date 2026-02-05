#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>

#include "motocam_api_l2.h"
#include "motocam_config_api_l2.h"
#include "fw/fw_image.h"
#include "fw/fw_sensor.h"
#include "fw/fw_streaming.h"
#include "fw.h"
#include "l2/motocam_image_api_l2.h"
#include "l2/motocam_streaming_api_l2.h"
#include "l2/motocam_system_api_l2.h"

const char *motocam_default_config_file =
     "/mnt/flash/vienna/motocam/motocam_default_config";
const char *motocam_current_config_file =              
    "/mnt/flash/vienna/motocam/motocam_current_config";

int8_t writeConfigFile(const char *fileName, struct MotocamConfig *config);

int8_t set_config_defaulttofactory_l2() {
  printf("config_defaulttofactory_l2\n");
  int8_t ret = writeConfigFile(motocam_default_config_file, &factory_config);
  if (ret == -1) {
    return -1;
  }
  default_config = factory_config;
  return 0;
}

int8_t set_config_defaulttocurrent_l2() {
  printf("config_defaulttocurrent_l2\n");
  int8_t ret = writeConfigFile(motocam_default_config_file, &current_config);
  if (ret == -1) {
    return -1;
  }
  default_config = current_config;
  return 0;
}

int8_t set_config_currenttofactory_l2() {
  printf("config_currenttofactory_l2\n");
  int8_t ret = writeConfigFile(motocam_current_config_file, &factory_config);
  if (ret == -1) {
    return -1;
  }
  current_config = factory_config;
  return 0;
}

int8_t set_config_currenttodefault_l2() {
  printf("config_currenttodefault_l2\n");
  int8_t ret = writeConfigFile(motocam_current_config_file, &default_config);
  if (ret == -1) {
    return -1;
  }
  current_config = default_config;
  return 0;
}

int8_t get_config_factory_l2(uint8_t **config, uint8_t *length) {
  printf("get_config_factory_l2\n");
  *length = 14;
  *config = (uint8_t *)malloc(*length);
  (*config)[0] = factory_config.zoom;
  (*config)[1] = factory_config.rotation;
  (*config)[2] = factory_config.ircutfilter;
  (*config)[3] = factory_config.irledbrightness;
  (*config)[4] = factory_config.daymode;
  (*config)[5] = factory_config.resolution;
  (*config)[6] = factory_config.mirror;
  (*config)[7] = factory_config.flip;
  (*config)[8] = factory_config.tilt;
  (*config)[9] = factory_config.wdr;
  (*config)[10] = factory_config.eis;
  (*config)[11] = factory_config.gyroreader;
  (*config)[12] = factory_config.misc;
  (*config)[13] = factory_config.mic;
  return 0;
}

int8_t get_config_default_l2(uint8_t **config, uint8_t *length) {
  printf("get_config_default_l2\n");
  *length = 14;
  *config = (uint8_t *)malloc(*length);
  (*config)[0] = default_config.zoom;
  (*config)[1] = default_config.rotation;
  (*config)[2] = default_config.ircutfilter;
  (*config)[3] = default_config.irledbrightness;
  (*config)[4] = default_config.daymode;
  (*config)[5] = default_config.resolution;
  (*config)[6] = default_config.mirror;
  (*config)[7] = default_config.flip;
  (*config)[8] = default_config.tilt;
  (*config)[9] = default_config.wdr;
  (*config)[10] = default_config.eis;
  (*config)[11] = default_config.gyroreader;
  (*config)[12] = default_config.misc;
  (*config)[13] = default_config.mic;
  return 0;
}

void initialize_config(MotocamConfig *config) {
  printf("initialize_config\n");
  uint8_t *data = NULL;
  uint8_t length = 0;

  // Get the current configuration as a byte array
  if (get_config_current_l2(&data, &length) == 0 && data != NULL) {
    // Make sure we got the expected amount of data
    if (length >= 14) {
      // Unpack the byte array into the struct
      config->zoom = data[0];
      config->rotation = data[1];
      config->ircutfilter = data[2];
      config->irledbrightness = data[3];
      config->daymode = data[4];
      config->resolution = data[5];
      config->mirror = data[6];
      config->flip = data[7];
      config->tilt = data[8];
      config->wdr = data[9];
      config->eis = data[10];
      config->gyroreader = data[11];
      config->misc = data[12];
      config->mic = data[13];
    }

    // Free the allocated memory
    free(data);
  }
}

int8_t get_config_current_l2(uint8_t **config, uint8_t *length) {
  printf("get_config_current_l2\n");
  *length = 14;
  *config = (uint8_t *)malloc(*length);

  uint8_t zoom;
  get_image_zoom(&zoom);
  (*config)[0] = zoom;

  (*config)[1] = 1; // rotaion
  OnOff onOff;
  get_ir_cutfilter(&onOff);
  (*config)[2] = onOff;
  uint8_t ir;
  get_ir_led_brightness(&ir);
  (*config)[3] = ir;
  uint8_t day_mode;
  get_day_mode(&day_mode);
  (*config)[4] = day_mode;
  uint8_t resolution;
  get_image_resolution(&resolution);
  (*config)[5] = resolution;
  uint8_t mirror;
  get_mirror(&mirror);
  (*config)[6] = mirror;
  uint8_t flip;
  get_flip(&flip);
  (*config)[7] = flip;
  (*config)[8] = 1; // tilt
  uint8_t wdr;
  get_wdr(&wdr);
  (*config)[9] = wdr;
  uint8_t eis;
  get_eis(&eis);
  (*config)[10] = eis;
  uint8_t gyro_reader;
  get_gyro_reader(&gyro_reader);
  (*config)[11] = gyro_reader;
  uint8_t misc;
  get_image_misc(&misc);
  (*config)[12] = misc;
  // (*config)[13] = current_config.mic;
  (*config)[13] = 0;

  return 0;
}



int8_t get_config_streaming_config_l2(uint8_t **config, uint8_t *length) {
  printf("get_config_streaming_config_current_l2\n");
  
  // Parse stream.ini file
  std::string stream_ini_path = std::string(CONFIG_PATH) + "/stream.ini";
  
  // Get stream_count from [streamer] section
  int stream_count = get_ini_value(stream_ini_path, "streamer", "stream_count", 2);
  if (stream_count < 1 || stream_count > 10) {
    printf("Invalid stream_count: %d, defaulting to 2\n", stream_count);
    stream_count = 2;
  }
  
  // Allocate memory: 4 bytes per stream (resolution, fps, bitrate, encoder)
  *length = stream_count * 4;
  *config = (uint8_t *)malloc(*length);
  if (*config == NULL) {
    printf("Failed to allocate memory for streaming config\n");
    return -1;
  }
  
  // Parse each stream
  for (int i = 0; i < stream_count; i++) {
    char section[32];
    snprintf(section, sizeof(section), "stream%d", i);
    
    // Get width and height
    int width = get_ini_value(stream_ini_path, section, "width", 1920);
    int height = get_ini_value(stream_ini_path, section, "height", 1080);
    ImageResolution resolution = map_resolution(width, height);
    (*config)[i * 4 + 0] = resolution;
    
    // Get fps
    int fps = get_ini_value(stream_ini_path, section, "fps", 25);
    if (fps < 0 || fps > 255) {
      printf("Warning: Invalid fps %d for stream%d, defaulting to 25\n", fps, i);
      fps = 25;
    }
    (*config)[i * 4 + 1] = (uint8_t)fps;
    
    // Get bitrate (in bps from stream.ini, convert to Mbps to match original format)
    int bitrate_bps = get_ini_value(stream_ini_path, section, "bitrate", 2000000);
    // Convert from bps to Mbps (divide by 1000000)
    // Original config files store bitrate in Mbps as uint8_t
    int bitrate_mbps = bitrate_bps / 1000000;
    if (bitrate_mbps < 0) {
      bitrate_mbps = 0;
    } else if (bitrate_mbps > 255) {
      printf("Warning: Bitrate %d bps (%d Mbps) for stream%d exceeds uint8_t range, capping at 255\n", 
             bitrate_bps, bitrate_mbps, i);
      bitrate_mbps = 255;
    }
    (*config)[i * 4 + 2] = (uint8_t)bitrate_mbps;
    
    // Get codec/encoder
    int codec = get_ini_value(stream_ini_path, section, "codec", 1);
    Encoder encoder = map_encoder(codec);
    (*config)[i * 4 + 3] = encoder;
    
    printf("Stream %d: resolution=%d, fps=%d, bitrate=%d Mbps, encoder=%d\n", 
           i, resolution, fps, (*config)[i * 4 + 2], encoder);
  }

  return 0;
}

int8_t writeConfigFile(const char *fileName, struct MotocamConfig *config) {
  printf("writeToFile %s\n", fileName);
  FILE *f = fopen(fileName, "wb");
  if (f == NULL) {
    printf("open file error %s\n", fileName);
    return -1;
  }
  size_t write_bytes = fwrite(config, sizeof(struct MotocamConfig), 1, f);
  if (write_bytes <= 0) {
    printf("error writing bytes to file %s\n", fileName);
    fclose(f);
    return -1;
  }
  fclose(f);
  return 0;
}
