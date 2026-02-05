#include "fw/fw_system.h"

#define GET_CPU_USAGE                                         \
  "{ head -n1 /proc/stat; sleep 1; head -n1 /proc/stat; } | " \
  "awk 'NR==1 {u=$2; s=$4; t=$2+$3+$4+$5+$6+$7} "             \
  "NR==2 {du=$2-u; ds=$4-s; dt=$2+$3+$4+$5+$6+$7 - t; "       \
  "printf \"%.1f\\n\", (du+ds)/dt*100}'"

#define GET_MEMORY_USAGE                                                \
  "awk '/MemTotal/{total=$2} /MemFree/{free=$2} /Buffers/{buffers=$2} " \
  "/Cached/{cached=$2} END {used=total-free-buffers-cached;printf "     \
  "(used/total)*100;}' /proc/meminfo"

#define SET_FACTORY_RESET "/mnt/flash/vienna/bin/factory_reset_bin camera_factory_reset"

#define SET_CONFIG_RESET "/mnt/flash/vienna/bin/factory_reset_bin camera_configuration_reset"
#define SET_CAMERA_NAME "camera_name"
#define SET_LOGIN_PIN "login_pin"

#define OTA_UPDATE_COMMAND "/mnt/flash/vienna/firmware/ota/ota_service &"

#define OTA_STATUS "ota_status"
#define GET_OTA_STATUS "cat " M5S_CONFIG_DIR "/ota_status"

#define OTA_FILE_PATH "/mnt/flash/vienna/firmware/ota/"

#define FACTORY_RESET_STATUS "factory_reset_status"
#define GET_FACTORY_RESET_STATUS "cat " M5S_CONFIG_DIR "/factory_reset_status"

// system commands
#define GET_CAMERA_NAME "cat " M5S_CONFIG_DIR "/camera_name"
#define GET_FIRMWARE_VERSION "cat " M5S_CONFIG_DIR "/firmware_version"
#define GET_MAC_ADDRESS "cat " M5S_CONFIG_DIR "/ethaddr"
#define GET_LOGIN_PIN "cat " M5S_CONFIG_DIR "/login_pin"

#define GET_STREAMING_STATE \
  "cat " M5S_CONFIG_DIR "/stream_state" // 1:Started, 0:Stopped
#define START_STREAM "/mnt/flash/vienna/motocam/set/outdu_start_stream.sh"
#define STOP_STREAM "/mnt/flash/vienna/motocam/set/outdu_stop_stream.sh"
#define SHUTDOWN "/mnt/flash/vienna/motocam/set/outdu_shutdown.sh"
#define USER_DOB_FILE M5S_CONFIG_DIR "/user_dob"
#define SET_TIME_COMMAND "/mnt/flash/vienna/scripts/time_manager.sh update_time"

// ---------------------------------------------------------------------

int8_t get_cpu_usage(uint8_t *cpu_usage);

int8_t get_memory_usage(uint8_t *memory_usage)
{
  std::string output = exec(GET_MEMORY_USAGE);
  if (output.empty())
  {
    LOG_ERROR("Failed to get memory usage");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  printf("Memory Usage: %s\n", output.c_str());
  *memory_usage = (uint8_t)atoi(output.c_str());
  return 0;
}

int8_t camera_health_check(uint8_t *streamer, uint8_t *rtsp,
                           uint8_t *portable_rtc, uint8_t *cpu_usage,
                           uint8_t *memory_usage, uint8_t *isp_temp,
                           uint8_t *ir_temp, uint8_t *sensor_temp)
{
  pthread_mutex_lock(&lock);

  // Check if the streamer is running
  *streamer = is_running("streamer");
  *rtsp = is_running("rtsps");
  *portable_rtc = is_running("portablertc");

  // Get CPU usage
  if (get_cpu_usage(cpu_usage) != 0)
  {
    pthread_mutex_unlock(&lock);
    return -1;
  }

  // Get memory usage
  if (get_memory_usage(memory_usage) != 0)
  {
    pthread_mutex_unlock(&lock);
    return -1;
  }

  pthread_mutex_unlock(&lock);

  get_isp_temp(isp_temp);
  get_ir_temp(ir_temp);
  get_sensor_temp(sensor_temp);
  return 0;
}

int8_t provisioning_mode(const char *mac_address, const char *serial_number,
                         const char *manufacture_date)
{

  // check if macaddress, serial number and manufacture date files exist at
  // /mnt/flash/vienna/m5s_ config folder
  if (access_file(DEVICE_SETUP_FILE) != 1)
  {
    printf(
        "Mac address, Serial number or Manufacture date already provisioned\n");
    return 0;
  }

  // Provision mac address

  char cmd[500];
  sprintf(cmd, "%s %s %s %s", DEVICE_SETUP_FILE, mac_address, serial_number,
          manufacture_date);

  int ret = exec_return(cmd);
  if (ret != 0)
  {
    printf("Device provisioning failed\n");
    return -1;
  }

  return 1;
}

int8_t remove_ota_files()
{
  pthread_mutex_lock(&lock);

  // Remove all files matching ota.tar.gz*
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "rm -f %sota.tar.gz*", OTA_FILE_PATH);
  int ret = system(cmd);
  if (ret != 0)
  {
    perror("Failed to remove OTA files");
  }

  pthread_mutex_unlock(&lock);

  return 0;
}


int8_t shutdown_device()
{
  pthread_mutex_lock(&lock);

  // Sleep for 1 second
  sleep(1);

  // Flush file system buffers
  sync();

  // Reboot the system
  int ret = system("reboot");
  if (ret != 0)
  {
    perror("Failed to reboot");
  }

  pthread_mutex_unlock(&lock);

  return 0;
}

int8_t ota_update()
{
  pthread_mutex_lock(&lock);

  printf("executing ota update command");
  set_uboot_env_chars(OTA_STATUS, "update_started");
  // Sleep for 1 second
  kill_all_processes();

  sleep(1);

  // Flush file system buffers
  sync();

  // Update the camera

  std::string output = exec(OTA_UPDATE_COMMAND);
  printf("executing ota update command %s\n", output.c_str());

  pthread_mutex_unlock(&lock);

  return 0;
}


int8_t set_camera_name(const char *camera_name)
{
  pthread_mutex_lock(&lock);
  // char cmd[100];
  // sprintf(cmd, "%s %s", SET_CAMERA_NAME, camera_name);
  // exec(cmd);
  set_uboot_env_chars(SET_CAMERA_NAME, camera_name);
  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t set_login(const char *login_pin, const char *dob)
{
  int8_t dob_validation = validate_user_dob(dob);

  pthread_mutex_lock(&lock);
  
  // Validate DOB
  if (dob_validation != 0) {
    pthread_mutex_unlock(&lock);
    return dob_validation; // Return -6 or -7
  }
  
  
  // char cmd[100];
  // sprintf(cmd, "%s %s", SET_CURRENT_LOGIN_PIN, login_pin);
  // exec(cmd);
  set_uboot_env_chars(SET_LOGIN_PIN, login_pin);
  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t get_camera_name(char *camera_name)
{
  pthread_mutex_lock(&lock);
  std::string output = exec(GET_CAMERA_NAME);
  if (!output.empty() && output.back() == '\n')
  {
    output.pop_back();
  }
  strcpy(camera_name, output.c_str());
  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t get_firmware_version(char *firmware_version)
{
  pthread_mutex_lock(&lock);
  std::string output = exec(GET_FIRMWARE_VERSION);
  strcpy(firmware_version, output.c_str());
  printf("firmware-version %s\n", firmware_version);
  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t get_mac_address(char *mac_address)
{
  pthread_mutex_lock(&lock);
  std::string output = exec(GET_MAC_ADDRESS);
  strcpy(mac_address, output.c_str());
  printf("mac-address %s\n", mac_address);

  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t get_ota_update_status(char *ota_status)
{
  pthread_mutex_lock(&lock);
  std::string output = exec(GET_OTA_STATUS);
  strcpy(ota_status, output.c_str());
  printf("ota_status %s\n", ota_status);

  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t get_factory_reset_status(char *factory_reset_status)
{
  pthread_mutex_lock(&lock);
  std::string output = exec(GET_FACTORY_RESET_STATUS);
  strcpy(factory_reset_status, output.c_str());
  printf("factory_reset_status %s\n", factory_reset_status);

  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t get_login_pin(char *loginPin)
{
  pthread_mutex_lock(&lock);
  std::string output = exec(GET_LOGIN_PIN);
  if (!output.empty() && output.back() == '\n')
  {
    output.pop_back();
  }
  strcpy(loginPin, output.c_str());

  pthread_mutex_unlock(&lock);
  return 0;
}

// Validate DOB: Returns 0 on success, -6 if DOB not set, -7 if DOB mismatch
int8_t validate_user_dob(const char *input_dob)
{
  char stored_dob[11];
  
  // Check if DOB exists in the system
  if (get_user_dob(stored_dob) != 0) {
    LOG_ERROR("validate_user_dob: DOB not set in system\n");
    return -6; // DOB not set
  }
    if(strlen(stored_dob) == 0) {
    LOG_ERROR("validate_user_dob: DOB not set in system\n");
    return -6; // DOB not set
  }
  if(strlen(stored_dob) != 10) {
    LOG_ERROR("validate_user_dob: DOB length mismatch. Expected: 10, Got: %d\n", strlen(stored_dob));
    return -5; // DOB length mismatch
  }
  
  // Validate input DOB matches stored DOB
  if (strcmp(stored_dob, input_dob) != 0) {
    LOG_ERROR("validate_user_dob: DOB mismatch. Expected: %s, Got: %s\n", stored_dob, input_dob);
    return -7; // DOB validation failed
  }
  
  LOG_DEBUG("validate_user_dob: DOB validation successful\n");
  return 0;
}

int8_t set_factory_reset(const char *dob)
{

  int8_t dob_validation = validate_user_dob(dob);

  pthread_mutex_lock(&lock);
  
  // Validate DOB
  if (dob_validation != 0) {
    pthread_mutex_unlock(&lock);
    return dob_validation; // Return -6 or -7
  }
  kill_all_processes();

  char cmd[500];
  sprintf(cmd, "%s", SET_FACTORY_RESET);
  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);
  system(background_cmd);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t set_config_reset(const char *dob)
{
  int8_t dob_validation = validate_user_dob(dob);

  pthread_mutex_lock(&lock);
  
  // Validate DOB
  if (dob_validation != 0) {
    pthread_mutex_unlock(&lock);
    return dob_validation; // Return -6 or -7
  }
  kill_all_processes();

  char cmd[300];
  sprintf(cmd, "%s", SET_CONFIG_RESET);

  char background_cmd[600];
  snprintf(background_cmd, sizeof(background_cmd),
           "%s < /dev/null > /dev/null 2>&1 &", cmd);

  printf("set_config_reset command:%s\n", background_cmd);
  system(background_cmd);



  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_stream_resolution(enum image_resolution *resolution,
                             uint8_t stream_number)
{
  pthread_mutex_lock(&lock);
  char cmd[100];
  sprintf(cmd, "cat " M5S_CONFIG_DIR "/stream%d_resolution", stream_number);
  std::string output = exec(cmd);
  *resolution = (enum image_resolution)atoi(output.c_str());
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t get_stream_fps(uint8_t *fps, uint8_t stream_number)
{
  pthread_mutex_lock(&lock);
  char cmd[100];
  sprintf(cmd, "cat " M5S_CONFIG_DIR "/stream%d_fps", stream_number);
  std::string output = exec(cmd);
  *fps = (uint8_t)atoi(output.c_str());
  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t get_stream_bitrate(uint8_t *bitrate, uint8_t stream_number)
{
  pthread_mutex_lock(&lock);
  char cmd[100];
  sprintf(cmd, "cat " M5S_CONFIG_DIR "/stream%d_bitrate", stream_number);
  std::string output = exec(cmd);
  *bitrate = (uint8_t)atoi(output.c_str());
  pthread_mutex_unlock(&lock);
  return 0;
}
int8_t get_stream_encoder(enum encoder_type *encoder1, uint8_t stream_number)
{
  pthread_mutex_lock(&lock);
  char cmd[100];
  sprintf(cmd, "cat " M5S_CONFIG_DIR "/stream%d_encoder", stream_number);
  std::string output = exec(cmd);
  *encoder1 = (enum encoder_type)atoi(output.c_str());
  pthread_mutex_unlock(&lock);
  return 0;
}

typedef struct
{
  unsigned long user;
  unsigned long nice;
  unsigned long system;
  unsigned long idle;
  unsigned long iowait;
  unsigned long irq;
  unsigned long softirq;
  unsigned long steal;
  unsigned long guest;
  unsigned long guest_nice;
} cpu_stats_t;

int read_cpu_stats(cpu_stats_t *stats)
{
  FILE *fp = fopen("/proc/stat", "r");
  if (fp == NULL)
  {
    return -1;
  }

  int result = fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                      &stats->user, &stats->nice, &stats->system, &stats->idle,
                      &stats->iowait, &stats->irq, &stats->softirq,
                      &stats->steal, &stats->guest, &stats->guest_nice);

  fclose(fp);
  return (result == 10) ? 0 : -1;
}

// Function to calculate total CPU time
unsigned long get_total_time(const cpu_stats_t *stats)
{
  return stats->user + stats->nice + stats->system + stats->idle +
         stats->iowait + stats->irq + stats->softirq + stats->steal +
         stats->guest + stats->guest_nice;
}

// Function to calculate idle time
unsigned long get_idle_time(const cpu_stats_t *stats)
{
  return stats->idle + stats->iowait;
}

int8_t get_cpu_usage(uint8_t *cpu_usage)
{
  cpu_stats_t stats1, stats2;

  // Read first measurement
  if (read_cpu_stats(&stats1) != 0)
  {
    printf("Failed to read CPU stats (first measurement)\n");
    return -1;
  }

  // Wait for 1 second
  sleep(1);

  // Read second measurement
  if (read_cpu_stats(&stats2) != 0)
  {
    printf("Failed to read CPU stats (second measurement)\n");
    return -1;
  }

  // Calculate differences
  unsigned long total1 = get_total_time(&stats1);
  unsigned long total2 = get_total_time(&stats2);
  unsigned long idle1 = get_idle_time(&stats1);
  unsigned long idle2 = get_idle_time(&stats2);

  unsigned long total_diff = total2 - total1;
  unsigned long idle_diff = idle2 - idle1;

  // Avoid division by zero
  if (total_diff == 0)
  {
    printf("No CPU time difference detected\n");
    *cpu_usage = 0;
    return 0;
  }

  // Calculate CPU usage percentage
  double usage = (double)(total_diff - idle_diff) / total_diff * 100.0;

  // Round and clamp to uint8_t range
  *cpu_usage = (uint8_t)(usage + 0.5);
  if (usage > 100.0)
  {
    *cpu_usage = 100;
  }

  printf("CPU Usage: %.1f%%\n", usage);
  return 0;
}

// int8_t get_cpu_usage(uint8_t *cpu_usage)
// {
//     std::string output = exec(GET_CPU_USAGE);
//     if (output.empty())
//     {
//         LOG_ERROR("Failed to get CPU usage");
//         pthread_mutex_unlock(&lock);
//         return -1;
//     }
//     printf("CPU Usage: %s\n", output.c_str());
//     *cpu_usage = (uint8_t)atoi(output.c_str());
//     return 0;
// }

//-----------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------


int8_t get_process_status(const char *process_name, uint8_t *status)
{
  pthread_mutex_lock(&lock);
  *status = is_running(process_name) ? 1 : 0;
  pthread_mutex_unlock(&lock);
  return 0;
}
//---------------------------------------------------------CPU
// USAGE------------------------------------------------------------


int8_t set_user_dob(const char *dob) {
    pthread_mutex_lock(&lock);
    FILE *fp = fopen(USER_DOB_FILE, "w");
    if (fp == NULL) {
        LOG_ERROR("set_user_dob: failed to open file\n");
        pthread_mutex_unlock(&lock);
        return -1;
    }
    
    fprintf(fp, "%s", dob);
    fclose(fp);
    pthread_mutex_unlock(&lock);
    return 0;
}

int8_t get_user_dob(char *dob) {
    pthread_mutex_lock(&lock);
    FILE *fp = fopen(USER_DOB_FILE, "r");
    if (fp == NULL) {
        LOG_ERROR("get_user_dob: file not found\n");
        pthread_mutex_unlock(&lock);
        return -1;
    }
    
    if (fgets(dob, 11, fp) != NULL) {
        // Remove trailing newline if present, though likely handled by set logic
        size_t len = strlen(dob);
        if (len > 0 && dob[len-1] == '\n') dob[len-1] = '\0';
        
        fclose(fp);
        pthread_mutex_unlock(&lock);
        return 0;
    }
    
    fclose(fp);
    pthread_mutex_unlock(&lock);
    return -1;
}

int8_t set_system_time(const char *epoch_time)
{
  pthread_mutex_lock(&lock);
  char cmd[300];
  sprintf(cmd, "%s %s", SET_TIME_COMMAND, epoch_time);
  exec(cmd);
  pthread_mutex_unlock(&lock);
  return 0;
}

