#include <cstdio>
#include <memory>
#include <new>
#include "motocam_api_l2.h"
#include "motocam_image_api_l2.h"
#include "fw/fw_image.h"
#include "fw/fw_sensor.h"

int8_t set_image_zoom_l2(const uint8_t zoom) {
  printf("set_image_zoom_l2 %d\n", zoom);
  if (set_image_zoom(zoom) < 0) {
    return -1;
  }
  current_config.zoom = zoom;
  return 0;
}
int8_t set_image_rotation_l2(const uint8_t rotation) {
  printf("set_image_rotation_l2 %d\n", rotation);
  if (set_image_rotation(rotation) < 0) {
    return -1;
  }
  current_config.rotation = rotation;
  return 0;
}
int8_t set_image_ircutfilter_l2(const uint8_t ircutfilter) {
  printf("set_image_ircutfilter_l2\n");
  printf("set_image_ircutfilter_l2 %d\n", ircutfilter);
  if (ircutfilter == 1) {
    printf("set_image_ircutfilter_l2 1 %d\n", ircutfilter);
    if (set_ir_cutfilter(ON) < 0) {
      return -1;
    }
    current_config.ircutfilter = ircutfilter;
    return 0;
  }
  if (ircutfilter == 0) {
    if (set_ir_cutfilter(OFF) < 0) {
      return -1;
    }
    current_config.ircutfilter = ircutfilter;
    return 0;
  }
  return -1;
}
int8_t set_image_irbrightness_l2(const uint8_t irbrightness) {
  printf("set_image_irbrightness_l2 %d\n", irbrightness);
  if (set_ir_led_brightness(irbrightness) < 0) {
    return -1;
  }
  current_config.irledbrightness = irbrightness;
  return 0;
}
int8_t set_image_mid_irbrightness_l2(const uint8_t irbrightness) {
  printf("set_image_mid_irbrightness_l2 %d\n", irbrightness);
  if (debug_pwm5_set(irbrightness) < 0) {
    return -1;
  }
  return 0;
}
int8_t set_image_side_irbrightness_l2(const uint8_t irbrightness) {
  printf("set_image_side_irbrightness_l2 %d\n", irbrightness);
  if (debug_pwm4_set(irbrightness) < 0) {
    return -1;
  }
  return 0;
}
int8_t set_image_daymode_l2(const uint8_t daymode) {
  printf("set_image_daymode_l2 %d\n", daymode);
  if (daymode == 1) {
    if (set_day_mode(ON) < 0) {
      return -1;
    }
    current_config.daymode = daymode;
    return 0;
  }
  if (daymode == 0) {
    if (set_day_mode(OFF) < 0) {
      return -1;
    }
    current_config.daymode = daymode;
    return 0;
  }
  return -1;
}
int8_t set_gyroreader_l2(const uint8_t gyroreader) {
  printf("set_gyroreader_l2 %d\n", gyroreader);
  if (gyroreader == 1) {
    if (set_gyro_reader(ON) < 0) {
      return -1;
    }
    current_config.gyroreader = gyroreader;
    return 0;
  }
  if (gyroreader == 0) {
    if (set_gyro_reader(OFF) < 0) {
      return -1;
    }
    current_config.gyroreader = gyroreader;
    return 0;
  }
  return -1;
}
int8_t set_image_resolution_l2(const uint8_t resolution) {
  printf("set_image_resolution_l2 %d\n", resolution);
  if (set_image_resolution(resolution) < 0) {
    return -1;
  }
  return 0;
}
int8_t set_image_mirror_l2(const uint8_t mirror) {
  printf("set_image_mirror_l2 %d\n", mirror);
  if (set_image_mirror(mirror) < 0) {
    return -1;
  }
  return 0;
}
int8_t set_image_flip_l2(const uint8_t flip) {
  printf("set_image_flip_l2 %d\n", flip);
  if (set_image_flip(flip) < 0) {
    return -1;
  }
  return 0;
}
int8_t set_image_tilt_l2(const uint8_t tilt) {
  printf("set_image_tilt_l2 %d\n", tilt);
  if (set_image_tilt(tilt) < 0) {
    return -1;
  }
  current_config.tilt = tilt;
  return 0;
}
int8_t set_image_wdr_l2(const uint8_t wdr) {
  printf("set_image_wdr_l2 %d\n", wdr);
  if (set_image_wdr(wdr) < 0) {
    return -1;
  }
  return 0;
}
int8_t set_image_eis_l2(const uint8_t eis) {
  printf("set_image_eis_l2 %d\n", eis);
  if (set_image_eis(eis) < 0) {
    return -1;
  }
  return 0;
}

int8_t set_image_misc_l2(const uint8_t misc) {
  printf("set_image_misc_l2 %d\n", misc);
  uint8_t day_mode;
  get_day_mode(&day_mode);
  printf("set_image_misc_l2 current day_mode=%d\n", day_mode);
  if(day_mode ==1)
  {
    printf("Cannot set misc while in auto day/night mode\n");
     return -1;}
  if (set_image_misc(misc) < 0) {
    return -2;
  }
  current_config.misc = misc;
  return 0;
}

int8_t get_image_zoom_l2(uint8_t **zoom, uint8_t *length) {
  printf("get_image_zoom_l2\n");
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = current_config.zoom;
  *zoom = buf.release();
  return 0;
}
int8_t get_image_rotation_l2(uint8_t **rotation, uint8_t *length) {
  printf("get_image_rotation_l2\n");
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = current_config.rotation;
  *rotation = buf.release();
  return 0;
}
int8_t get_image_ircutfilter_l2(uint8_t **ircutfilter, uint8_t *length) {
  printf("get_image_ircutfilter_l2\n");
  enum ON_OFF ir_cutfilter;
  if (get_ir_cutfilter(&ir_cutfilter) < 0) {
    return -1;
  }
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = ir_cutfilter;
  *ircutfilter = buf.release();
  return 0;
}
int8_t get_image_irbrightness_l2(uint8_t **irbrightness, uint8_t *length) {
  printf("get_image_irbrightness_l2\n");
  uint8_t ir_led_brightness;
  if (get_ir_led_brightness(&ir_led_brightness) < 0) {
    return -1;
  }
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = ir_led_brightness;
  *irbrightness = buf.release();
  return 0;
}
int8_t get_image_daymode_l2(uint8_t **daymode, uint8_t *length) {
  printf("get_image_daymode_l2\n");
  uint8_t day_mode;
  if (get_day_mode(&day_mode) < 0) {
    return -1;
  }
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = day_mode;
  *daymode = buf.release();
  return 0;
}
int8_t get_gyroreader_l2(uint8_t **gyroreader, uint8_t *length) {
  printf("get_gyroreader_l2\n");
  uint8_t day_mode;
  if (get_day_mode(&day_mode) < 0) {
    return -1;
  }
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = day_mode;
  *gyroreader = buf.release();
  return 0;
}
int8_t get_image_resolution_l2(uint8_t **mirror, uint8_t *length) {
  printf("get_image_resolution_l2\n");
  uint8_t resolution;
  if (get_image_resolution(&resolution) < 0) {
    return -1;
  }
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = resolution;
  *mirror = buf.release();
  return 0;
}
int8_t get_wdr_l2(uint8_t **mirror, uint8_t *length) {
  printf("get_wdr_l2\n");
  uint8_t wdr;
  if (get_wdr(&wdr) < 0) {
    return -1;
  }
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = wdr;
  *mirror = buf.release();
  return 0;
}
int8_t get_eis_l2(uint8_t **mirror, uint8_t *length) {
  printf("get_eis_l2\n");
  uint8_t eis;
  if (get_eis(&eis) < 0) {
    return -1;
  }
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = eis;
  *mirror = buf.release();
  return 0;
}
int8_t get_image_mirror_l2(uint8_t **mirror, uint8_t *length) {
  printf("get_image_mirror_l2\n");
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = current_config.mirror;
  *mirror = buf.release();
  return 0;
}
int8_t get_image_flip_l2(uint8_t **flip, uint8_t *length) {
  printf("get_image_flip_l2\n");
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = current_config.flip;
  *flip = buf.release();
  return 0;
}
int8_t get_image_tilt_l2(uint8_t **tilt, uint8_t *length) {
  printf("get_image_tilt_l2\n");
  *length = 1;
  std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[*length]);
  if (!buf) return -1;
  buf[0] = current_config.tilt;
  *tilt = buf.release();
  return 0;
}
