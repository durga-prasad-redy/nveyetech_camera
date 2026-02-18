#include "metrics_handler.h"
#include "http_utils.h"
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

namespace {

const std::string CONFIG_DIR = "/mnt/flash/vienna/m5s_config";
const std::string SNAPSHOT_FILE = "/mnt/flash/vienna/default_snapshot.txt";
const std::string FACTORY_BACKUP_DIR = "/mnt/flash/vienna/factory_backup";
const std::string FACTORY_RESET_STATUS_FILE =
    "/mnt/flash/vienna/m5s_config/factory_reset_status";
const std::string VERSION_FILE = "/mnt/flash/jffs2_version";

} // anonymous namespace

// Helper function to check if file exists
static bool file_exists(const std::string &path) {
  struct stat st;
  return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

// Helper function to check if directory exists
static bool dir_exists(const std::string &path) {
  DIR *dir = opendir(path.c_str());
  if (dir) {
    closedir(dir);
    return true;
  }
  return false;
}

// Helper function to get file size
static long long get_file_size(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) == 0)
    return st.st_size;
  return -1;
}

// Helper function to execute command and get output
static std::string exec_command(const std::string &cmd) {
  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe)
    return "";

  std::string buffer(128, '\0');
  std::string result;
  while (fgets(&buffer[0], static_cast<int>(buffer.size()), pipe) != nullptr) {
    result += buffer.c_str();
  }
  pclose(pipe);

  // Remove trailing newline
  if (!result.empty() && result.back() == '\n')
    result.pop_back();

  return result;
}

// Helper function to get disk space info
static void get_disk_space(const std::string &path, long long *total,
                           long long *available, long long *used) {
  struct statvfs st;
  if (statvfs(path.c_str(), &st) == 0) {
    *total = (long long)st.f_blocks * st.f_frsize;
    *available = (long long)st.f_bavail * st.f_frsize;
    *used = *total - (long long)st.f_bfree * st.f_frsize;
  } else {
    *total = -1;
    *available = -1;
    *used = -1;
  }
}

static std::string json_escape(const std::string &str) {
  static const char hex[] = "0123456789ABCDEF";

  std::string escaped;
  escaped.reserve(str.size() * 2);

  for (unsigned char c : str) {
    switch (c) {
    // Using raw string literals R"(...)"
    case '"':
      escaped += R"(\")";
      break;
    case '\\':
      escaped += R"(\\)";
      break;
    case '\b':
      escaped += R"(\b)";
      break;
    case '\f':
      escaped += R"(\f)";
      break;
    case '\n':
      escaped += R"(\n)";
      break;
    case '\r':
      escaped += R"(\r)";
      break;
    case '\t':
      escaped += R"(\t)";
      break;

    default:
      if (c < 0x20 || c > 0x7E) {
        escaped += R"(\u00)";
        escaped += hex[(c >> 4) & 0xF];
        escaped += hex[c & 0xF];
      } else {
        escaped += c;
      }
      break;
    }
  }

  return escaped;
}
// Count files in directory
static int count_files_in_dir(const std::string &path) {
  DIR *dir = opendir(path.c_str());
  if (!dir)
    return -1;

  int count = 0;
  const struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] != '.') {
      count++;
    }
  }
  closedir(dir);
  return count;
}

// Get CPU usage percentage
static int get_cpu_usage() {
  FILE *f;
  std::string buf(256, '\0');

  unsigned long u1;
  unsigned long n1;
  unsigned long s1;
  unsigned long i1;
  unsigned long uw1;
  unsigned long irq1;
  unsigned long si1;
  unsigned long st1;

  unsigned long u2;
  unsigned long n2;
  unsigned long s2;
  unsigned long i2;
  unsigned long uw2;
  unsigned long irq2;
  unsigned long si2;
  unsigned long st2;

  f = fopen("/proc/stat", "r");
  if (!f)
    return -1;

  fgets(&buf[0], static_cast<int>(buf.size()), f);
  sscanf(buf.c_str(), "cpu %lu %lu %lu %lu %lu %lu %lu %lu", &u1, &n1, &s1, &i1,
         &uw1, &irq1, &si1, &st1);
  fclose(f);

  sleep(1);

  f = fopen("/proc/stat", "r");
  if (!f)
    return -1;

  fgets(&buf[0], static_cast<int>(buf.size()), f);
  sscanf(buf.c_str(), "cpu %lu %lu %lu %lu %lu %lu %lu %lu", &u2, &n2, &s2, &i2,
         &uw2, &irq2, &si2, &st2);
  fclose(f);

  unsigned long t1 = u1 + n1 + s1 + i1 + uw1 + irq1 + si1 + st1;
  unsigned long t2 = u2 + n2 + s2 + i2 + uw2 + irq2 + si2 + st2;
  unsigned long idle1 = i1;
  unsigned long idle2 = i2;
  unsigned long diff_total = t2 - t1;
  unsigned long diff_idle = idle2 - idle1;

  if (diff_total == 0)
    return -1;

  int cpu = 100 * (diff_total - diff_idle) / diff_total;
  return cpu;
}

// Get temperature in Celsius
static int get_temperature() {
  FILE *tf = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
  if (!tf)
    return -1;

  int raw;
  if (fscanf(tf, "%d", &raw) != 1) {
    fclose(tf);
    return -1;
  }
  fclose(tf);

  return raw; // Convert from millidegrees to degrees
}

// Get firmware version
static std::string get_firmware_version() {
  std::string version = read_file_content(VERSION_FILE);
  if (version.empty())
    return "UNKNOWN";

  // Remove trailing newline if present
  if (!version.empty() && version.back() == '\n')
    version.pop_back();

  return version;
}

// Extract basename from path
static std::string get_basename(const std::string &path) {
  std::string::size_type pos = path.rfind('/');
  return (pos != std::string::npos) ? path.substr(pos + 1) : path;
}

// Check if /proc/<pid>/comm matches the target process name
static bool match_proc_comm(const char *pid_dir, const std::string &name) {
  std::string path = "/proc/" + std::string(pid_dir) + "/comm";

  FILE *f = fopen(path.c_str(), "r");
  if (!f)
    return false;

  std::string buf(128, '\0');
  if (!fgets(&buf[0], static_cast<int>(buf.size()), f)) {
    fclose(f);
    return false;
  }
  fclose(f);

  // Strip trailing newline
  //   size_t len = strcspn(buf.c_str(), "\n");
  size_t pos = buf.find_first_of("\n");
  size_t len = (pos == std::string::npos) ? buf.length() : pos;
  std::string comm(buf.c_str(), len);

  // Check exact match
  if (comm == name)
    return true;

  // comm is truncated to 15 chars by the kernel; check prefix match
  if (comm.size() == 15 && name.size() >= 15 && name.compare(0, 15, comm) == 0)
    return true;

  return false;
}

// Check if /proc/<pid>/cmdline matches the target process name
static bool match_proc_cmdline(const char *pid_dir, const std::string &name) {
  std::string path = "/proc/" + std::string(pid_dir) + "/cmdline";

  FILE *f = fopen(path.c_str(), "r");
  if (!f)
    return false;

  std::string buf(512, '\0');
  size_t len = fread(&buf[0], 1, buf.size() - 1, f);
  fclose(f);

  if (len == 0)
    return false;

  std::string cmdline(buf.c_str(), len);

  // Check if basename of the executable matches
  if (get_basename(cmdline) == name)
    return true;

  // Check if name appears anywhere in cmdline (for full path matches)
  if (cmdline.find(name) != std::string::npos)
    return true;

  return false;
}

// Check if process is running
static bool is_process_running(const std::string &name) {
  DIR *dp = opendir("/proc");
  if (!dp)
    return false;

  const struct dirent *e;
  while ((e = readdir(dp)) != nullptr) {
    if (!isdigit(static_cast<unsigned char>(e->d_name[0])))
      continue;

    if (match_proc_comm(e->d_name, name) ||
        match_proc_cmdline(e->d_name, name)) {
      closedir(dp);
      return true;
    }
  }
  closedir(dp);
  return false;
}

// Format uptime string
static std::string format_uptime(double seconds) {
  if (seconds < 0)
    return "N/A";

  auto days = (int)(seconds / 86400);
  auto hours = (int)((seconds - days * 86400) / 3600);
  auto minutes = (int)((seconds - days * 86400 - hours * 3600) / 60);

  std::ostringstream oss;
  if (days > 0)
    oss << days << " days, " << hours << " hours, " << minutes << " minutes";
  else if (hours > 0)
    oss << hours << " hours, " << minutes << " minutes";
  else
    oss << minutes << " minutes";
  return oss.str();
}
static void append_factory_reset(std::ostringstream &json) {
  std::string reset_status = read_file_content(FACTORY_RESET_STATUS_FILE);

  if (reset_status.empty())
    reset_status = "unknown";

  json << "  \"factory_reset\": {\n";

  // Converted to a raw string literal R"(...)"
  json << R"(    "status": ")" << json_escape(reset_status) << R"(",)" << "\n";

  json << "    \"status_file_exists\": "
       << (file_exists(FACTORY_RESET_STATUS_FILE) ? "true" : "false") << "\n";
  json << "  },\n";
}

static void append_configuration(std::ostringstream &json) {
  json << "  \"configuration\": {\n";

  // Converted to raw string literal R"(...)"
  json << R"(    "config_dir": ")" << CONFIG_DIR << R"(",)" << "\n";

  json << "    \"config_dir_exists\": "
       << (dir_exists(CONFIG_DIR) ? "true" : "false") << ",\n";

  if (dir_exists(CONFIG_DIR))
    json << "    \"config_files_count\": " << count_files_in_dir(CONFIG_DIR)
         << ",\n";

  json << R"(    "snapshot_file": ")" << SNAPSHOT_FILE << R"(",)" << "\n";

  json << "    \"snapshot_file_exists\": "
       << (file_exists(SNAPSHOT_FILE) ? "true" : "false");

  if (file_exists(SNAPSHOT_FILE))
    json << ",\n    \"snapshot_file_size\": " << get_file_size(SNAPSHOT_FILE);

  json << "\n  },\n";
}

static void append_factory_backup(std::ostringstream &json) {
  json << "  \"factory_backup\": {\n";
  json << R"(    "backup_dir": ")" << FACTORY_BACKUP_DIR << R"(",)" << ",\n";
  json << "    \"backup_dir_exists\": "
       << (dir_exists(FACTORY_BACKUP_DIR) ? "true" : "false");
  if (!dir_exists(FACTORY_BACKUP_DIR)) {
    json << "\n  },\n";
    return;
  }
  std::string vienna_tar = FACTORY_BACKUP_DIR + "/vienna.tar";
  std::string etc_tar = FACTORY_BACKUP_DIR + "/etc.tar";
  json << ",\n    \"backup_files_count\": "
       << count_files_in_dir(FACTORY_BACKUP_DIR);
  json << ",\n    \"vienna_tar_exists\": "
       << (file_exists(vienna_tar) ? "true" : "false");
  json << ",\n    \"etc_tar_exists\": "
       << (file_exists(etc_tar) ? "true" : "false");
  if (file_exists(vienna_tar))
    json << ",\n    \"vienna_tar_size\": " << get_file_size(vienna_tar);
  if (file_exists(etc_tar))
    json << ",\n    \"etc_tar_size\": " << get_file_size(etc_tar);
  json << "\n  },\n";
}

static void append_system_uptime(std::ostringstream &json) {
  std::string uptime_str =
      exec_command("cat /proc/uptime 2>/dev/null | awk '{print $1}'");
  if (uptime_str.empty())
    return;
  double uptime_seconds = atof(uptime_str.c_str());
  json << "    \"uptime_seconds\": " << uptime_seconds << ",\n";
  json << R"(    "uptime_formatted": ")"
       << json_escape(format_uptime(uptime_seconds)) << R"(",)" << ",\n";
}

static void append_system_cpu_temp_firmware(std::ostringstream &json) {
  int cpu_usage = get_cpu_usage();
  if (cpu_usage >= 0)
    json << "    \"cpu_usage_percent\": " << cpu_usage << ",\n";

  int temperature = get_temperature();
  if (temperature >= 0)
    json << "    \"temperature_celsius\": " << temperature << ",\n";

  // Converted to a proper raw string literal R"(...)"
  json << R"(    "firmware_version": ")" << json_escape(get_firmware_version())
       << R"(",)" << "\n";
}

struct MemInfo {
  long long total_kb = -1;
  long long available_kb = -1;
  long long free_kb = -1;
};

static MemInfo parse_meminfo(const std::string &meminfo) {
  MemInfo out;
  std::istringstream iss(meminfo);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.find("MemTotal:") == 0)
      sscanf(line.c_str(), "MemTotal: %lld kB", &out.total_kb);
    else if (line.find("MemAvailable:") == 0)
      sscanf(line.c_str(), "MemAvailable: %lld kB", &out.available_kb);
    else if (line.find("MemFree:") == 0)
      sscanf(line.c_str(), "MemFree: %lld kB", &out.free_kb);
  }
  return out;
}

static void append_system_memory(std::ostringstream &json, const MemInfo &mem) {
  if (mem.total_kb <= 0)
    return;
  json << "    \"memory\": {\n";
  json << "      \"total_kb\": " << mem.total_kb << ",\n";
  json << "      \"total_mb\": " << (mem.total_kb / 1024) << ",\n";
  if (mem.available_kb > 0) {
    json << "      \"available_kb\": " << mem.available_kb << ",\n";
    json << "      \"available_mb\": " << (mem.available_kb / 1024) << ",\n";
    json << "      \"used_kb\": " << (mem.total_kb - mem.available_kb) << ",\n";
    json << "      \"used_mb\": " << ((mem.total_kb - mem.available_kb) / 1024)
         << ",\n";
    json << "      \"usage_percent\": "
         << (100.0 * (mem.total_kb - mem.available_kb) / mem.total_kb) << ",\n";
  }
  if (mem.free_kb > 0) {
    json << "      \"free_kb\": " << mem.free_kb << ",\n";
    json << "      \"free_mb\": " << (mem.free_kb / 1024);
  }
  json << "\n    },\n";
}

static void append_system_disk(std::ostringstream &json) {
  long long disk_total = -1;
  long long disk_available = -1;
  long long disk_used = -1;
  get_disk_space("/mnt/flash", &disk_total, &disk_available, &disk_used);
  if (disk_total <= 0)
    return;
  json << "    \"disk\": {\n";
  json << "      \"mount_point\": \"/mnt/flash\",\n";
  json << "      \"total_bytes\": " << disk_total << ",\n";
  json << "      \"total_mb\": " << (disk_total / (1024 * 1024)) << ",\n";
  json << "      \"total_gb\": " << (disk_total / (1024 * 1024 * 1024))
       << ",\n";
  json << "      \"available_bytes\": " << disk_available << ",\n";
  json << "      \"available_mb\": " << (disk_available / (1024 * 1024))
       << ",\n";
  json << "      \"available_gb\": " << (disk_available / (1024 * 1024 * 1024))
       << ",\n";
  json << "      \"used_bytes\": " << disk_used << ",\n";
  json << "      \"used_mb\": " << (disk_used / (1024 * 1024)) << ",\n";
  json << "      \"used_gb\": " << (disk_used / (1024 * 1024 * 1024)) << ",\n";
  json << "      \"usage_percent\": " << (100.0 * disk_used / disk_total)
       << "\n";
  json << "    },\n";
}

static void append_system_loadavg(std::ostringstream &json) {
  std::string loadavg = read_file_content("/proc/loadavg");
  double load1;
  double load5;
  double load15;
  if (loadavg.empty() ||
      sscanf(loadavg.c_str(), "%lf %lf %lf", &load1, &load5, &load15) != 3)
    return;
  json << "    \"load_average\": {\n";
  json << "      \"1min\": " << load1 << ",\n";
  json << "      \"5min\": " << load5 << ",\n";
  json << "      \"15min\": " << load15 << "\n";
  json << "    },\n";
}

static int count_cpu_from_cpuinfo(const std::string &cpuinfo) {
  int cpu_count = 0;
  std::istringstream iss(cpuinfo);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.find("processor") == 0)
      cpu_count++;
  }
  return cpu_count;
}

static void append_system_cpu_count(std::ostringstream &json) {
  std::string cpuinfo = read_file_content("/proc/cpuinfo");
  int cpu_count = count_cpu_from_cpuinfo(cpuinfo);
  if (cpu_count > 0)
    json << "    \"cpu_count\": " << cpu_count << ",\n";
}

static void append_system_processes(std::ostringstream &json) {
  // Using std::array for fixed-size compile-time lists
  static const std::array<std::string, 13> processes = {
      "m5s_mw_server",    "m5s_http_server", "onvif_netlink_monitor",
      "multionvifserver", "rtsps",           "streamer",
      "violet",           "daemon_gyro",     "wpa_supplicant",
      "xinetd",           "syslogd",         "klogd"};

  json << "    \"processes\": {\n";

  for (auto it = processes.begin(); it != processes.end(); ++it) {
    bool is_running = is_process_running(*it);

    json << "      \"" << *it << "\": " << (is_running ? "true" : "false");

    // Add a comma if it's not the last element
    if (std::next(it) != processes.end()) {
      json << ",";
    }
    json << "\n";
  }

  json << "    },\n";
}

static std::string build_metrics_json() {
  std::ostringstream json;
  json << "{\n";
  append_factory_reset(json);
  append_configuration(json);
  append_factory_backup(json);
  json << "  \"system\": {\n";
  append_system_uptime(json);
  append_system_cpu_temp_firmware(json);
  MemInfo mem = parse_meminfo(read_file_content("/proc/meminfo"));
  append_system_memory(json, mem);
  append_system_disk(json);
  append_system_loadavg(json);
  append_system_cpu_count(json);
  append_system_processes(json);
  std::string json_str = json.str();
  size_t last_non_ws = json_str.find_last_not_of(" \t\n\r");
  if (last_non_ws != std::string::npos && json_str[last_non_ws] == ',')
    json_str.erase(last_non_ws, 1);
  json_str += "\n  }\n}\n";
  return json_str;
}

// Handler for /api/metrics endpoint
int handle_metrics_api(struct mg_connection *conn,
                       const struct mg_request_info *ri) {
  (void)ri;
  std::string json_str = build_metrics_json();
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %zu\r\n\r\n%s",
            json_str.length(), json_str.c_str());
  return 1;
}