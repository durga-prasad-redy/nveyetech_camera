/**
 * Unit tests for motocam_api_libs: L1 do_processing, CRC/validation, L2 config init.
 */
#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "motocam_api_l1.h"
#include "motocam_api_l2.h"
#include "motocam_command_enums.h"
#include "l1/motocam_audio_api_l1.h"
#include "l1/motocam_config_api_l1.h"
#include "l1/motocam_image_api_l1.h"
#include "l1/motocam_network_api_l1.h"
#include "l1/motocam_streaming_api_l1.h"
#include "l1/motocam_system_api_l1.h"
#include "l2/motocam_network_api_l2.h"
#include "l2/motocam_system_api_l2.h"
#include "mock_fw_control.h"
}

/* L1 packet layout: [header, command, sub_command, data_length, data..., crc] */
/* CRC valid when sum of all bytes (including crc) mod 256 == 0 (validate_req_bytes_crc) */

static void build_valid_crc_packet(uint8_t *out, size_t cap,
                                   uint8_t header, uint8_t cmd, uint8_t sub,
                                   uint8_t data_len, const uint8_t *data,
                                   size_t *out_len) {
  size_t n = 4 + data_len;
  if (cap < n + 1) {
    *out_len = 0;
    return;
  }
  out[0] = header;
  out[1] = cmd;
  out[2] = sub;
  out[3] = data_len;
  if (data_len && data)
    memcpy(out + 4, data, data_len);
  uint64_t sum = 0;
  for (size_t i = 0; i < n; i++)
    sum += out[i];
  /* crc = get2sComplement(sum) so that sum + crc ≡ 0 (mod 256) */
  uint8_t crc = (uint8_t)((sum ^ 255) + 1);
  out[n] = crc;
  *out_len = n + 1;
}

class MotocamApiLibsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    system("mkdir -p /tmp/test_api/config");
    system("mkdir -p /tmp/test_api/m5s_config");
    reset_mock_fw_control();
  }

  void TearDown() override {
    /* Free any response from do_processing */
  }
};

TEST_F(MotocamApiLibsTest, DoProcessing_InvalidCRC_ReturnsErrorResponse) {
  /* Packet with invalid CRC: sum of bytes != 0 mod 256 */
  uint8_t req[] = { 1, 4, 1, 0, 0 }; /* SET, IMAGE, ZOOM, len=0, bad crc */
  req[4] = 0x00; /* sum = 1+4+1+0+0 = 6, not 0 */
  uint8_t *res = nullptr;
  uint8_t res_size = 0;

  int8_t ret = do_processing(req, sizeof(req), &res, &res_size);

  ASSERT_EQ(ret, 0);
  ASSERT_NE(res, nullptr);
  ASSERT_GE(res_size, 6u);
  /* Response: [header, command, sub_command, data_len, failed=1, err=-6, crc] */
  EXPECT_EQ(res[0], (uint8_t)1);   /* echo header */
  EXPECT_EQ(res[1], (uint8_t)4);   /* IMAGE */
  EXPECT_EQ(res[2], (uint8_t)1);   /* ZOOM */
  EXPECT_EQ(res[4], 1u);            /* failed */
  EXPECT_EQ((int8_t)res[5], -6);    /* invalid CRC err */
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_InvalidHeader_ReturnsErrorResponse) {
  /* Valid CRC but header not SET(1) or GET(2) */
  uint8_t req[5];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), 99, 4, 1, 0, nullptr, &len);
  ASSERT_EQ(len, 5u);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;

  int8_t ret = do_processing(req, (uint8_t)len, &res, &res_size);

  ASSERT_EQ(ret, 0);
  ASSERT_NE(res, nullptr);
  ASSERT_GE(res_size, 6u);
  EXPECT_EQ(res[4], 1u);            /* failed */
  EXPECT_EQ((int8_t)res[5], -2);    /* invalid header */
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_ImageZoom_ValidCRC_ReturnsAck) {
  /* SET, IMAGE, ZOOM, data_len=1, data=[1], crc */
  uint8_t data = 1;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, ZOOM, 1, &data, &len);
  ASSERT_GE(len, 6u);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;

  int8_t ret = do_processing(req, (uint8_t)len, &res, &res_size);

  ASSERT_EQ(ret, 0);
  ASSERT_NE(res, nullptr);
  ASSERT_GE(res_size, 6u);
  EXPECT_EQ(res[0], (uint8_t)ACK);  /* ACK = 3 */
  EXPECT_EQ(res[1], (uint8_t)IMAGE);
  EXPECT_EQ(res[2], (uint8_t)ZOOM);
  EXPECT_EQ(res[4], 0u);             /* success */
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_ImageZoom_ValidCRC_ReturnsResponse) {
  /* GET, IMAGE, ZOOM, data_len=0, crc */
  uint8_t req[6];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, IMAGE, ZOOM, 0, nullptr, &len);
  ASSERT_GE(len, 5u);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;

  int8_t ret = do_processing(req, (uint8_t)len, &res, &res_size);

  ASSERT_EQ(ret, 0);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[0], (uint8_t)RESPONSE); /* RESPONSE = 4 */
  EXPECT_EQ(res[1], (uint8_t)IMAGE);
  EXPECT_EQ(res[2], (uint8_t)ZOOM);
  EXPECT_EQ(res[4], 0u);               /* success */
  free(res);
}

TEST_F(MotocamApiLibsTest, InitMotocamConfigs_Succeeds) {
  int8_t ret = init_motocam_configs();
  EXPECT_EQ(ret, 0);
}

TEST_F(MotocamApiLibsTest, DoProcessing_InvalidCommand_ReturnsError) {
  /* SET with invalid command (e.g. 0 or 99), valid CRC */
  uint8_t req[6];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, 99, 0, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;

  int8_t ret = do_processing(req, (uint8_t)len, &res, &res_size);

  ASSERT_EQ(ret, 0);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[0], (uint8_t)ACK);
  EXPECT_EQ(res[4], 1u);   /* failed */
  EXPECT_EQ((int8_t)res[5], -3);  /* invalid command */
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Config_DefaultToCurrent_Deprecated) {
  /* Config DefaultToCurrent with data_length=0 is deprecated, returns -1 */
  uint8_t req[6];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, CONFIG, 11, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  int8_t ret = do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_EQ(ret, 0);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[0], (uint8_t)ACK);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
}

/* ---- do_processing GET failure path (ret != 0, res_data_bytes may be NULL) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_InvalidImageSubCommand_ReturnsFailed) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, IMAGE, 99, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[0], (uint8_t)RESPONSE);
  EXPECT_EQ(res[4], 1u);
  EXPECT_EQ((int8_t)res[5], -4);
  free(res);
}

/* ---- IMAGE SET: all sub-commands (success) and invalid data length / invalid sub ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_AllSubCommands_Success) {
  uint8_t d = 1;
  uint8_t req[8];
  size_t len = 0;
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  /* MISC fails when day_mode is auto (mock returns 1). VIDEO_FREQUENCY valid range is F50(3)-F60(4). */
  const uint8_t subs[] = { ZOOM, ROTATION, IRCUTFILTER, IRBRIGHTNESS, DAYMODE, RESOLUTION,
                           MIRROR, FLIP, TILT, WDR, EIS, GYROREADER,
                           MID_IRBRIGHTNESS, SIDE_IRBRIGHTNESS };
  for (unsigned i = 0; i < sizeof(subs)/sizeof(subs[0]); i++) {
    build_valid_crc_packet(req, sizeof(req), SET, IMAGE, subs[i], 1, &d, &len);
    do_processing(req, (uint8_t)len, &res, &res_size);
    ASSERT_NE(res, nullptr) << " sub " << (int)subs[i];
    EXPECT_EQ(res[0], (uint8_t)ACK);
    EXPECT_EQ(res[4], 0u) << " sub " << (int)subs[i];
    free(res);
    res = nullptr;
  }
  /* VIDEO_FREQUENCY: valid value 3 (F50) or 4 (F60) */
  uint8_t vf = 3;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, VIDEO_FREQUENCY, 1, &vf, &len);
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_Misc_WhenDayModeAuto_ReturnsError) {
  uint8_t d = 1;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, MISC, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_InvalidDataLength_ReturnsMinus5) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, ZOOM, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_InvalidSubCommand_ReturnsMinus4) {
  uint8_t d = 1;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, 99, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -4);
  free(res);
}

/* ---- IMAGE GET: all sub-commands and invalid data length / invalid sub ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_Image_AllSubCommands_Success) {
  uint8_t req[8];
  size_t len = 0;
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  const uint8_t subs[] = { ZOOM, ROTATION, IRCUTFILTER, IRBRIGHTNESS, DAYMODE, RESOLUTION,
                           MIRROR, FLIP, TILT, WDR, EIS, VIDEO_FREQUENCY, GYROREADER };
  for (unsigned i = 0; i < sizeof(subs)/sizeof(subs[0]); i++) {
    build_valid_crc_packet(req, sizeof(req), GET, IMAGE, subs[i], 0, nullptr, &len);
    do_processing(req, (uint8_t)len, &res, &res_size);
    ASSERT_NE(res, nullptr) << " sub " << (int)subs[i];
    EXPECT_EQ(res[0], (uint8_t)RESPONSE);
    EXPECT_EQ(res[4], 0u) << " sub " << (int)subs[i];
    free(res);
    res = nullptr;
  }
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_Image_InvalidDataLength_ReturnsMinus5) {
  uint8_t req[8];
  size_t len = 0;
  uint8_t d = 0;
  build_valid_crc_packet(req, sizeof(req), GET, IMAGE, ZOOM, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

/* ---- CONFIG: all set (deprecated) and get ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Config_AllDeprecated_ReturnError) {
  const uint8_t subs[] = { 9 /* DefaultToFactory */, 11 /* DefaultToCurrent */,
                           13 /* CurrentToFactory */, 14 /* CurrentToDefault */ };
  for (unsigned i = 0; i < sizeof(subs)/sizeof(subs[0]); i++) {
    uint8_t req[8];
    size_t len = 0;
    build_valid_crc_packet(req, sizeof(req), SET, CONFIG, subs[i], 0, nullptr, &len);
    uint8_t *res = nullptr;
    uint8_t res_size = 0;
    do_processing(req, (uint8_t)len, &res, &res_size);
    ASSERT_NE(res, nullptr);
    EXPECT_EQ((int8_t)res[5], -1);
    free(res);
  }
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Config_DataLengthNonZero_ReturnsMinus5) {
  uint8_t d = 0;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, CONFIG, 11, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_Config_AllSubCommands_Success) {
  const uint8_t subs[] = { 4 /* Factory */, 8 /* Default */, 12 /* Current */, 10 /* StreamingConfig */ };
  for (unsigned i = 0; i < sizeof(subs)/sizeof(subs[0]); i++) {
    uint8_t req[8];
    size_t len = 0;
    build_valid_crc_packet(req, sizeof(req), GET, CONFIG, subs[i], 0, nullptr, &len);
    uint8_t *res = nullptr;
    uint8_t res_size = 0;
    do_processing(req, (uint8_t)len, &res, &res_size);
    ASSERT_NE(res, nullptr) << " sub " << (int)subs[i];
    EXPECT_EQ(res[0], (uint8_t)RESPONSE);
    EXPECT_EQ(res[4], 0u);
    free(res);
  }
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_Config_InvalidSub_ReturnsMinus4) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, CONFIG, 99, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -4);
  free(res);
}

/* ---- STREAMING: set and get ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Streaming_AllSubCommands_Success) {
  const uint8_t subs[] = { START_STREAMING, STOP_STREAMING, START_WEBRTC_STREAMING, STOP_WEBRTC_STREAMING };
  for (unsigned i = 0; i < sizeof(subs)/sizeof(subs[0]); i++) {
    uint8_t req[8];
    size_t len = 0;
    build_valid_crc_packet(req, sizeof(req), SET, STREAMING, subs[i], 0, nullptr, &len);
    uint8_t *res = nullptr;
    uint8_t res_size = 0;
    do_processing(req, (uint8_t)len, &res, &res_size);
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res[4], 0u) << " sub " << (int)subs[i];
    free(res);
  }
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_Streaming_State_And_WebRTC_Success) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, STREAMING, STREAM_STATE, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[0], (uint8_t)RESPONSE);
  EXPECT_EQ(res[4], 0u);
  free(res);

  build_valid_crc_packet(req, sizeof(req), GET, STREAMING, WEBRTC_STREAMING_STATUS, 0, nullptr, &len);
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_Streaming_InvalidSub_ReturnsMinus4) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, STREAMING, 99, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -4);
  free(res);
}

/* ---- AUDIO ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Audio_Mic_Success) {
  uint8_t data[4] = { 0x01, 0x02, 0x03, 0x04 };
  uint8_t req[16];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, AUDIO, MIC, 4, data, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_Audio_Mic_Success) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, AUDIO, MIC, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_Audio_InvalidSub_ReturnsMinus4) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, AUDIO, 99, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -4);
  free(res);
}

/* ---- NETWORK: set and get ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_EthernetDhcp_Success) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, ETHERNET_DHCP, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_Onvif_Success) {
  /* L2 returns -2 if new state equals current; mock get_onvif_interface_state returns 0, so use 1 (WIFI) */
  uint8_t iface = 1;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, Onvif, 1, &iface, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_WifiCountryCode_Success) {
  /* L2 expects data[0]=length (2), then data[1], data[2] = two-char code */
  uint8_t data[3] = { 2, 'U', 'S' };
  uint8_t req[12];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, WifiCountryCode, 3, data, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_Network_AllSubCommands_Success) {
  const uint8_t subs[] = { WifiHotspot, WifiClient, WifiState, ETHERNET, Onvif, WifiCountryCode };
  for (unsigned i = 0; i < sizeof(subs)/sizeof(subs[0]); i++) {
    uint8_t req[8];
    size_t len = 0;
    build_valid_crc_packet(req, sizeof(req), GET, NETWORK, subs[i], 0, nullptr, &len);
    uint8_t *res = nullptr;
    uint8_t res_size = 0;
    do_processing(req, (uint8_t)len, &res, &res_size);
    ASSERT_NE(res, nullptr) << " sub " << (int)subs[i];
    EXPECT_EQ(res[4], 0u);
    free(res);
  }
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_InvalidLength_ReturnsMinus5) {
  uint8_t d = 0;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, Onvif, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_Network_InvalidSub_ReturnsMinus4) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, NETWORK, 99, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -4);
  free(res);
}

/* ---- SYSTEM: set and get ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_Shutdown_Success) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, SHUTDOWN, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_OtaUpdate_Success) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, OTA_UPDATE, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_HapticMotor_Success) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, HAPTIC_MOTOR, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_CameraName_Success) {
  uint8_t name[] = "TestCam";
  uint8_t req[24];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, SETCAMERANAME, sizeof(name)-1, name, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_UserDob_Success) {
  uint8_t dob[10] = { '0', '1', '-', '0', '1', '-', '2', '0', '0', '0' };
  uint8_t req[24];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, SET_USER_DOB, 10, dob, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_Time_Success) {
  /* L2 expects each byte to be digit 0-9 (value), not ASCII */
  uint8_t epoch[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
  uint8_t req[24];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, SET_TIME, sizeof(epoch), epoch, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_ProvisionDevice_Success) {
  uint8_t data[20] = { 0 };
  uint8_t req[32];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, PROVISION_DEVICE, 20, data, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_FactoryReset_InvalidLength_ReturnsMinus5) {
  uint8_t d = 0;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, FACTORY_RESET, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_ConfigReset_InvalidLength_ReturnsMinus5) {
  uint8_t d = 0;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, CONFIG_RESET, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_System_AllSubCommands_Success) {
  const uint8_t subs[] = { GETCAMERANAME, FIRMWAREVERSION, MACADDRESS, OTA_UPDATE_STATUS, HEALTH_CHECK, GET_USER_DOB };
  for (unsigned i = 0; i < sizeof(subs)/sizeof(subs[0]); i++) {
    uint8_t req[8];
    size_t len = 0;
    build_valid_crc_packet(req, sizeof(req), GET, SYSTEM, subs[i], 0, nullptr, &len);
    uint8_t *res = nullptr;
    uint8_t res_size = 0;
    do_processing(req, (uint8_t)len, &res, &res_size);
    ASSERT_NE(res, nullptr) << " sub " << (int)subs[i];
    EXPECT_EQ(res[0], (uint8_t)RESPONSE);
    EXPECT_EQ(res[4], 0u);
    free(res);
  }
}

TEST_F(MotocamApiLibsTest, DoProcessing_GET_System_InvalidSub_ReturnsMinus4) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, SYSTEM, 99, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -4);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_Shutdown_InvalidLength_ReturnsMinus5) {
  uint8_t d = 0;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, SHUTDOWN, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

/* ---- STREAMING invalid data length ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_Streaming_InvalidDataLength_ReturnsMinus5) {
  uint8_t d = 0;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, STREAMING, STREAM_STATE, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

/* ---- CONFIG GET invalid data length ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_Config_InvalidDataLength_ReturnsMinus5) {
  uint8_t d = 0;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, CONFIG, 12, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

/* ---- do_processing with data_length > 0 so data pointer is set ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_RequestWithData_DataPointerUsed) {
  uint8_t data = 0;  /* daymode off */
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, DAYMODE, 1, &data, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

/* ---- SET IMAGE IRCUTFILTER 0 (OFF path in L2) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_Ircutfilter_Off) {
  uint8_t data = 0;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, IRCUTFILTER, 1, &data, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

/* ---- SET SYSTEM FACTORY_RESET / CONFIG_RESET with valid 10-byte DOB ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_FactoryReset_ValidDob) {
  uint8_t dob[10] = { '0', '1', '-', '0', '1', '-', '2', '0', '0', '0' };
  uint8_t req[24];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, FACTORY_RESET, 10, dob, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_ConfigReset_ValidDob) {
  uint8_t dob[10] = { '0', '1', '-', '0', '1', '-', '2', '0', '0', '0' };
  uint8_t req[24];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, CONFIG_RESET, 10, dob, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  free(res);
}

/* ---- SET SYSTEM SET_LOGIN with valid format (1 + pin_len + 10 DOB) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_System_Login_ValidFormat) {
  /* 1 (pin_len) + 4 (pin "1234") + 10 (DOB "01-01-2000") = 15 bytes */
  uint8_t data[15] = { 4, '1', '2', '3', '4', '0', '1', '-', '0', '1', '-', '2', '0', '0', '0' };
  uint8_t req[32];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, SYSTEM, SET_LOGIN, sizeof(data), data, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  free(res);
}

/* ---- GET SYSTEM LOGIN (with pin data) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_System_Login) {
  uint8_t pin[4] = { '1', '2', '3', '4' };
  uint8_t req[16];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, SYSTEM, LOGIN, 4, pin, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  free(res);
}

/* ---- SET IMAGE invalid video frequency (1 = F25, outside F50-F60 range) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_VideoFrequency_Invalid_ReturnsError) {
  uint8_t vf = 1;  /* F25 invalid for set_image_video_frequency_l2 range check */
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, VIDEO_FREQUENCY, 1, &vf, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
}

/* ========== Mock-controlled tests for higher coverage ========== */

/* ---- init_motocam_configs with daymode 0 (set_day_mode OFF) ---- */
TEST_F(MotocamApiLibsTest, InitMotocamConfigs_DaymodeOff_SetsDayModeOff) {
  set_mock_day_mode(0);
  EXPECT_EQ(init_motocam_configs(), 0);
}

/* ---- init_motocam_configs with each misc value (set_misc_actions_on_boot branches 1..12) ---- */
TEST_F(MotocamApiLibsTest, InitMotocamConfigs_AllMiscBranches) {
  for (uint8_t misc = 1; misc <= 12; misc++) {
    set_mock_image_misc(misc);
    EXPECT_EQ(init_motocam_configs(), 0) << "misc=" << (int)misc;
  }
}

/* ---- SET IMAGE ZOOM when fw returns -1 (L1 error path) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_ImageZoom_WhenFwFails_ReturnsError) {
  set_mock_set_image_zoom_fail(1);
  uint8_t d = 1;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, ZOOM, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
  set_mock_set_image_zoom_fail(0);
}

/* ---- GET STREAM_STATE when stream is stopped (L1 error path) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_StreamState_WhenStopped_ReturnsFailed) {
  set_mock_stream_state(0);
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, STREAMING, STREAM_STATE, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  free(res);
  set_mock_stream_state(1);
}

/* ---- GET WEBRTC_STREAMING_STATUS when fw fails ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_WebRTCStatus_WhenFwFails_ReturnsFailed) {
  set_mock_get_webrtc_fail(1);
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, STREAMING, WEBRTC_STREAMING_STATUS, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  free(res);
  set_mock_get_webrtc_fail(0);
}

/* ---- GET IMAGE IRCUTFILTER when get_ir_cutfilter fails ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_ImageIrcutfilter_WhenFwFails_ReturnsFailed) {
  set_mock_get_ir_cutfilter_fail(1);
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, IMAGE, IRCUTFILTER, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  free(res);
  set_mock_get_ir_cutfilter_fail(0);
}

/* ---- GET IMAGE GYROREADER when fw fails (get_gyroreader_l2 uses get_day_mode) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_ImageGyroreader_WhenFwFails_ReturnsFailed) {
  set_mock_get_day_mode_fail(1);  /* get_gyroreader_l2 calls get_day_mode */
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, IMAGE, GYROREADER, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
  set_mock_get_day_mode_fail(0);
}

/* ---- GET IMAGE RESOLUTION when fw fails ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_ImageResolution_WhenFwFails_ReturnsFailed) {
  set_mock_get_image_resolution_fail(1);
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, IMAGE, RESOLUTION, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  free(res);
  set_mock_get_image_resolution_fail(0);
}

/* ---- SET NETWORK WifiHotspot with valid payload (L2 parsing) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_WifiHotspot_ValidPayload) {
  /* [ssid_len, ssid, enc_type, enc_key_len, enc_key, ip_len, ip, subnet_len, subnet] */
  uint8_t payload[40];
  size_t i = 0;
  payload[i++] = 4;
  payload[i++] = 'T'; payload[i++] = 'e'; payload[i++] = 's'; payload[i++] = 't';
  payload[i++] = 1;   /* encryption type */
  payload[i++] = 8;
  for (int j = 0; j < 8; j++) payload[i++] = '1' + (j % 10);
  payload[i++] = 9;
  memcpy(payload + i, "192.168.1.1", 9); i += 9;
  payload[i++] = 9;
  memcpy(payload + i, "255.255.0.0", 9); i += 9;
  uint8_t req[64];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, WifiHotspot, (uint8_t)i, payload, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

/* ---- SET NETWORK ETHERNET (set_ethernet_ip_address_l2) with valid payload ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_EthernetIp_ValidPayload) {
  /* [ip_len, ip..., subnet_len, subnet...] */
  uint8_t payload[32];
  size_t i = 0;
  payload[i++] = 11;
  memcpy(payload + i, "192.168.1.1", 11); i += 11;
  payload[i++] = 11;
  memcpy(payload + i, "255.255.0.0", 11); i += 11;
  uint8_t req[48];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, ETHERNET, (uint8_t)i, payload, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

/* ---- SET NETWORK WifiClient with DHCP (ipaddress_len=0) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_WifiClient_DhcpPayload) {
  /* [ssid_len, ssid, enc_type, enc_key_len, enc_key, ipaddress_len=0]; need len >= 17 */
  uint8_t payload[24];
  size_t i = 0;
  payload[i++] = 4;
  payload[i++] = 'T'; payload[i++] = 'e'; payload[i++] = 's'; payload[i++] = 't';
  payload[i++] = 1;
  payload[i++] = 8;
  for (int j = 0; j < 8; j++) payload[i++] = '1' + (j % 10);
  payload[i++] = 0;  /* ipaddress_len = 0 => DHCP */
  while (i < 17u) payload[i++] = 0;  /* L2 needs len >= 17 */
  uint8_t req[40];
  size_t len = 0;
  set_mock_wifi_state(2);  /* Client mode so L2 returns 0 */
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, WifiClient, (uint8_t)i, payload, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
  set_mock_wifi_state(0);
}

/* ---- GET CONFIG when get_config_current_l2 fails (initialize_config length < 14) ---- */
/* (Covered indirectly via init - get_config_current_l2 always returns 14 bytes in mock) */

/* ---- set_misc_actions_on_boot default/invalid branch ---- */
TEST_F(MotocamApiLibsTest, InitMotocamConfigs_MiscDefault_InvalidBranch) {
  set_mock_image_misc(99);  /* invalid misc, hits default in switch */
  EXPECT_EQ(init_motocam_configs(), 0);
}

/* ---- SET IMAGE MISC when daymode 0 (manual) - success ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_Misc_WhenDaymodeManual_Success) {
  set_mock_day_mode(0);
  uint8_t misc = 1;
  uint8_t req[12];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, MISC, 1, &misc, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

/* ---- SET IMAGE MISC when fw set_image_misc fails -> -2 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_Misc_WhenFwFails_ReturnsMinus2) {
  set_mock_day_mode(0);
  set_mock_set_image_misc_fail(1);
  uint8_t misc = 1;
  uint8_t req[12];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, MISC, 1, &misc, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -2);
  free(res);
  set_mock_set_image_misc_fail(0);
}

/* ---- SET IMAGE IRCUTFILTER invalid value (not 0 or 1) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_Ircutfilter_InvalidValue_ReturnsError) {
  uint8_t v = 2;
  uint8_t req[10];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, IRCUTFILTER, 1, &v, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
}

/* ---- SET NETWORK invalid data length (too short) ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_WifiHotspot_InvalidLength_ReturnsMinus5) {
  uint8_t payload[8] = { 1, 2, 3, 4, 5 };
  uint8_t req[20];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, WifiHotspot, 5, payload, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_WifiClient_InvalidLength_ReturnsMinus5) {
  uint8_t payload[8] = { 1, 2, 3, 4, 5 };
  uint8_t req[20];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, WifiClient, 5, payload, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_Ethernet_InvalidLength_ReturnsMinus5) {
  uint8_t payload[8] = { 1, 2, 3, 4, 5 };
  uint8_t req[20];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, ETHERNET, 5, payload, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

/* ---- SET STREAMING invalid data length ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Streaming_InvalidLength_ReturnsMinus5) {
  uint8_t d = 0;
  uint8_t req[10];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, STREAMING, START_STREAMING, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

/* ---- SET STREAMING START/STOP: L2 does not propagate fw return, so no failure test. ---- */

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Streaming_StartWebRtc_WhenFwFails_ReturnsError) {
  set_mock_start_webrtc_stream_fail(1);
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, STREAMING, START_WEBRTC_STREAMING, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
  set_mock_start_webrtc_stream_fail(0);
}

TEST_F(MotocamApiLibsTest, DoProcessing_SET_Streaming_StopWebRtc_WhenFwFails_ReturnsError) {
  set_mock_stop_webrtc_stream_fail(1);
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, STREAMING, STOP_WEBRTC_STREAMING, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
  set_mock_stop_webrtc_stream_fail(0);
}

/* ---- SET NETWORK WifiClient DHCP when wifi state is 1 (Hotspot) -> -2 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_WifiClient_Dhcp_WhenHotspot_ReturnsMinus2) {
  uint8_t payload[24];
  size_t i = 0;
  payload[i++] = 4;
  payload[i++] = 'T'; payload[i++] = 'e'; payload[i++] = 's'; payload[i++] = 't';
  payload[i++] = 1;
  payload[i++] = 8;
  for (int j = 0; j < 8; j++) payload[i++] = '1' + (j % 10);
  payload[i++] = 0;
  while (i < 17u) payload[i++] = 0;
  set_mock_wifi_state(1);  /* Hotspot: L2 returns -2 */
  uint8_t req[40];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, WifiClient, (uint8_t)i, payload, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -2);
  free(res);
  set_mock_wifi_state(0);
}

/* ========== Additional tests for line coverage ========== */

/* ---- SET IMAGE VIDEO_FREQUENCY F60 (value 4) success ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_VideoFrequency_F60_Success) {
  uint8_t vf = 4;  /* F60 */
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, VIDEO_FREQUENCY, 1, &vf, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

/* ---- SET IMAGE MISC invalid data length (0) -> -5 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_Misc_InvalidDataLength_ReturnsMinus5) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, MISC, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

/* ---- GET CONFIG StreamingConfig (sub 10) success ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_Config_StreamingConfig_Success) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, CONFIG, 10 /* StreamingConfig */, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[0], (uint8_t)RESPONSE);
  EXPECT_EQ(res[4], 0u);
  free(res);
}

/* ---- SET STREAMING START/STOP: L2 does not propagate start_stream/stop_stream
       return value, so no failure test; success covered by AllSubCommands. ---- */

/* ---- SET IMAGE DAYMODE invalid value (2) -> -1 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_Daymode_InvalidValue_ReturnsError) {
  uint8_t d = 2;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, DAYMODE, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
}

/* ---- SET IMAGE GYROREADER invalid value (2) -> -1 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Image_Gyroreader_InvalidValue_ReturnsError) {
  uint8_t d = 2;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, IMAGE, GYROREADER, 1, &d, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  EXPECT_EQ((int8_t)res[5], -1);
  free(res);
}

/* ---- SET NETWORK Onvif when current state equals new -> -2 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_Onvif_SameState_ReturnsMinus2) {
  /* Mock get_onvif_interface_state returns 0 (ETH). Sending 0 means no change, L2 returns -2 */
  uint8_t iface = 0;
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, Onvif, 1, &iface, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -2);
  free(res);
}

/* ---- GET IMAGE DAYMODE when fw get_day_mode fails ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_Image_Daymode_WhenFwFails_ReturnsFailed) {
  set_mock_get_day_mode_fail(1);
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, IMAGE, DAYMODE, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(res[4], 1u);
  free(res);
  set_mock_get_day_mode_fail(0);
}

/* ---- SET NETWORK WifiCountryCode invalid (code_len 0) -> L2 returns -5 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_WifiCountryCode_InvalidCodeLen_ReturnsError) {
  uint8_t data[3] = { 0, 'U', 'S' };  /* code_len=0 is invalid */
  uint8_t req[12];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, WifiCountryCode, 3, data, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

/* ---- SET NETWORK WifiCountryCode invalid (code_len != 2) -> L2 returns -5 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_SET_Network_WifiCountryCode_InvalidCodeNotTwo_ReturnsMinus5) {
  uint8_t data[2] = { 1, 'x' };  /* code_len=1, L2 expects 2-char ISO code */
  uint8_t req[12];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), SET, NETWORK, WifiCountryCode, 2, data, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -5);
  free(res);
}

/* ---- GET STREAMING invalid sub command -> -4 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_Streaming_InvalidSub_ReturnsMinus4) {
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, STREAMING, 99, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -4);
  free(res);
}

/* ---- GET IMAGE invalid sub command (not in GET list) -> -4 ---- */
TEST_F(MotocamApiLibsTest, DoProcessing_GET_Image_MiscSubCommand_Invalid_ReturnsMinus4) {
  /* MISC is SET-only; GET IMAGE with sub MISC hits default in get_image_command */
  uint8_t req[8];
  size_t len = 0;
  build_valid_crc_packet(req, sizeof(req), GET, IMAGE, MISC, 0, nullptr, &len);
  uint8_t *res = nullptr;
  uint8_t res_size = 0;
  do_processing(req, (uint8_t)len, &res, &res_size);
  ASSERT_NE(res, nullptr);
  EXPECT_EQ((int8_t)res[5], -4);
  free(res);
}

TEST_F(MotocamApiLibsTest, DirectL1_AudioAndConfig_ErrorPaths) {
  uint8_t d = 1;
  uint8_t *out = nullptr;
  uint8_t out_size = 0;

  EXPECT_EQ(set_audio_command(99, 0, nullptr), -4);
  EXPECT_EQ(set_audio_command(MIC, 1, nullptr), 0); /* executes NULL-data branch */
  EXPECT_EQ(get_audio_mic_l1(1, &d, &out, &out_size), -5);

  EXPECT_EQ(set_config_command(99, 0, nullptr), -4);
  EXPECT_EQ(set_config_currenttofactory(1, &d), -5);
  EXPECT_EQ(set_config_currenttodefault(1, &d), -5);

  EXPECT_EQ(get_config_factory(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_config_default(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_config_current(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_config_streaming_config(1, &d, &out, &out_size), -5);
}

TEST_F(MotocamApiLibsTest, DirectL1_StreamingAndNetwork_InvalidLengthAndInvalidSub) {
  uint8_t d = 0;
  uint8_t *out = nullptr;
  uint8_t out_size = 0;

  EXPECT_EQ(set_streaming_command(99, 0, nullptr), -4);
  EXPECT_EQ(start_webrtc_streaming_l1(1, &d), -5);
  EXPECT_EQ(stop_webrtc_streaming_l1(1, &d), -5);
  EXPECT_EQ(set_streaming_stop(1, &d), -5);
  EXPECT_EQ(get_webrtc_streaming_state_l1(1, &d, &out, &out_size), -5);

  EXPECT_EQ(set_network_command(99, 0, nullptr), -4);
  EXPECT_EQ(set_wifi_country_code_l1(4, &d), -5);
  EXPECT_EQ(get_network_WifiHotspot(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_network_WifiClient(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_wifi_country_code_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_wifi_state_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_ethernet_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_onvif_interface_l1(1, &d, &out, &out_size), -5);
}

TEST_F(MotocamApiLibsTest, DirectL1_Image_InvalidLengthsAndValues) {
  uint8_t d = 1;
  uint8_t bad = 2;
  uint8_t *out = nullptr;
  uint8_t out_size = 0;

  EXPECT_EQ(set_image_rotation_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_irbrightness_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_mid_irbrightness_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_side_irbrightness_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_daymode_l1(1, &bad), -1);
  EXPECT_EQ(set_image_gyroreader_l1(1, &bad), -1);
  EXPECT_EQ(set_image_resolution_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_mirror_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_flip_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_tilt_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_wdr_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_eis_l1(0, nullptr), -5);
  EXPECT_EQ(set_image_video_frequency_l1(0, nullptr), -5);

  EXPECT_EQ(get_image_zoom_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_rotation_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_ircutfilter_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_irbrightness_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_daymode_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_gyroreader_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_resolution_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_mirror_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_flip_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_tilt_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_wdr_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_eis_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_image_video_frequency_l1(1, &d, &out, &out_size), -5);
}

TEST_F(MotocamApiLibsTest, DirectL1_System_ErrorPaths) {
  uint8_t d = 1;
  uint8_t *out = nullptr;
  uint8_t out_size = 0;
  uint8_t bad_login_len_data[12] = { 0 };
  uint8_t mismatch_login_data[15] = { 4, '1', '2', '3', '4', '0', '1', '-', '0', '1', '-', '2', '0', '0', '0' };
  mismatch_login_data[0] = 3; /* expected length now 14, actual 15 */
  uint8_t bad_dob_login_data[15] = { 4, '1', '2', '3', '4', '3', '1', '-', '1', '3', '-', '2', '0', '0', '0' };
  uint8_t invalid_dob[10] = { '3', '1', '-', '1', '3', '-', '2', '0', '0', '0' };

  EXPECT_EQ(set_system_command(99, 0, nullptr), -4);
  EXPECT_EQ(set_system_shutdown_l1(1, &d), -5);
  EXPECT_EQ(set_ota_update_l1(1, &d), -5);
  EXPECT_EQ(set_system_camera_name_l1(33, nullptr), -5);
  EXPECT_EQ(set_system_login_l1(11, bad_login_len_data), -5);
  EXPECT_EQ(set_system_login_l1(12, bad_login_len_data), -5);
  EXPECT_EQ(set_system_login_l1(sizeof(mismatch_login_data), mismatch_login_data), -5);
  EXPECT_EQ(set_system_login_l1(15, bad_dob_login_data), -2);
  EXPECT_EQ(set_system_factory_reset_l1(10, invalid_dob), -2);
  EXPECT_EQ(set_system_config_reset_l1(10, invalid_dob), -2);
  EXPECT_EQ(set_system_time_l1(0, nullptr), -5);
  EXPECT_EQ(set_system_haptic_motor_l1(1, &d), -5);
  EXPECT_EQ(set_system_user_dob_l1(9, invalid_dob), -5);

  EXPECT_EQ(get_system_command(99, 0, nullptr, &out, &out_size), -4);
  EXPECT_EQ(get_system_camera_name_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_system_firmware_version_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_system_mac_address_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_ota_update_status_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_camera_health_l1(1, &d, &out, &out_size), -5);
  EXPECT_EQ(get_system_user_dob_l1(1, &d, &out, &out_size), -5);

  EXPECT_EQ(login_with_pin_l1(3, (uint8_t *)"123", &out, &out_size), -1);
  EXPECT_EQ(login_with_pin_l1(4, (uint8_t *)"12a4", &out, &out_size), -2);
  EXPECT_EQ(login_with_pin_l1(4, (uint8_t *)"1234", &out, &out_size), -3);
  if (out)
    free(out);
}

TEST_F(MotocamApiLibsTest, DirectL2_Network_ParsingAndValidationPaths) {
  /* Hotspot parsing validation branches */
  uint8_t hs1[] = { 4, 'T', 'E' };
  EXPECT_EQ(set_WifiHotspot_l2(sizeof(hs1), hs1), -1);
  uint8_t hs2[] = { 1, 'T', 1 };
  EXPECT_EQ(set_WifiHotspot_l2(sizeof(hs2), hs2), -1);
  uint8_t hs3[] = { 1, 'T', 1, 8, '1', '2' };
  EXPECT_EQ(set_WifiHotspot_l2(sizeof(hs3), hs3), -1);
  uint8_t hs4[] = { 1, 'T', 1, 1, 'k', 9, '1', '2' };
  EXPECT_EQ(set_WifiHotspot_l2(sizeof(hs4), hs4), -1);
  uint8_t hs5[] = { 1, 'T', 1, 1, 'k', 1, '1', 9, '2', '5' };
  EXPECT_EQ(set_WifiHotspot_l2(sizeof(hs5), hs5), -1);
  uint8_t hs_with_newline[] = {
    2, 'A', '\n', 1, 1, 'k', 1, '1', 1, '2'
  };
  EXPECT_EQ(set_WifiHotspot_l2(sizeof(hs_with_newline), hs_with_newline), 0);

  /* Ethernet parser validation */
  uint8_t eth1[] = { 11, '1', '9', '2' };
  EXPECT_EQ(set_ethernet_ip_address_l2(sizeof(eth1), eth1), -1);
  uint8_t eth2[] = { 1, '1', 9, '2' };
  EXPECT_EQ(set_ethernet_ip_address_l2(sizeof(eth2), eth2), -1);

  /* WifiClient parser validation and branch coverage */
  uint8_t wc1[] = { 4, 'T', 'E' };
  EXPECT_EQ(set_WifiClient_l2(sizeof(wc1), wc1), -1);
  uint8_t wc2[] = { 1, 'T', 1 };
  EXPECT_EQ(set_WifiClient_l2(sizeof(wc2), wc2), -1);
  uint8_t wc3[] = { 1, 'T', 1, 8, '1', '2' };
  EXPECT_EQ(set_WifiClient_l2(sizeof(wc3), wc3), -1);
  uint8_t wc_dhcp[] = { 1, 'T', 1, 1, 'k', 0, 0 };
  set_mock_wifi_state(0);
  EXPECT_EQ(set_WifiClient_l2(sizeof(wc_dhcp), wc_dhcp), 0);
  set_mock_wifi_state(1);
  EXPECT_EQ(set_WifiClient_l2(sizeof(wc_dhcp), wc_dhcp), -2);
  set_mock_wifi_state(2);
  EXPECT_EQ(set_WifiClient_l2(sizeof(wc_dhcp), wc_dhcp), 0);
  set_mock_wifi_state(0);
  uint8_t wc_static_bad_ip[] = { 1, 'T', 1, 1, 'k', 3, '1', '2' };
  EXPECT_EQ(set_WifiClient_l2(sizeof(wc_static_bad_ip), wc_static_bad_ip), -1);
  uint8_t wc_static_bad_subnet[] = { 1, 'T', 1, 1, 'k', 1, '1', 3, '2' };
  EXPECT_EQ(set_WifiClient_l2(sizeof(wc_static_bad_subnet), wc_static_bad_subnet), -1);

  /* get_* null parameter validation */
  uint8_t *out = nullptr;
  uint8_t out_size = 0;
  EXPECT_EQ(get_wifi_country_code_l2(nullptr, &out_size), -1);
  EXPECT_EQ(get_wifi_country_code_l2(&out, nullptr), -1);

  /* Country code setter validation */
  EXPECT_EQ(set_wifi_country_code_l2(0, nullptr), -1);
  uint8_t code0[] = { 0, 'U', 'S' };
  EXPECT_EQ(set_wifi_country_code_l2(sizeof(code0), code0), -5);
  uint8_t code_short[] = { 2, 'U' };
  EXPECT_EQ(set_wifi_country_code_l2(sizeof(code_short), code_short), -1);
  uint8_t code_bad_len[] = { 1, 'U' };
  EXPECT_EQ(set_wifi_country_code_l2(sizeof(code_bad_len), code_bad_len), -5);
}

TEST_F(MotocamApiLibsTest, DirectL2_System_ValidationPaths) {
  uint8_t valid_dob[10] = { '0', '1', '-', '0', '1', '-', '2', '0', '2', '4' };
  uint8_t invalid_sep[10] = { '0', '1', '/', '0', '1', '/', '2', '0', '2', '4' };
  uint8_t invalid_month[10] = { '0', '1', '-', '1', '3', '-', '2', '0', '2', '4' };
  uint8_t invalid_day[10] = { '3', '2', '-', '0', '1', '-', '2', '0', '2', '4' };

  EXPECT_EQ(set_system_factory_reset_l2(9, valid_dob), -5);
  EXPECT_EQ(set_system_factory_reset_l2(10, invalid_sep), -1);
  EXPECT_EQ(set_system_factory_reset_l2(10, invalid_month), -2);
  EXPECT_EQ(set_system_factory_reset_l2(10, invalid_day), -2);
  EXPECT_EQ(set_system_factory_reset_l2(10, valid_dob), 0);

  EXPECT_EQ(set_system_config_reset_l2(9, valid_dob), -5);
  EXPECT_EQ(set_system_config_reset_l2(10, invalid_sep), -1);
  EXPECT_EQ(set_system_config_reset_l2(10, invalid_month), -2);
  EXPECT_EQ(set_system_config_reset_l2(10, invalid_day), -2);
  EXPECT_EQ(set_system_config_reset_l2(10, valid_dob), 0);

  EXPECT_EQ(set_system_camera_name_l2(33, valid_dob), -1);
  EXPECT_EQ(set_system_login_l2(33, valid_dob, 10, valid_dob), -1);
  EXPECT_EQ(set_system_login_l2(4, (uint8_t *)"1234", 9, valid_dob), -5);
  EXPECT_EQ(set_system_login_l2(4, (uint8_t *)"1234", 10, invalid_sep), -1);
  EXPECT_EQ(set_system_login_l2(4, (uint8_t *)"1234", 10, invalid_month), -2);
  EXPECT_EQ(set_system_login_l2(4, (uint8_t *)"1234", 10, valid_dob), 0);

  uint8_t *auth = nullptr;
  uint8_t auth_size = 0;
  EXPECT_EQ(login_with_pin_l2(3, (uint8_t *)"123", &auth, &auth_size), -1);
  EXPECT_EQ(login_with_pin_l2(4, (uint8_t *)"12a4", &auth, &auth_size), -2);
  EXPECT_EQ(login_with_pin_l2(4, (uint8_t *)"1234", &auth, &auth_size), -3);
  if (auth)
    free(auth);

  EXPECT_EQ(set_system_user_dob_l2(11, valid_dob), -1);
  EXPECT_EQ(set_system_user_dob_l2(10, invalid_sep), -1);
  EXPECT_EQ(set_system_user_dob_l2(10, invalid_month), -2);
  EXPECT_EQ(set_system_user_dob_l2(10, valid_dob), 0);

  for (int month = 1; month <= 12; month++) {
    uint8_t date[10] = { '0', '1', '-', '0', '1', '-', '2', '0', '2', '4' };
    date[3] = (uint8_t)('0' + (month / 10));
    date[4] = (uint8_t)('0' + (month % 10));
    EXPECT_EQ(set_system_user_dob_l2(10, date), 0);
  }

  EXPECT_EQ(set_system_time_l2(0, valid_dob), -1);
  uint8_t bad_epoch[] = { 10 };
  uint8_t good_epoch[] = { 1, 2, 3, 4 };
  EXPECT_EQ(set_system_time_l2(1, bad_epoch), -1);
  EXPECT_EQ(set_system_time_l2(4, good_epoch), 0);
}

TEST_F(MotocamApiLibsTest, DirectL1_Image_ErrorBranches_WhenFwFails) {
  uint8_t d = 1;
  uint8_t vf = 3;
  uint8_t *out = nullptr;
  uint8_t out_size = 0;

  set_mock_fail_image_set(1);
  EXPECT_EQ(set_image_rotation_l1(1, &d), -1);
  EXPECT_EQ(set_image_irbrightness_l1(1, &d), -1);
  EXPECT_EQ(set_image_mid_irbrightness_l1(1, &d), -1);
  EXPECT_EQ(set_image_side_irbrightness_l1(1, &d), -1);
  EXPECT_EQ(set_image_daymode_l1(1, &d), -1);
  EXPECT_EQ(set_image_gyroreader_l1(1, &d), -1);
  EXPECT_EQ(set_image_resolution_l1(1, &d), -1);
  EXPECT_EQ(set_image_mirror_l1(1, &d), -1);
  EXPECT_EQ(set_image_flip_l1(1, &d), -1);
  EXPECT_EQ(set_image_tilt_l1(1, &d), -1);
  EXPECT_EQ(set_image_wdr_l1(1, &d), -1);
  EXPECT_EQ(set_image_eis_l1(1, &d), -1);
  EXPECT_EQ(set_image_video_frequency_l1(1, &vf), 0); /* does not rely on fw mock */
  set_mock_fail_image_set(0);

  set_mock_fail_image_get(1);
  EXPECT_EQ(get_image_zoom_l1(0, nullptr, &out, &out_size), 0);
  EXPECT_EQ(get_image_ircutfilter_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_image_irbrightness_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_image_resolution_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_image_mirror_l1(0, nullptr, &out, &out_size), 0);
  EXPECT_EQ(get_image_flip_l1(0, nullptr, &out, &out_size), 0);
  EXPECT_EQ(get_image_tilt_l1(0, nullptr, &out, &out_size), 0);
  EXPECT_EQ(get_image_wdr_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_image_eis_l1(0, nullptr, &out, &out_size), -1);
  set_mock_fail_image_get(0);

  set_mock_get_day_mode_fail(1);
  EXPECT_EQ(get_image_daymode_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_image_gyroreader_l1(0, nullptr, &out, &out_size), -1);
  set_mock_get_day_mode_fail(0);
}

TEST_F(MotocamApiLibsTest, DirectL1_NetworkAndSystem_ErrorBranches_WhenFwFails) {
  uint8_t *out = nullptr;
  uint8_t out_size = 0;
  uint8_t name[] = "Cam";
  uint8_t epoch[] = { 1, 2, 3, 4 };
  uint8_t dob[10] = { '0', '1', '-', '0', '1', '-', '2', '0', '2', '4' };
  uint8_t iface = 1;
  uint8_t country[3] = { 2, 'U', 'S' };
  uint8_t hs[13] = { 1, 'T', 1, 1, 'k', 1, '1', 1, '2', 0, 0, 0, 0 };
  uint8_t wc[13] = { 1, 'T', 1, 1, 'k', 0, 0, 0, 0, 0, 0, 0, 0 };
  uint8_t eth[13] = { 1, '1', 1, '2', 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  uint8_t prov_small[3] = { 0, 0, 0 };

  set_mock_fail_network_fw(1);
  EXPECT_EQ(set_network_WifiHotspot(sizeof(hs), hs), -1);
  EXPECT_EQ(set_network_WifiClient(sizeof(wc), wc), -1);
  EXPECT_EQ(set_ethernet_ip_address_l1(sizeof(eth), eth), -1);
  EXPECT_EQ(set_onvif_interface_l1(1, &iface), -1);
  EXPECT_EQ(set_wifi_country_code_l1(3, country), -1);
  EXPECT_EQ(get_network_WifiHotspot(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_network_WifiClient(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_wifi_state_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_ethernet_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_onvif_interface_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_wifi_country_code_l1(0, nullptr, &out, &out_size), -1);
  set_mock_fail_network_fw(0);

  set_mock_fail_system_fw(1);
  EXPECT_EQ(set_system_shutdown_l1(0, nullptr), -1);
  EXPECT_EQ(set_ota_update_l1(0, nullptr), -1);
  EXPECT_EQ(provision_device_l1(sizeof(prov_small), prov_small), -1);
  EXPECT_EQ(set_system_camera_name_l1(3, name), -1);
  EXPECT_EQ(set_system_time_l1(sizeof(epoch), epoch), -1);
  EXPECT_EQ(set_system_haptic_motor_l1(0, nullptr), -1);
  EXPECT_EQ(set_system_user_dob_l1(10, dob), -1);
  EXPECT_EQ(get_system_camera_name_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_system_firmware_version_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_system_mac_address_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_ota_update_status_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_camera_health_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(get_system_user_dob_l1(0, nullptr, &out, &out_size), -1);
  EXPECT_EQ(login_with_pin_l1(4, (uint8_t *)"1234", &out, &out_size), -1);
  set_mock_fail_system_fw(0);
}

TEST_F(MotocamApiLibsTest, DirectL1_System_SpecificNegativeReturnPropagation) {
  uint8_t dob[10] = { '0', '1', '-', '0', '1', '-', '2', '0', '2', '4' };
  uint8_t login_data[15] = {
    4, '1', '2', '3', '4', '0', '1', '-', '0', '1', '-', '2', '0', '2', '4'
  };

  set_mock_set_factory_reset_ret(-6);
  EXPECT_EQ(set_system_factory_reset_l1(10, dob), -6);
  set_mock_set_factory_reset_ret(-7);
  EXPECT_EQ(set_system_factory_reset_l1(10, dob), -7);
  set_mock_set_factory_reset_ret(0);

  set_mock_set_config_reset_ret(-6);
  EXPECT_EQ(set_system_config_reset_l1(10, dob), -6);
  set_mock_set_config_reset_ret(-7);
  EXPECT_EQ(set_system_config_reset_l1(10, dob), -7);
  set_mock_set_config_reset_ret(0);

  set_mock_set_login_ret(-6);
  EXPECT_EQ(set_system_login_l1(sizeof(login_data), login_data), -6);
  set_mock_set_login_ret(-7);
  EXPECT_EQ(set_system_login_l1(sizeof(login_data), login_data), -7);
  set_mock_set_login_ret(0);
}

TEST_F(MotocamApiLibsTest, DirectL2_Network_And_System_FwFailureBranches) {
  uint8_t *out = nullptr;
  uint8_t out_size = 0;
  uint8_t iface = 1;
  uint8_t country[3] = { 2, 'U', 'S' };
  uint8_t hs_valid[] = { 1, 'T', 1, 1, 'k', 1, '1', 1, '2' };
  uint8_t wc_valid[] = { 1, 'T', 1, 1, 'k', 1, '1', 1, '2' };
  uint8_t eth_valid[] = { 1, '1', 1, '2' };
  uint8_t name[] = "Cam";
  uint8_t pin[] = "1234";
  uint8_t dob[10] = { '0', '1', '-', '0', '1', '-', '2', '0', '2', '4' };
  uint8_t epoch[] = { 1, 2, 3, 4 };
  uint8_t prov[12] = {
    1, 'A', 1, 'B', 10, '0', '1', '-', '0', '1', '-', '2'
  };

  set_mock_network_config_nonempty(1);
  get_WifiHotspot_l2(&out, &out_size);
  if (out) free(out);
  out = nullptr;
  get_WifiClient_l2(&out, &out_size);
  if (out) free(out);
  out = nullptr;

  set_mock_fail_network_fw(1);
  EXPECT_EQ(set_WifiHotspot_l2(sizeof(hs_valid), hs_valid), -1);
  EXPECT_EQ(set_ethernet_ip_address_l2(sizeof(eth_valid), eth_valid), -1);
  EXPECT_EQ(set_onvif_interface_l2(1, &iface), (uint8_t)255);
  EXPECT_EQ(get_WifiHotspot_l2(&out, &out_size), -1);
  EXPECT_EQ(get_WifiClient_l2(&out, &out_size), -1);
  EXPECT_EQ(get_wifi_state_l2(&out, &out_size), -1);
  EXPECT_EQ(get_ethernet_l2(&out, &out_size), -1);
  EXPECT_EQ(get_onvif_interface_state_l2(&out, &out_size), -1);
  EXPECT_EQ(set_wifi_country_code_l2(3, country), -1);
  EXPECT_EQ(get_wifi_country_code_l2(&out, &out_size), -1);
  set_mock_fail_network_fw(0);

  set_mock_wifi_country_code_mode(1);
  EXPECT_EQ(get_wifi_country_code_l2(&out, &out_size), -1);
  set_mock_wifi_country_code_mode(2);
  EXPECT_EQ(get_wifi_country_code_l2(&out, &out_size), -1);
  set_mock_wifi_country_code_mode(0);
  set_mock_network_config_nonempty(0);

  set_mock_fail_system_fw(1);
  EXPECT_EQ(set_system_shutdown_l2(), -1);
  EXPECT_EQ(set_ota_update_l2(), -1);
  EXPECT_EQ(provision_device_l2(sizeof(prov), prov), -1);
  EXPECT_EQ(set_system_camera_name_l2(3, name), -1);
  EXPECT_EQ(set_system_login_l2(4, pin, 10, dob), -1);
  EXPECT_EQ(get_system_camera_name_l2(&out, &out_size), -1);
  EXPECT_EQ(get_system_firmware_version_l2(&out, &out_size), -1);
  EXPECT_EQ(get_system_mac_address_l2(&out, &out_size), -1);
  EXPECT_EQ(get_ota_update_status_l2(&out, &out_size), -1);
  EXPECT_EQ(get_camera_health_l2(&out, &out_size), -1);
  EXPECT_EQ(login_with_pin_l2(4, pin, &out, &out_size), -1);
  EXPECT_EQ(set_system_user_dob_l2(10, dob), -1);
  EXPECT_EQ(get_system_user_dob_l2(&out, &out_size), -1);
  EXPECT_EQ(set_system_time_l2(sizeof(epoch), epoch), -1);
  EXPECT_EQ(set_system_haptic_motor_l2(), -1);
  set_mock_fail_system_fw(0);

  set_mock_login_pin_mode(1);
  EXPECT_EQ(login_with_pin_l2(4, pin, &out, &out_size), 0);
  if (out) free(out);
  out = nullptr;
  set_mock_login_pin_mode(0);
}
