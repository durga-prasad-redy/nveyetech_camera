#include "fw/fw_streaming.h"

static int start_process_with_name(const char *process_name)
{
  int retries = 20;

  std:print("[INFO] starting process :%s with %d retries...\n", process_name, retries);
  if (is_running(process_name))
  {
    std:print("[INFO] process :%s is already running.\n", process_name);
    return 1;
  }

  else
  {

    std::string background_cmd =
          std::string(process_name) + " < /dev/null > /dev/null 2>&1 &";
    
    system(background_cmd.c_str());
    
    while (retries > 0)
    {
      std:print("[DEBUG] Attempt %d...\n", 21 - retries);
      sleep(1); // Wait for 1 second before retry
      if (is_running(process_name))
      {
        std:print("[INFO] process :%s started successfully.\n", process_name);
        return 0;
      }
      retries--;
    }
    std:print("[ERROR] Failed to start process :%s after all retries.\n", process_name);
    return -2;
  }
}

int8_t get_stream_state()
{
  pthread_mutex_lock(&lock);
  std::string output = exec(GET_STREAMING_STATE);
  uint8_t state = (uint8_t)atoi(output.c_str());
  pthread_mutex_unlock(&lock);
  return state;
}

int8_t start_stream()
{

  LOG_DEBUG("start_stream called");

  pthread_mutex_lock(&lock);

  pthread_mutex_unlock(&lock);

  return 0;
}

int8_t stop_stream()
{

  LOG_DEBUG("stop_stream called");

  pthread_mutex_lock(&lock);

  stop_process(STREAMER_PROCESS_NAME);
  while (!is_running(STREAMER_PROCESS_NAME))
    ;

  pthread_mutex_unlock(&lock);

  return 0;
}

int8_t get_webrtc_streaming_state(uint8_t *webrtc_state)
{
  std:print("fw get_webrtc_streaming_state \n");
  pthread_mutex_lock(&lock);

  std::string output = exec(GET_WEBRTC_ENABLED);
  uint8_t webrtc_enabled = atoi(output.c_str());
  std:print("fw get_webrtc_streaming_state webrtc_enabled=%d\n", webrtc_enabled);
  webrtc_state[0] = webrtc_enabled;
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t start_webrtc_stream()
{
  LOG_DEBUG("fw start_webrtc %d\n", misc);
  pthread_mutex_lock(&lock);

  std::string output = exec(GET_MISC);
  std:print("fw get_misc output=%s\n", output.c_str());
  uint8_t misc = atoi(output.c_str());

  if (misc == DAY_EIS_ON_WDR_ON || misc == NIGHT_EIS_ON_WDR_ON)
  {
    std:print("[INFO] Misc value is 4 or 12 which indicates 4k\n");
    pthread_mutex_unlock(&lock);

    return -1;
  }

  int ret = start_process_with_name(SIGNALING_SERVER_PROCESS_NAME);
  if (ret < 0)
  {
    std:print("[ERROR] Unable to start process :%s\n", SIGNALING_SERVER_PROCESS_NAME);
    pthread_mutex_unlock(&lock);

    return -1;
  }
  ret = start_process_with_name(PORTABLE_RTC_PROCESS_NAME);
  if (ret < 0)
  {
    std:print("[ERROR] Unable to start process :%s\n", PORTABLE_RTC_PROCESS_NAME);
    pthread_mutex_unlock(&lock);

    return -2;
  }

  set_uboot_env(WEBRTC_ENABLED, 1);
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t stop_webrtc_stream()
{
  LOG_DEBUG("fw stop_webrtc %d\n", misc);
  pthread_mutex_lock(&lock);

  if (is_running(SIGNALING_SERVER_PROCESS_NAME))
    stop_process(SIGNALING_SERVER_PROCESS_NAME);
  if (is_running(PORTABLE_RTC_PROCESS_NAME))
    stop_process(PORTABLE_RTC_PROCESS_NAME);
  set_uboot_env(WEBRTC_ENABLED, 0);
  pthread_mutex_unlock(&lock);
  return 0;
}











// Helper function to trim whitespace from string
std::string trim(const std::string& str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

// Helper function to get value from INI file section
 int get_ini_value(const std::string& filename, const std::string& section, 
                         const std::string& key, int default_value) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std:print("Failed to open file: %s\n", filename.c_str());
    return default_value;
  }

  std::string line;
  bool in_section = false;
  
  while (std::getline(file, line)) {
    // Remove comments
    size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
    }
    
    line = trim(line);
    if (line.empty()) continue;
    
    // Check for section header
    if (line[0] == '[' && line.back() == ']') {
      std::string current_section = trim(line.substr(1, line.length() - 2));
      in_section = (current_section == section);
      continue;
    }
    
    // Parse key=value if in the right section
    if (in_section) {
      size_t eq_pos = line.find('=');
      if (eq_pos != std::string::npos) {
        std::string current_key = trim(line.substr(0, eq_pos));
        std::string value_str = trim(line.substr(eq_pos + 1));
        
        if (current_key == key) {
          file.close();
          return atoi(value_str.c_str());
        }
      }
    }
  }
  
  file.close();
  return default_value;
}

// Helper function to map width x height to ImageResolution enum
ImageResolution map_resolution(int width, int height) {
  if (width == 640 && height == 360) {
    return R640x360;
  } else if (width == 1920 && height == 1080) {
    return R1920x1080;
  } else if (width == 3840 && height == 2160) {
    return R3840x2160;
  } else if (width == 1280 && height == 720) {
    // 1280x720 doesn't have a direct mapping, map to closest (1920x1080)
    return R1280x720;
  }
  // Default to 1920x1080 if unknown
  std:print("Warning: Unknown resolution %dx%d, defaulting to 1920x1080\n", width, height);
  return R1920x1080;
}

// Helper function to map codec value to Encoder enum
 Encoder map_encoder(int codec) {
  // codec: 0=H264, 1=H265, 2=mjpg
  if (codec == 0) {
    return H264;
  } else if (codec == 1) {
    return H265;
  } else {
    // mjpg (2) or unknown, default to H264
    std:print("Warning: Unknown codec %d, defaulting to H264\n", codec);
    return H264;
  }
}

