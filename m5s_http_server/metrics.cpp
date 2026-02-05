#include "metrics.h"
#include "civetweb.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <unistd.h>
#include <cctype>

#define CONFIG_DIR "/mnt/flash/vienna/m5s_config"
#define SNAPSHOT_FILE "/mnt/flash/vienna/default_snapshot.txt"
#define FACTORY_BACKUP_DIR "/mnt/flash/vienna/factory_backup"
#define FACTORY_RESET_STATUS_FILE "/mnt/flash/vienna/m5s_config/factory_reset_status"
#define VERSION_FILE "/mnt/flash/jffs2_version"

// Helper function to read file content
static std::string read_file_content(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return "";
    
    std::string content;
    std::string line;
    while (std::getline(file, line))
    {
        content += line;
        if (!file.eof())
            content += "\n";
    }
    file.close();
    
    // Remove trailing newline if present
    if (!content.empty() && content.back() == '\n')
        content.pop_back();
    
    return content;
}

// Helper function to check if file exists
static bool file_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

// Helper function to check if directory exists
static bool dir_exists(const char *path)
{
    DIR *dir = opendir(path);
    if (dir)
    {
        closedir(dir);
        return true;
    }
    return false;
}

// Helper function to get file size
static long long get_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0)
        return st.st_size;
    return -1;
}

// Helper function to execute command and get output
static std::string exec_command(const char *cmd)
{
    FILE *pipe = popen(cmd, "r");
    if (!pipe)
        return "";
    
    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }
    pclose(pipe);
    
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    
    return result;
}

// Helper function to get disk space info
static void get_disk_space(const char *path, long long *total, long long *available, long long *used)
{
    struct statvfs st;
    if (statvfs(path, &st) == 0)
    {
        *total = (long long)st.f_blocks * st.f_frsize;
        *available = (long long)st.f_bavail * st.f_frsize;
        *used = *total - (long long)st.f_bfree * st.f_frsize;
    }
    else
    {
        *total = -1;
        *available = -1;
        *used = -1;
    }
}

// Helper function to escape JSON string
static std::string json_escape(const std::string &str)
{
    std::string escaped;
    for (char c : str)
    {
        switch (c)
        {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (c < 0x20 || c > 0x7E)
                {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    escaped += buf;
                }
                else
                {
                    escaped += c;
                }
                break;
        }
    }
    return escaped;
}

// Count files in directory
static int count_files_in_dir(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
        return -1;
    
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_name[0] != '.')
        {
            count++;
        }
    }
    closedir(dir);
    return count;
}

// Get CPU usage percentage
static int get_cpu_usage()
{
    FILE *f;
    char buf[256];
    unsigned long u1, n1, s1, i1, uw1, irq1, si1, st1;
    unsigned long u2, n2, s2, i2, uw2, irq2, si2, st2;

    f = fopen("/proc/stat", "r");
    if (!f)
        return -1;
    
    fgets(buf, sizeof(buf), f);
    sscanf(buf, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
           &u1, &n1, &s1, &i1, &uw1, &irq1, &si1, &st1);
    fclose(f);
    
    sleep(1);

    f = fopen("/proc/stat", "r");
    if (!f)
        return -1;
    
    fgets(buf, sizeof(buf), f);
    sscanf(buf, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
           &u2, &n2, &s2, &i2, &uw2, &irq2, &si2, &st2);
    fclose(f);

    unsigned long t1 = u1 + n1 + s1 + i1 + uw1 + irq1 + si1 + st1;
    unsigned long t2 = u2 + n2 + s2 + i2 + uw2 + irq2 + si2 + st2;
    unsigned long idle1 = i1, idle2 = i2;
    unsigned long diff_total = t2 - t1;
    unsigned long diff_idle = idle2 - idle1;

    if (diff_total == 0)
        return -1;

    int cpu = 100 * (diff_total - diff_idle) / diff_total;
    return cpu;
}

// Get temperature in Celsius
static int get_temperature()
{
    FILE *tf = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!tf)
        return -1;
    
    int raw;
    if (fscanf(tf, "%d", &raw) != 1)
    {
        fclose(tf);
        return -1;
    }
    fclose(tf);
    
    return raw ; // Convert from millidegrees to degrees
}

// Get firmware version
static std::string get_firmware_version()
{
    std::string version = read_file_content(VERSION_FILE);
    if (version.empty())
        return "UNKNOWN";
    
    // Remove trailing newline if present
    if (!version.empty() && version.back() == '\n')
        version.pop_back();
    
    return version;
}

// Extract basename from path
static const char* get_basename(const char* path)
{
    const char* basename = strrchr(path, '/');
    return basename ? basename + 1 : path;
}

// Check if process is running
static bool is_process_running(const char *name)
{
    DIR *dp = opendir("/proc");
    if (!dp)
        return false;

    struct dirent *e;
    while ((e = readdir(dp)) != nullptr)
    {
        if (!isdigit(e->d_name[0]))
            continue;

        // First try /proc/<pid>/comm (may be truncated to 15 chars)
        char path[256];
        snprintf(path, sizeof(path), "/proc/%s/comm", e->d_name);
        FILE *f = fopen(path, "r");
        if (f)
        {
            char comm[128];
            if (fgets(comm, sizeof(comm), f))
            {
                comm[strcspn(comm, "\n")] = 0;
                // Check exact match
                if (strcmp(comm, name) == 0)
                {
                    fclose(f);
                    closedir(dp);
                    return true;
                }
                // Check if comm matches the beginning of name (for truncated names)
                if (strlen(comm) == 15 && strncmp(name, comm, 15) == 0)
                {
                    fclose(f);
                    closedir(dp);
                    return true;
                }
            }
            fclose(f);
        }

        // Also check /proc/<pid>/cmdline for full command line
        snprintf(path, sizeof(path), "/proc/%s/cmdline", e->d_name);
        f = fopen(path, "r");
        if (f)
        {
            char cmdline[512];
            size_t len = fread(cmdline, 1, sizeof(cmdline) - 1, f);
            fclose(f);
            
            if (len > 0)
            {
                cmdline[len] = '\0';
                // Extract basename from cmdline (first argument is the executable path)
                const char* basename = get_basename(cmdline);
                // Check if name matches basename
                if (strcmp(basename, name) == 0)
                {
                    closedir(dp);
                    return true;
                }
                // Also check if name appears anywhere in cmdline (for full path matches)
                if (strstr(cmdline, name) != nullptr)
                {
                    closedir(dp);
                    return true;
                }
            }
        }
    }
    closedir(dp);
    return false;
}

// Format uptime string
static std::string format_uptime(double seconds)
{
    if (seconds < 0)
        return "N/A";
    
    int days = (int)(seconds / 86400);
    int hours = (int)((seconds - days * 86400) / 3600);
    int minutes = (int)((seconds - days * 86400 - hours * 3600) / 60);
    
    char buf[128];
    if (days > 0)
    {
        snprintf(buf, sizeof(buf), "%d days, %d hours, %d minutes", days, hours, minutes);
    }
    else if (hours > 0)
    {
        snprintf(buf, sizeof(buf), "%d hours, %d minutes", hours, minutes);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%d minutes", minutes);
    }
    
    return std::string(buf);
}

// Handler for /api/metrics endpoint
int handle_metrics_api(struct mg_connection *conn, const struct mg_request_info *ri)
{
    (void)ri; // Unused parameter
    
    // Build JSON response
    std::ostringstream json;
    json << "{\n";
    
    // Factory reset status
    json << "  \"factory_reset\": {\n";
    std::string reset_status = read_file_content(FACTORY_RESET_STATUS_FILE);
    if (reset_status.empty())
        reset_status = "unknown";
    json << "    \"status\": \"" << json_escape(reset_status) << "\",\n";
    json << "    \"status_file_exists\": " << (file_exists(FACTORY_RESET_STATUS_FILE) ? "true" : "false") << "\n";
    json << "  },\n";
    
    // Configuration directory
    json << "  \"configuration\": {\n";
    json << "    \"config_dir\": \"" << CONFIG_DIR << "\",\n";
    json << "    \"config_dir_exists\": " << (dir_exists(CONFIG_DIR) ? "true" : "false") << ",\n";
    if (dir_exists(CONFIG_DIR))
    {
        json << "    \"config_files_count\": " << count_files_in_dir(CONFIG_DIR) << ",\n";
    }
    json << "    \"snapshot_file\": \"" << SNAPSHOT_FILE << "\",\n";
    json << "    \"snapshot_file_exists\": " << (file_exists(SNAPSHOT_FILE) ? "true" : "false");
    if (file_exists(SNAPSHOT_FILE))
    {
        long long snapshot_size = get_file_size(SNAPSHOT_FILE);
        json << ",\n    \"snapshot_file_size\": " << snapshot_size;
    }
    json << "\n  },\n";
    
    // Factory backup
    json << "  \"factory_backup\": {\n";
    json << "    \"backup_dir\": \"" << FACTORY_BACKUP_DIR << "\",\n";
    json << "    \"backup_dir_exists\": " << (dir_exists(FACTORY_BACKUP_DIR) ? "true" : "false");
    if (dir_exists(FACTORY_BACKUP_DIR))
    {
        json << ",\n    \"backup_files_count\": " << count_files_in_dir(FACTORY_BACKUP_DIR);
        
        // Check for specific backup files
        std::string vienna_tar = std::string(FACTORY_BACKUP_DIR) + "/vienna.tar";
        std::string etc_tar = std::string(FACTORY_BACKUP_DIR) + "/etc.tar";
        json << ",\n    \"vienna_tar_exists\": " << (file_exists(vienna_tar.c_str()) ? "true" : "false");
        json << ",\n    \"etc_tar_exists\": " << (file_exists(etc_tar.c_str()) ? "true" : "false");
        
        if (file_exists(vienna_tar.c_str()))
        {
            json << ",\n    \"vienna_tar_size\": " << get_file_size(vienna_tar.c_str());
        }
        if (file_exists(etc_tar.c_str()))
        {
            json << ",\n    \"etc_tar_size\": " << get_file_size(etc_tar.c_str());
        }
    }
    json << "\n  },\n";
    
    // System information
    json << "  \"system\": {\n";
    
    // Uptime
    std::string uptime_str = exec_command("cat /proc/uptime 2>/dev/null | awk '{print $1}'");
    double uptime_seconds = -1;
    if (!uptime_str.empty())
    {
        uptime_seconds = atof(uptime_str.c_str());
        json << "    \"uptime_seconds\": " << uptime_seconds << ",\n";
        json << "    \"uptime_formatted\": \"" << json_escape(format_uptime(uptime_seconds)) << "\",\n";
    }
    
    // CPU usage
    int cpu_usage = get_cpu_usage();
    if (cpu_usage >= 0)
    {
        json << "    \"cpu_usage_percent\": " << cpu_usage << ",\n";
    }
    
    // Temperature
    int temperature = get_temperature();
    if (temperature >= 0)
    {
        json << "    \"temperature_celsius\": " << temperature << ",\n";
    }
    
    // Firmware version
    std::string firmware_version = get_firmware_version();
    json << "    \"firmware_version\": \"" << json_escape(firmware_version) << "\",\n";
    
    // Memory info
    std::string meminfo = read_file_content("/proc/meminfo");
    if (!meminfo.empty())
    {
        // Parse meminfo for total and available memory
        std::istringstream iss(meminfo);
        std::string line;
        long long mem_total = -1, mem_available = -1, mem_free = -1;
        
        while (std::getline(iss, line))
        {
            if (line.find("MemTotal:") == 0)
            {
                sscanf(line.c_str(), "MemTotal: %lld kB", &mem_total);
            }
            else if (line.find("MemAvailable:") == 0)
            {
                sscanf(line.c_str(), "MemAvailable: %lld kB", &mem_available);
            }
            else if (line.find("MemFree:") == 0)
            {
                sscanf(line.c_str(), "MemFree: %lld kB", &mem_free);
            }
        }
        
        if (mem_total > 0)
        {
            json << "    \"memory\": {\n";
            json << "      \"total_kb\": " << mem_total << ",\n";
            json << "      \"total_mb\": " << (mem_total / 1024) << ",\n";
            if (mem_available > 0)
            {
                json << "      \"available_kb\": " << mem_available << ",\n";
                json << "      \"available_mb\": " << (mem_available / 1024) << ",\n";
                json << "      \"used_kb\": " << (mem_total - mem_available) << ",\n";
                json << "      \"used_mb\": " << ((mem_total - mem_available) / 1024) << ",\n";
                json << "      \"usage_percent\": " << (100.0 * (mem_total - mem_available) / mem_total) << ",\n";
            }
            if (mem_free > 0)
            {
                json << "      \"free_kb\": " << mem_free << ",\n";
                json << "      \"free_mb\": " << (mem_free / 1024);
            }
            json << "\n    },\n";
        }
    }
    
    // Disk space for /mnt/flash
    long long disk_total = -1, disk_available = -1, disk_used = -1;
    get_disk_space("/mnt/flash", &disk_total, &disk_available, &disk_used);
    if (disk_total > 0)
    {
        json << "    \"disk\": {\n";
        json << "      \"mount_point\": \"/mnt/flash\",\n";
        json << "      \"total_bytes\": " << disk_total << ",\n";
        json << "      \"total_mb\": " << (disk_total / (1024 * 1024)) << ",\n";
        json << "      \"total_gb\": " << (disk_total / (1024 * 1024 * 1024)) << ",\n";
        json << "      \"available_bytes\": " << disk_available << ",\n";
        json << "      \"available_mb\": " << (disk_available / (1024 * 1024)) << ",\n";
        json << "      \"available_gb\": " << (disk_available / (1024 * 1024 * 1024)) << ",\n";
        json << "      \"used_bytes\": " << disk_used << ",\n";
        json << "      \"used_mb\": " << (disk_used / (1024 * 1024)) << ",\n";
        json << "      \"used_gb\": " << (disk_used / (1024 * 1024 * 1024)) << ",\n";
        json << "      \"usage_percent\": " << (100.0 * disk_used / disk_total) << "\n";
        json << "    },\n";
    }
    
    // Load average
    std::string loadavg = read_file_content("/proc/loadavg");
    if (!loadavg.empty())
    {
        double load1, load5, load15;
        if (sscanf(loadavg.c_str(), "%lf %lf %lf", &load1, &load5, &load15) == 3)
        {
            json << "    \"load_average\": {\n";
            json << "      \"1min\": " << load1 << ",\n";
            json << "      \"5min\": " << load5 << ",\n";
            json << "      \"15min\": " << load15 << "\n";
            json << "    },\n";
        }
    }
    
    // CPU info
    std::string cpuinfo = read_file_content("/proc/cpuinfo");
    if (!cpuinfo.empty())
    {
        int cpu_count = 0;
        std::istringstream iss(cpuinfo);
        std::string line;
        while (std::getline(iss, line))
        {
            if (line.find("processor") == 0)
                cpu_count++;
        }
        if (cpu_count > 0)
        {
            json << "    \"cpu_count\": " << cpu_count << ",\n";
        }
    }
    
    // Process status
    json << "    \"processes\": {\n";
    const char* processes[] = {
        "m5s_mw_server",
        "m5s_http_server",
        "onvif_netlink_monitor",
        "multionvifserver",
        "rtsps",
        "streamer",
        "violet",
        "daemon_gyro",
        "wpa_supplicant",
        "xinetd",
        "syslogd",
        "klogd"
    };
    int num_processes = sizeof(processes) / sizeof(processes[0]);
    for (int i = 0; i < num_processes; i++)
    {
        bool running = is_process_running(processes[i]);
        json << "      \"" << processes[i] << "\": " << (running ? "true" : "false");
        if (i < num_processes - 1)
            json << ",";
        json << "\n";
    }
    json << "    },\n";
    
    // Remove trailing comma and newline from system section
    std::string json_str = json.str();
    // Find last non-whitespace character
    size_t last_non_ws = json_str.find_last_not_of(" \t\n\r");
    if (last_non_ws != std::string::npos && json_str[last_non_ws] == ',')
    {
        json_str.erase(last_non_ws, 1);
    }
    json_str += "\n  }\n";
    
    json_str += "}\n";
    
    // Send response
    mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Content-Length: %zu\r\n\r\n%s",
              json_str.length(), json_str.c_str());
    
    return 1;
}
