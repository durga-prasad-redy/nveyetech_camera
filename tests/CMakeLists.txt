# Set up test builds with gcov coverage flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage -g -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage -g -O0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -lgcov")

# We mock dependencies by wrapping them
set(MOCK_LINK_FLAGS "-Wl,--wrap=system;-Wl,--wrap=popen;-Wl,--wrap=pclose;-Wl,--wrap=fcntl;-Wl,--wrap=pthread_create;-Wl,--wrap=rename;-Wl,--wrap=remove;-Wl,--wrap=symlink;-Wl,--wrap=opendir;-Wl,--undefined=system;-Wl,--undefined=popen;-Wl,--undefined=pclose;-Wl,--undefined=fcntl;-Wl,--undefined=pthread_create;-Wl,--undefined=rename;-Wl,--undefined=remove;-Wl,--undefined=symlink;-Wl,--undefined=opendir")

add_library(mock_hw mocks/mock_hw.c)
target_include_directories(mock_hw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mocks)

# --- 1. factory_reset ---
add_executable(test_factory_reset test_factory_reset.cpp ../factory_reset/factory_reset.c)
target_include_directories(test_factory_reset PRIVATE ../factory_reset)
target_compile_definitions(test_factory_reset PRIVATE main=factory_reset_main)
target_link_libraries(test_factory_reset gtest gtest_main mock_hw)
target_link_options(test_factory_reset PRIVATE ${MOCK_LINK_FLAGS})
target_compile_definitions(test_factory_reset PRIVATE
    CONFIG_DIR=\"/tmp/test_factory/m5s_config\"
    SNAPSHOT_FILE=\"/tmp/test_factory/default_snapshot.txt\"
    FACTORY_BACKUP_DIR=\"/tmp/test_factory/factory_backup\"
    FACTORY_RESET_STATUS_FILE=\"/tmp/test_factory/m5s_config/factory_reset_status\"
)
add_test(NAME test_factory_reset COMMAND test_factory_reset)

# --- 2. board_setup ---
add_executable(test_board_setup test_board_setup.cpp ../board_setup_implementation/camera_provisioning/device_setup.c)
target_include_directories(test_board_setup PRIVATE ../board_setup_implementation/camera_provisioning)
target_compile_definitions(test_board_setup PRIVATE main=device_setup_main)
target_link_libraries(test_board_setup gtest gtest_main mock_hw)
target_link_options(test_board_setup PRIVATE ${MOCK_LINK_FLAGS})
target_compile_definitions(test_board_setup PRIVATE
    SERIAL_FILE=\"/tmp/test_board/serial_number\"
    MFG_FILE=\"/tmp/test_board/mfg_date\"
    MAC_FILE=\"/tmp/test_board/ethaddr\"
    HOTSPOT_FILE=\"/tmp/test_board/hotspot_ssid\"
    CONFIG_DIR=\"/tmp/test_board\"
    SNAPSHOT_FILE=\"/tmp/test_board/default_snapshot.txt\"
)
add_test(NAME test_board_setup COMMAND test_board_setup)

# --- 3. m5s_mw_server ---
add_executable(test_m5s_mw_server test_m5s_mw_server.cpp ../m5s_mw_server/timer.c ../m5s_mw_server/motocam_api_server.c)
target_include_directories(test_m5s_mw_server PRIVATE ../m5s_mw_server ../motocam_fw_libs/include)
target_compile_definitions(test_m5s_mw_server PRIVATE main=m5s_mw_server_main)
target_link_libraries(test_m5s_mw_server gtest gtest_main mock_hw pthread)
target_link_options(test_m5s_mw_server PRIVATE ${MOCK_LINK_FLAGS})
target_compile_definitions(test_m5s_mw_server PRIVATE 
    CONFIG_PATH=\"/tmp/test_m5s/config\"
    GYRO_PATH=\"/tmp/test_m5s/gyro\"
    GAIN_FILE=\"/tmp/test_m5s/tmp/asc_ae_log.txt\"
    M5S_CONFIG_DIR=\"/tmp/test_m5s/m5s_config\"
    RES_PATH=\"/tmp/test_m5s\"
)
add_test(NAME test_m5s_mw_server COMMAND test_m5s_mw_server)

# --- 4. motocam_fw_libs ---
add_executable(test_motocam_fw_libs test_motocam_fw_libs.cpp 
    ../motocam_fw_libs/src/fw.c
    ../motocam_fw_libs/src/gpio.c
)
target_include_directories(test_motocam_fw_libs PRIVATE ../motocam_fw_libs/include)
target_link_libraries(test_motocam_fw_libs gtest gtest_main mock_hw pthread)
target_link_options(test_motocam_fw_libs PRIVATE ${MOCK_LINK_FLAGS})
target_compile_definitions(test_motocam_fw_libs PRIVATE
    GPIO_EXPORT_PATH=\"/tmp/test_fw/gpio/export\"
    GPIO_DIRECTION_PATH=\"/tmp/test_fw/gpio/gpio%d/direction\"
    GPIO_VALUE_PATH=\"/tmp/test_fw/gpio/gpio%d/value\"
    OTA_STATUS_FILE=\"/tmp/test_fw/ota_status\"
    WIFI_STATE_PATH=\"/tmp/test_fw/wifi_operstate\"
    IRCUT_STATE_FILE=\"/tmp/test_fw/ircut_filter\"
    WIFI_STATE_FILE=\"/tmp/test_fw/wifi_state\"
    LOCK_FILE=\"/tmp/test_fw/fw_lock\"
    PWM4_FILE=\"/tmp/test_fw/pwmdev-4\"
    PWM5_FILE=\"/tmp/test_fw/pwmdev-5\"
    PWM7_FILE=\"/tmp/test_fw/pwmdev-7\"
    M5S_CONFIG_DIR=\"/tmp/test_fw\"
    CONFIG_PATH=\"/tmp/test_fw/config\"
    PROC_PATH=\"/tmp/test_fw/proc\"
)
add_test(NAME test_motocam_fw_libs COMMAND test_motocam_fw_libs)

# --- 5. motocam_api_libs ---
file(GLOB MOTOCAM_API_L1_SRCS "../motocam_api_libs/src/l1/motocam_*.c")
file(GLOB MOTOCAM_API_L2_SRCS "../motocam_api_libs/src/l2/motocam_*.c")
add_executable(test_motocam_api_libs
  test_motocam_api_libs.cpp
  ../motocam_api_libs/src/motocam_api_l1.c
  ../motocam_api_libs/src/motocam_api_l2.c
  ${MOTOCAM_API_L1_SRCS}
  ${MOTOCAM_API_L2_SRCS}
  mocks/mock_fw_api.c
)
target_include_directories(test_motocam_api_libs PRIVATE
  ../motocam_api_libs/include
  ../motocam_api_libs/include/l1
  ../motocam_api_libs/include/l2
  ../motocam_fw_libs/include
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)
target_compile_options(test_motocam_api_libs PRIVATE
  -include ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_fw_extra.h
)
target_compile_definitions(test_motocam_api_libs PRIVATE
  CONFIG_PATH=\"/tmp/test_api/config\"
  M5S_CONFIG_DIR=\"/tmp/test_api/m5s_config\"
  RES_PATH=\"/tmp/test_api\"
)
target_link_libraries(test_motocam_api_libs gtest gtest_main pthread)
add_test(NAME test_motocam_api_libs COMMAND test_motocam_api_libs)
