#include "fw/fw_streaming.h"

static int start_process_with_name(const char *process_name)
{
  int retries = 20;

  printf("[INFO] starting process :%s with %d retries...\n", process_name, retries);
  if (is_running(process_name))
  {
    printf("[INFO] process :%s is already running.\n", process_name);
    return 1;
  }

  else
  {

    char background_cmd[600];
    snprintf(background_cmd, sizeof(background_cmd),
             "%s < /dev/null > /dev/null 2>&1 &", process_name);
    system(background_cmd);
    while (retries > 0)
    {
      printf("[DEBUG] Attempt %d...\n", 21 - retries);
      sleep(1); // Wait for 1 second before retry
      if (is_running(process_name))
      {
        printf("[INFO] process :%s started successfully.\n", process_name);
        return 0;
      }
      retries--;
    }
    printf("[ERROR] Failed to start process :%s after all retries.\n", process_name);
    return -2;
  }
}

int8_t get_stream_state()
{
    uint8_t state = 0;

    pthread_mutex_lock(&lock);
    EXEC_GET_UINT8(GET_STREAMING_STATE, &state);
    pthread_mutex_unlock(&lock);
    return (int8_t)state;
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
  uint8_t webrtc_enabled = 0;
  printf("fw get_webrtc_streaming_state \n");
  pthread_mutex_lock(&lock);
  EXEC_GET_UINT8(GET_WEBRTC_ENABLED, &webrtc_enabled);
  printf("fw get_webrtc_streaming_state webrtc_enabled=%d\n", webrtc_enabled);
  webrtc_state[0] = webrtc_enabled;
  pthread_mutex_unlock(&lock);
  return 0;
}

int8_t start_webrtc_stream()
{
  uint8_t misc = 0;
  LOG_DEBUG("fw start_webrtc %d\n", misc);
  pthread_mutex_lock(&lock);

  EXEC_GET_UINT8(GET_MISC, &misc);
  printf("fw get_misc misc=%d\n", misc);

  if (misc == DAY_EIS_ON_WDR_ON || misc == NIGHT_EIS_ON_WDR_ON)
  {
    printf("[INFO] Misc value is 4 or 12 which indicates 4k\n");
    pthread_mutex_unlock(&lock);

    return -1;
  }

  int ret = start_process_with_name(SIGNALING_SERVER_PROCESS_NAME);
  if (ret < 0)
  {
    printf("[ERROR] Unable to start process :%s\n", SIGNALING_SERVER_PROCESS_NAME);
    pthread_mutex_unlock(&lock);

    return -1;
  }
  ret = start_process_with_name(PORTABLE_RTC_PROCESS_NAME);
  if (ret < 0)
  {
    printf("[ERROR] Unable to start process :%s\n", PORTABLE_RTC_PROCESS_NAME);
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









/* Trim leading and trailing whitespace (modifies string in place) */
char *trim(char *str)
{
    char *end;

    if (!str)
        return str;

    /* Trim leading space */
    while (*str && isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';
    return str;
}

int get_ini_value(const char *filename,
                  const char *section,
                  const char *key,
                  int default_value)
{
    FILE *file;
    char line[256];
    int in_section = 0;

    if (!filename || !section || !key)
        return default_value;

    file = fopen(filename, "r");
    if (!file)
    {
        printf("Failed to open file: %s\n", filename);
        return default_value;
    }

    while (fgets(line, sizeof(line), file))
    {
        /* Remove comments */
        char *comment = strchr(line, '#');
        if (comment)
            *comment = '\0';

        char *trimmed = trim(line);
        if (*trimmed == '\0')
            continue;

        /* Section header */
        if (trimmed[0] == '[')
        {
            char *end = strchr(trimmed, ']');
            if (end)
            {
                *end = '\0';
                in_section = (strcmp(trim(trimmed + 1), section) == 0);
            }
            continue;
        }

        /* key=value */
        if (in_section)
        {
            char *eq = strchr(trimmed, '=');
            if (eq)
            {
                *eq = '\0';
                const char *current_key = trim(trimmed);
                const char *value_str = trim(eq + 1);

                if (strcmp(current_key, key) == 0)
                {
                    fclose(file);
                    return atoi(value_str);
                }
            }
        }
    }

    fclose(file);
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
  printf("Warning: Unknown resolution %dx%d, defaulting to 1920x1080\n", width, height);
  return R1920x1080;
}

// Helper function to map codec value to Encoder enum
Encoder map_encoder(int codec)
{
    if (codec == 0) {
        return H264;
    } else if (codec == 1) {
        return H265;
    } else {
    printf("Warning: Unknown codec %d, defaulting to H264\n", codec);
    return H264;
    }
}

