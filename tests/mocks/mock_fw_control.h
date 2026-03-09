#ifndef MOCK_FW_CONTROL_H
#define MOCK_FW_CONTROL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Control mock state so tests can trigger different code paths. */

/** Set value returned by get_day_mode (0 = OFF, 1 = ON). Affects init_motocam_configs and set_image_misc_l2. */
void set_mock_day_mode(uint8_t value);

/** Set value returned by get_image_misc (1..12 = image_mode_t). Affects init_motocam_configs set_misc_actions_on_boot. */
void set_mock_image_misc(uint8_t value);

/** Set value returned by get_stream_state (0 = stopped, 1 = running). */
void set_mock_stream_state(int8_t value);

/** If non-zero, set_image_zoom returns -1 to trigger L1 error path. */
void set_mock_set_image_zoom_fail(int fail);

/** If non-zero, set_image_misc returns -1 (set_image_misc_l2 returns -2). */
void set_mock_set_image_misc_fail(int fail);

/** If non-zero, get_webrtc_streaming_state returns -1. */
void set_mock_get_webrtc_fail(int fail);

/** If non-zero, get_ir_cutfilter fails. */
void set_mock_get_ir_cutfilter_fail(int fail);

/** If non-zero, get_gyro_reader returns -1. */
void set_mock_get_gyro_reader_fail(int fail);

/** If non-zero, get_day_mode returns -1 (used by get_gyroreader_l2). */
void set_mock_get_day_mode_fail(int fail);

/** If non-zero, get_image_resolution returns -1. */
void set_mock_get_image_resolution_fail(int fail);

/** get_wifi_state returns this (0, 1=Hotspot, 2=Client). */
void set_mock_wifi_state(uint8_t value);

/** If non-zero, start_stream (used by start_stream_l2) returns -1. */
void set_mock_start_stream_fail(int fail);
/** If non-zero, stop_stream returns -1. */
void set_mock_stop_stream_fail(int fail);
/** If non-zero, start_webrtc_stream returns -1. */
void set_mock_start_webrtc_stream_fail(int fail);
/** If non-zero, stop_webrtc_stream returns -1. */
void set_mock_stop_webrtc_stream_fail(int fail);

/** If non-zero, most fw image set calls fail with -1. */
void set_mock_fail_image_set(int fail);
/** If non-zero, most fw image get calls fail with -1. */
void set_mock_fail_image_get(int fail);

/** If non-zero, fw network set/get calls fail with -1. */
void set_mock_fail_network_fw(int fail);
/** If non-zero, fw system set/get calls fail with -1. */
void set_mock_fail_system_fw(int fail);

/** Force return value of set_factory_reset (default 0). */
void set_mock_set_factory_reset_ret(int8_t ret);
/** Force return value of set_config_reset (default 0). */
void set_mock_set_config_reset_ret(int8_t ret);
/** Force return value of set_login (default 0). */
void set_mock_set_login_ret(int8_t ret);

/** 0: empty login pin, 1: "1234" */
void set_mock_login_pin_mode(uint8_t mode);
/** 0: "US", 1: "U", 2: "" */
void set_mock_wifi_country_code_mode(uint8_t mode);
/** 0: empty hotspot/client config strings, 1: non-empty sample strings. */
void set_mock_network_config_nonempty(uint8_t mode);

/** Reset all mock overrides to defaults. */
void reset_mock_fw_control(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_FW_CONTROL_H */
