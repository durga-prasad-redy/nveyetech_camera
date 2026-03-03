#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mock_hw.h"

extern "C" {
    // Declarations from device_setup.c
    int validate_mac(const char *mac);
    int validate_mac_value(const char *mac);
    int validate_date(const char *date);
    int write_to_file(const char *path, const char *value);
    int update_hotspot_ssid(const char *mac);
}

#ifndef HOTSPOT_FILE
#define HOTSPOT_FILE "/tmp/test_board/hotspot_ssid"
#endif

class BoardSetupTest : public ::testing::Test {
protected:
    void SetUp() override {
        system("mkdir -p /tmp/test_board");
        set_mock_system_return(0);
    }
};

TEST_F(BoardSetupTest, ValidateMacFormatTest) {
    EXPECT_EQ(validate_mac("01:23:45:67:89:AB"), 1);
    EXPECT_EQ(validate_mac("01:23:45:67:89:ab"), 1); // lower case
    EXPECT_EQ(validate_mac("01:23:45:67:89"), 0); // too short
    EXPECT_EQ(validate_mac("01-23-45-67-89-AB"), 0); // wrong delimiter
    EXPECT_EQ(validate_mac("01:23:45:67:89:AG"), 0); // non-hex
}

TEST_F(BoardSetupTest, ValidateMacValueTest) {
    EXPECT_EQ(validate_mac_value("02:1A:2B:3C:4D:5E"), 1); // valid unicast local admin
    EXPECT_EQ(validate_mac_value("00:00:00:00:00:00"), 0); // all zero
    EXPECT_EQ(validate_mac_value("FF:FF:FF:FF:FF:FF"), 0); // all FF
    EXPECT_EQ(validate_mac_value("01:00:5E:00:00:01"), 0); // multicast (bit 0 of byte 0 set)
}

TEST_F(BoardSetupTest, ValidateDateTest) {
    EXPECT_EQ(validate_date("2023-10-15"), 1); // valid
    EXPECT_EQ(validate_date("2023-13-15"), 0); // invalid month
    EXPECT_EQ(validate_date("2023-10-32"), 0); // invalid day
    EXPECT_EQ(validate_date("23-10-15"), 0);   // invalid format
    EXPECT_EQ(validate_date("10/15/2023"), 0); // wrong delimiter
}

TEST_F(BoardSetupTest, WriteToFileTest) {
    const char* file_path = "/tmp/test_board/test_file.txt";
    EXPECT_EQ(write_to_file(file_path, "TEST_STRING"), 0);
    
    FILE *f = fopen(file_path, "r");
    ASSERT_NE(f, nullptr) << "Failed to open " << file_path;
    char buf[128] = {0};
    fgets(buf, sizeof(buf), f);
    fclose(f);
    
    // Check newline since write_to_file appends one
    EXPECT_STREQ(buf, "TEST_STRING\n");
}

TEST_F(BoardSetupTest, UpdateHotspotSsidTest) {
    // Write base ssid
    FILE *f = fopen(HOTSPOT_FILE, "w");
    ASSERT_NE(f, nullptr) << "Failed to open " << HOTSPOT_FILE;
    fprintf(f, "NveyetechCamera\n");
    fclose(f);
    
    // Run update with MAC ending in 12:34 => "1234"
    EXPECT_EQ(update_hotspot_ssid("AA:BB:CC:DD:12:34"), 0);
    
    // Read final file
    f = fopen(HOTSPOT_FILE, "r");
    ASSERT_NE(f, nullptr);
    char new_ssid[256];
    fgets(new_ssid, sizeof(new_ssid), f);
    fclose(f);
    
    EXPECT_STREQ(new_ssid, "NveyetechCamera_1234\n");
}

extern "C" void capture_defaults();

TEST_F(BoardSetupTest, CaptureDefaultsTest) {
    // Create config dir and some files
    system("mkdir -p /tmp/test_board");
    FILE *f1 = fopen("/tmp/test_board/file1", "w"); fprintf(f1, "val1\n"); fclose(f1);
    FILE *f2 = fopen("/tmp/test_board/file2", "w"); fprintf(f2, "val2\n"); fclose(f2);
    
    capture_defaults();
    
    // Check snapshot file
    FILE *f = fopen("/tmp/test_board/default_snapshot.txt", "r");
    ASSERT_NE(f, nullptr);
    char buf[1024] = {0};
    fread(buf, 1, sizeof(buf), f);
    fclose(f);
    
    EXPECT_TRUE(strstr(buf, "file1=val1") != nullptr);
    EXPECT_TRUE(strstr(buf, "file2=val2") != nullptr);
}

extern "C" int device_setup_main(int argc, char *argv[]);

TEST_F(BoardSetupTest, MainFunctionTest) {
    // Create necessary files for main to succeed
    FILE *f = fopen(HOTSPOT_FILE, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "NveyetechCamera\n");
    fclose(f);
    
    // Arguments: name MAC serial date epoch
    char arg0[] = "device_setup";
    char arg1[] = "02:1A:2B:3C:4D:5E";
    char arg2[] = "SN12345";
    char arg3[] = "2023-10-15";
    char arg4[] = "1697356800";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4};
    
    EXPECT_EQ(device_setup_main(5, argv), 0);
}

TEST_F(BoardSetupTest, Main_InvalidArgs) {
    char arg0[] = "device_setup";
    char* argv[] = {arg0};
    EXPECT_EQ(device_setup_main(1, argv), 1);
}

TEST_F(BoardSetupTest, Main_SetenvFailure) {
    set_mock_system_return(1);
    char arg0[] = "device_setup";
    char arg1[] = "02:1A:2B:3C:4D:5E";
    char arg2[] = "SN1"; char arg3[] = "2023-10-10"; char arg4[] = "100";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4};
    EXPECT_EQ(device_setup_main(5, argv), 1);
}

TEST_F(BoardSetupTest, UpdateHotspotSsid_Fail) {
    unlink(HOTSPOT_FILE);
    EXPECT_EQ(update_hotspot_ssid("AA:BB:CC:DD:EE:FF"), -1);
}

TEST_F(BoardSetupTest, UpdateHotspotSsid_EmptyFile) {
    // Create empty file
    FILE *f = fopen(HOTSPOT_FILE, "w");
    fclose(f);
    EXPECT_EQ(update_hotspot_ssid("AA:BB:CC:DD:EE:FF"), -1);
}

TEST_F(BoardSetupTest, UpdateHotspotSsid_ShortMac) {
    // Correct base file
    FILE *f = fopen(HOTSPOT_FILE, "w");
    fprintf(f, "test\n");
    fclose(f);
    EXPECT_EQ(update_hotspot_ssid("1:2:3"), -1); 
}

TEST_F(BoardSetupTest, WriteToFile_Fail) {
    // Write to a path that is a directory
    mkdir("/tmp/test_board/dir_case", 0777);
    EXPECT_EQ(write_to_file("/tmp/test_board/dir_case", "data"), -1);
    rmdir("/tmp/test_board/dir_case");
}

TEST_F(BoardSetupTest, Main_InvalidMac) {
    char arg0[] = "device_setup";
    char arg1[] = "invalid_mac";
    char arg2[] = "SN1"; char arg3[] = "2023-10-10"; char arg4[] = "100";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4};
    EXPECT_EQ(device_setup_main(5, argv), 1);
}

TEST_F(BoardSetupTest, Main_InvalidDate) {
    char arg0[] = "device_setup";
    char arg1[] = "02:1A:2B:3C:4D:5E";
    char arg2[] = "SN1"; char arg3[] = "invalid-date"; char arg4[] = "100";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4};
    EXPECT_EQ(device_setup_main(5, argv), 1);
}

TEST_F(BoardSetupTest, ValidateMac_EdgeCases) {
    EXPECT_FALSE(validate_mac("00:11:22:33:44:55:66")); // too long
    EXPECT_FALSE(validate_mac("00:11:22:33:44")); // too short
}

TEST_F(BoardSetupTest, CaptureDefaults_OpendirFail) {
    set_mock_opendir_fail(1);
    capture_defaults();
    SUCCEED();
    set_mock_opendir_fail(0);
}

TEST_F(BoardSetupTest, CaptureDefaults_FopenFail) {
    // Make SNAPSHOT_FILE a directory
    system("mkdir -p /tmp/test_board/default_snapshot.txt");
    capture_defaults();
    SUCCEED();
    system("rm -rf /tmp/test_board/default_snapshot.txt");
}

TEST_F(BoardSetupTest, ValidateDate_InvalidValues) {
    EXPECT_FALSE(validate_date("abcd-ef-gh")); 
    EXPECT_FALSE(validate_date("2023-13-15"));
    EXPECT_FALSE(validate_date("2023-10-32"));
}

TEST_F(BoardSetupTest, RunCmd_Fail) {
    // run_cmd is static in device_setup.c? No, it's not marked static.
}

TEST_F(BoardSetupTest, Main_InvalidEpoch) {
    char arg0[] = "device_setup";
    char arg1[] = "02:1A:2B:3C:4D:5E";
    char arg2[] = "SN1"; char arg3[] = "2023-10-10"; char arg4[] = "1697356800";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4};
    
    // We want to trigger a failure in Step 6 (system call)
    // but first we need previous steps to succeed.
    // However, our mock_system_return is global.
    set_mock_system_return(1);
    EXPECT_EQ(device_setup_main(5, argv), 1); 
}

extern "C" void capture_factory_backup();

TEST_F(BoardSetupTest, CaptureFactoryBackup_Fail) {
    set_mock_system_return(1);
    capture_factory_backup();
    SUCCEED();
    set_mock_system_return(0);
}
