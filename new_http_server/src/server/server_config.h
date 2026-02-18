#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string>
#include <cstdint>

namespace ServerConfig {

constexpr size_t MAX_TOKEN_SIZE = 64;
constexpr double MAX_FILE_SIZE_MB = 2.2;
constexpr auto MAX_FILE_SIZE_BYTES = static_cast<long long>(MAX_FILE_SIZE_MB * 1024 * 1024); // 2.2 MB in bytes

constexpr char MISC_SOCK_PATH[] = "/tmp/misc_change.sock";
constexpr char IR_SOCK_PATH[] = "/tmp/ir_change.sock";
constexpr char HTTP_PORT[] = "80";

struct Settings {
    bool log_enabled;
    int log_level;
    long brightness;
    std::string device_name;
    std::string eth_ipaddress;
    std::string wifi_hotspot_ipaddress;
    std::string wifi_client_ipaddress;
};

// Global settings instance (declaration)
extern const Settings g_settings;

} // namespace ServerConfig

#endif // SERVER_CONFIG_H
