//
// Created by sr on 05/06/22.
//
#include "fw.h"
#include <dirent.h>

pthread_mutex_t lock;

std::string exec(const char *cmd)
{
  LOG_DEBUG("%s\n", cmd);
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
  {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
  {
    result += buffer.data();
  }

  return result;
}

int exec_return(const char *cmd)
{
  LOG_DEBUG("%s\n", cmd);
  std::array<char, 4096> buffer = {{0}};
  std::string result;

  // open pipe/
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
  {
    throw std::runtime_error("popen() failed!");
  }

  // read stdout
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
  {
    result += buffer.data();
  }

  // get exit code
  int status =
      pclose(pipe.release()); // release() -> unique_ptr won't call pclose twice
  int exitCode = -1;
  if (status != -1)
  {
    if (WIFEXITED(status))
    {
      exitCode = WEXITSTATUS(status); // <- This gives 0, 1, etc.
    }
  }

  return exitCode;
}





int is_running(const char *process_name)
{
    DIR *proc;
    struct dirent *ent;
    char path[256];
    char comm[256];

    if (!process_name || *process_name == '\0')
        return 0;

    proc = opendir("/proc");
    if (!proc)
        return 0;

    while ((ent = readdir(proc)) != NULL)
    {
        /* Only numeric directory names = PIDs */
        if (!isdigit((unsigned char)ent->d_name[0]))
            continue;

        snprintf(path, sizeof(path), "/proc/%s/comm", ent->d_name);

        FILE *fp = fopen(path, "r");
        if (!fp)
            continue;

        if (fgets(comm, sizeof(comm), fp))
        {
            /* Remove trailing newline safely */
            char *nl = strchr(comm, '\n');
            if (nl)
                *nl = '\0';

            if (strcmp(comm, process_name) == 0)
            {
                fclose(fp);
                closedir(proc);
                return 1;   /* found */
            }
        }

        fclose(fp);
    }

    closedir(proc);
    return 0;   /* not found */
}

int8_t get_mode()
{
  int8_t day_mode;
  std::string output = exec(GET_DAY_MODE);
  day_mode = atoi(output.c_str());

  return day_mode;
}

int8_t is_ir_temp()
{
  int8_t is_ir_temp;
  std::string output = exec(GET_IS_IR_TEMP);
  is_ir_temp = atoi(output.c_str());

  return is_ir_temp;
}

int8_t is_sensor_temp()
{
  int8_t is_sensor_temp;
  std::string output = exec(GET_IS_SENSOR_TEMP);
  is_sensor_temp = atoi(output.c_str());

  return is_sensor_temp;
}

int8_t get_ir_tmp_ctl()
{
  int8_t ir_tmp_ctl;
  std::string output = exec(GET_IR_TEMP_CTL);
  ir_tmp_ctl = atoi(output.c_str());

  return ir_tmp_ctl;
}

// One-file-per-key configuration system functions

int8_t set_uboot_env(const char *key, uint8_t value)
{
  char tmp_path[256], file_path[256];
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
  char tmp_path[256], file_path[256];
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

  usleep(100000); // sleep 0.1s

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
  if (access(path, F_OK) == 0)
  {
    if (remove(path) != 0)
    {
      perror(path);
    }
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