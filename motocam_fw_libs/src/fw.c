//
// Created by sr on 05/06/22.
//
#include "fw.h"
#include <dirent.h>
#include <time.h>

pthread_mutex_t lock;

#define EXEC_TMP_BUF 128

int exec_cmd(const char *cmd, char *out, size_t out_size)
{
    FILE *pipe;
    char buffer[EXEC_TMP_BUF];
    size_t len;

    if (!cmd || !out || out_size == 0)
        return -1;

    LOG_DEBUG("%s\n", cmd);

    pipe = popen(cmd, "r");
    if (!pipe)
        return -1;

    out[0] = '\0';

    while (fgets(buffer, sizeof(buffer), pipe))
    {
        len = strlen(out);
        if (len >= out_size - 1)
            break;

        strncat(out, buffer, out_size - len - 1);
    }

    pclose(pipe);
    return 0;
}

int exec_return(const char *cmd)
{
    FILE *pipe;
    char buffer[4096];
    int status;
    int exitCode = -1;

    if (!cmd)
        return -1;

    LOG_DEBUG("%s\n", cmd);

    pipe = popen(cmd, "r");
    if (!pipe)
        return -1;

    /* Read and discard stdout */
    while (fgets(buffer, sizeof(buffer), pipe))
    {
        /* no-op */
    }

    status = pclose(pipe);

    if ((status != -1) && (WIFEXITED(status)))
    {
        exitCode = WEXITSTATUS(status);
    }

    return exitCode;
}

int is_running(const char *process_name)
{
    DIR *proc;
    struct dirent *ent;
    char path[256];
    char cmdline[512];

    if (!process_name || *process_name == '\0')
        return 0;

    proc = opendir("/proc");
    if (!proc)
        return 0;

    while ((ent = readdir(proc)) != NULL)
    {
        if (!isdigit((unsigned char)ent->d_name[0]))
            continue;

        snprintf(path, sizeof(path), "/proc/%s/cmdline", ent->d_name);

        FILE *fp = fopen(path, "r");
        if (!fp)
            continue;

        size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, fp);
        fclose(fp);

        if (n == 0)
            continue;

        cmdline[n] = '\0';

        /* cmdline is NUL-separated; first string is argv[0] */
        if (strstr(cmdline, process_name))
        {
            closedir(proc);
            return 1;
        }
    }

    closedir(proc);
    return 0;
}

int8_t get_mode(void)
{
    char output[64];
    int8_t day_mode = 0;

    if (exec_cmd(GET_DAY_MODE, output, sizeof(output)) == 0)
        day_mode = (int8_t)atoi(output);

    return day_mode;
}

int8_t is_ir_temp(void)
{
    char output[64];
    int8_t is_ir_temp = 0;

    if (exec_cmd(GET_IS_IR_TEMP, output, sizeof(output)) == 0)
        is_ir_temp = (int8_t)atoi(output);

    return is_ir_temp;
}

int8_t is_sensor_temp(void)
{
    char output[64];
    int8_t is_sensor_temp = 0;

    if (exec_cmd(GET_IS_SENSOR_TEMP, output, sizeof(output)) == 0)
        is_sensor_temp = (int8_t)atoi(output);

    return is_sensor_temp;
}

int8_t get_ir_tmp_ctl(void)
{
    char output[64];
    int8_t ir_tmp_ctl = 0;

    if (exec_cmd(GET_IR_TEMP_CTL, output, sizeof(output)) == 0)
        ir_tmp_ctl = (int8_t)atoi(output);

    return ir_tmp_ctl;
}

// One-file-per-key configuration system functions

int8_t set_uboot_env(const char *key, uint8_t value)
{
  char tmp_path[256];
  char file_path[256];
  snprintf(file_path, sizeof(file_path), "%s/%s", M5S_CONFIG_DIR, key);
  snprintf(tmp_path, sizeof(tmp_path), "%s/%s.tmp", M5S_CONFIG_DIR, key);

  FILE *f = fopen(tmp_path, "w");
  if (!f)
    return -1;

  fprintf(f, "%d\n", value);
  fflush(f);
  fsync(fileno(f));   // ensures data is written to flash
  fclose(f);

  if (rename(tmp_path, file_path) != 0)
  {
    perror("rename failed");
    unlink(tmp_path);
    return -1;
  }

  return 0;
}

int8_t set_uboot_env_chars(const char *key, const char *value)
{
  char tmp_path[256];
  char file_path[256];
  snprintf(file_path, sizeof(file_path), "%s/%s", M5S_CONFIG_DIR, key);
  snprintf(tmp_path, sizeof(tmp_path), "%s/%s.tmp", M5S_CONFIG_DIR, key);

  FILE *f = fopen(tmp_path, "w");
  if (!f)
    return -1;

  fprintf(f, "%s\n", value);
  fflush(f);
  fsync(fileno(f));
  fclose(f);

  if (rename(tmp_path, file_path) != 0)
  {
    perror("rename failed");
    unlink(tmp_path);
    return -1;
  }

  return 0;
}
// Stop any process by name

void stop_process(const char *process_name)
{

  if (process_name == NULL || *process_name == '\0')
  {

    fprintf(stderr, "Invalid process name.\n");

    return;
  }

  char cmd[256];

  snprintf(cmd, sizeof(cmd), "killall %s", process_name);

  int ret = system(cmd);

  if (ret == -1)
  {

    LOG_ERROR("Failed to execute killall");
  }
}

// PWM lookup tables
const char *get_pwm5_val(uint8_t val)
{
  switch (val)
  {
  case ULTRA:
    return IR_90;
  case MAX:
    return IR_80;
  case HIGH:
    return IR_60;
  case MEDIUM:
    return IR_30;
  case LOW:
    return IR_10;
  case IR_OFF:
    return IR_0;
  default:
    return NULL;
  }
}

const char *get_pwm4_val(uint8_t val)
{
  switch (val)
  {
  case ULTRA:
    return IR_80;
  case MAX:
    return IR_60;
  case HIGH:
    return IR_40;
  case MEDIUM:
    return IR_30;
  case LOW:
    return IR_10;
  case IR_OFF:
    return IR_0;
  default:
    return NULL;
  }
}

int8_t set_haptic_motor(int duty_cycle, int duration)
{
  static char pwm7_val[32]; // buffer to hold the string

  snprintf(pwm7_val, sizeof(pwm7_val), "%u,%u", duty_cycle, duration);

  // Open lock file
  int lock_fd = open(LOCK_FILE, O_RDWR | O_CREAT, 0666);
  if (lock_fd < 0)
  {
    perror("Failed to open lock file");
    return -1;
  }

  // Try to acquire file lock (non-blocking)
  struct flock fl;
  memset(&fl, 0, sizeof(fl));
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  if (fcntl(lock_fd, F_SETLK, &fl) == -1)
  {
    perror("Could not acquire lock, exiting");
    close(lock_fd);
    return -1;
  }

  // Write to /dev/pwmdev-7
  FILE *pwm7_fp = fopen(PWM7_FILE, "w");
  if (!pwm7_fp)
  {
    perror("Failed to write to pwmdev-7");
    close(lock_fd);
    return -1;
  }
  fprintf(pwm7_fp, "%s\n", pwm7_val);
  fclose(pwm7_fp);

  // Release lock
  fl.l_type = F_UNLCK;
  fcntl(lock_fd, F_SETLK, &fl);
  close(lock_fd);

  return 0;
}

int8_t debug_pwm4_set(uint8_t val)
{
  static char pwm4_val[32]; // buffer to hold the string
  uint32_t second = (100 - val) * 10000;

  snprintf(pwm4_val, sizeof(pwm4_val), "1000000,%u", second);

  // Open lock file
  int lock_fd = open(LOCK_FILE, O_RDWR | O_CREAT, 0666);
  if (lock_fd < 0)
  {
    perror("Failed to open lock file");
    return 1;
  }

  // Try to acquire file lock (non-blocking)
  struct flock fl;
  memset(&fl, 0, sizeof(fl));
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  if (fcntl(lock_fd, F_SETLK, &fl) == -1)
  {
    perror("Could not acquire lock, exiting");
    close(lock_fd);
    return 1;
  }

  // Write to /dev/pwmdev-4
  FILE *pwm4_fp = fopen(PWM4_FILE, "w");
  if (!pwm4_fp)
  {
    perror("Failed to write to pwmdev-4");
    close(lock_fd);
    return 1;
  }
  fprintf(pwm4_fp, "%s\n", pwm4_val);
  fclose(pwm4_fp);

  // Release lock
  fl.l_type = F_UNLCK;
  fcntl(lock_fd, F_SETLK, &fl);
  close(lock_fd);

  return 0;
}

int8_t debug_pwm5_set(uint8_t val)
{
  static char pwm5_val[32]; // buffer to hold the string
  uint32_t second = (100 - val) * 10000;

  snprintf(pwm5_val, sizeof(pwm5_val), "1000000,%u", second);

  // Open lock file
  int lock_fd = open(LOCK_FILE, O_RDWR | O_CREAT, 0666);
  if (lock_fd < 0)
  {
    perror("Failed to open lock file");
    return 1;
  }

  // Try to acquire file lock (non-blocking)
  struct flock fl;
  memset(&fl, 0, sizeof(fl));
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  if (fcntl(lock_fd, F_SETLK, &fl) == -1)
  {
    perror("Could not acquire lock, exiting");
    close(lock_fd);
    return 1;
  }

  // Write to /dev/pwmdev-5
  FILE *pwm5_fp = fopen(PWM5_FILE, "w");
  if (!pwm5_fp)
  {
    perror("Failed to write to pwmdev-5");
    close(lock_fd);
    return 1;
  }
  fprintf(pwm5_fp, "%s\n", pwm5_val);
  fclose(pwm5_fp);

  // Release lock
  fl.l_type = F_UNLCK;
  fcntl(lock_fd, F_SETLK, &fl);
  close(lock_fd);

  return 0;
}

int8_t outdu_update_brightness(uint8_t val)
{
  const char *pwm5_val = get_pwm5_val(val);
  const char *pwm4_val = get_pwm4_val(val);

  if (!pwm5_val || !pwm4_val)
  {
    fprintf(stderr, "Invalid PWM values computed.\n");
    return 1;
  }

  // Open lock file
  int lock_fd = open(LOCK_FILE, O_RDWR | O_CREAT, 0666);
  if (lock_fd < 0)
  {
    perror("Failed to open lock file");
    return 1;
  }

  // Try to acquire file lock (non-blocking)
  struct flock fl;
  memset(&fl, 0, sizeof(fl));
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  if (fcntl(lock_fd, F_SETLK, &fl) == -1)
  {
    perror("Could not acquire lock, exiting");
    close(lock_fd);
    return 1;
  }

  // Write to /dev/pwmdev-5
  FILE *pwm5_fp = fopen(PWM5_FILE, "w");
  if (!pwm5_fp)
  {
    perror("Failed to write to pwmdev-5");
    close(lock_fd);
    return 1;
  }
  fprintf(pwm5_fp, "%s\n", pwm5_val);
  fclose(pwm5_fp);

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 100 * 1000 * 1000; // 100 ms
  (void)nanosleep(&ts, NULL);

  // Write to /dev/pwmdev-4
  FILE *pwm4_fp = fopen(PWM4_FILE, "w");
  if (!pwm4_fp)
  {
    perror("Failed to write to pwmdev-4");
    close(lock_fd);
    return 1;
  }
  fprintf(pwm4_fp, "%s\n", pwm4_val);
  fclose(pwm4_fp);

  // Release lock
  fl.l_type = F_UNLCK;
  fcntl(lock_fd, F_SETLK, &fl);
  close(lock_fd);

  set_uboot_env(IR, val);

  return 0;
}

uint8_t access_file(const char *path)
{
  if (access(path, F_OK) == 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// Utility: Remove file or symlink if it exists
void safe_remove(const char *path)
{
    if ((access(path, F_OK) == 0) && (remove(path) != 0))
    {
        perror(path);
    }
}

// Utility: Create symbolic link after safely removing existing
void safe_symlink(const char *target, const char *linkpath)
{
  safe_remove(linkpath);
  if (symlink(target, linkpath) != 0)
  {
    perror("symlink");
    fprintf(stderr, "Failed to link %s -> %s\n", linkpath, target);
  }
}

void create_file(const char *path)
{
  FILE *file = fopen(path, "w");
  if (file)
  {
    fclose(file);
  }
  else
  {
    LOG_ERROR("Error creating file");
  }
}

void delete_file(const char *path)
{
  if (remove(path) != 0)
  {
    LOG_ERROR("Error deleting file");
  }
}

void fusion_eis_on()
{
  safe_remove(CONFIG_PATH "/stream.ini");
  safe_remove(CONFIG_PATH "/fec_conf.ini");

  safe_symlink(CONFIG_PATH "/stream_f_2.ini.eis_on", CONFIG_PATH "/stream.ini");
  safe_symlink(CONFIG_PATH "/fec_conf.ini.2M.WDR.eis_on",
               CONFIG_PATH "/fec_conf.ini");
}

void fusion_eis_off()
{
  safe_remove(CONFIG_PATH "/stream.ini");
  safe_remove(CONFIG_PATH "/fec_conf.ini");

  safe_symlink(CONFIG_PATH "/stream_f_2.ini.eis_off",
               CONFIG_PATH "/stream.ini");
  safe_symlink(CONFIG_PATH "/fec_conf.ini.2M.WDR.eis_off",
               CONFIG_PATH "/fec_conf.ini");
}

void linear_eis_on()
{
  safe_remove(CONFIG_PATH "/stream.ini");
  safe_remove(CONFIG_PATH "/fec_conf.ini");

  safe_symlink(CONFIG_PATH "/stream_l_2.ini.eis_on", CONFIG_PATH "/stream.ini");
  safe_symlink(CONFIG_PATH "/fec_conf.ini.2M.Linear.eis_on",
               CONFIG_PATH "/fec_conf.ini");
}

void linear_eis_off()
{
  safe_remove(CONFIG_PATH "/stream.ini");
  safe_remove(CONFIG_PATH "/fec_conf.ini");

  safe_symlink(CONFIG_PATH "/stream_l_2.ini.eis_off",
               CONFIG_PATH "/stream.ini");
  safe_symlink(CONFIG_PATH "/fec_conf.ini.2M.Linear.eis_off",
               CONFIG_PATH "/fec_conf.ini");
}

void linear_lowlight_on()
{
  safe_remove(CONFIG_PATH "/stream.ini");
  safe_remove(CONFIG_PATH "/fec_conf.ini");

  safe_symlink(CONFIG_PATH "/stream_ll_2.ini.eis_off",
               CONFIG_PATH "/stream.ini");
  safe_symlink(CONFIG_PATH "/fec_conf.ini.2M.Linear.eis_off",
               CONFIG_PATH "/fec_conf.ini");
}

void linear_4k_on()
{
  safe_remove(CONFIG_PATH "/stream.ini");
  safe_remove(CONFIG_PATH "/fec_conf.ini");

  safe_symlink(CONFIG_PATH "/stream_l_8.ini.eis_off",
               CONFIG_PATH "/stream.ini");
  safe_symlink(CONFIG_PATH "/fec_conf.ini.8M.Linear.eis_off",
               CONFIG_PATH "/fec_conf.ini");
}

void stream_server_config_4k_on(){
  safe_remove(CONFIG_PATH "/stream_server_config.ini");
  safe_symlink(CONFIG_PATH "/stream_server_config_4k.ini",
               CONFIG_PATH "/stream_server_config.ini");
}

void stream_server_config_2k_on(){
  safe_remove(CONFIG_PATH "/stream_server_config.ini");
  safe_symlink(CONFIG_PATH "/stream_server_config_2k.ini",
               CONFIG_PATH "/stream_server_config.ini");
  }

void kill_all_processes(){


  if(is_running(STREAM_RESTART_SERVICE_PROCESS_NAME))
    stop_process(STREAM_RESTART_SERVICE_PROCESS_NAME);
  if(is_running(PORTABLE_RTC_PROCESS_NAME))
    stop_process(PORTABLE_RTC_PROCESS_NAME);
  if(is_running(SIGNALING_SERVER_PROCESS_NAME))
    stop_process(SIGNALING_SERVER_PROCESS_NAME);
  if(is_running(STREAMER_PROCESS_NAME))
    stop_process(STREAMER_PROCESS_NAME);
  if(is_running(RTSP_SERVER_PROCESS_NAME))
    stop_process(RTSP_SERVER_PROCESS_NAME);
  if(is_running(ONVIF_SERVER_PROCESS_NAME))
    stop_process(ONVIF_SERVER_PROCESS_NAME);
  if(is_running(CAMERA_MONITOR_PROCESS_NAME))
    stop_process(CAMERA_MONITOR_PROCESS_NAME);
}
