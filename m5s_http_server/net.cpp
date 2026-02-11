// Copyright (c) 2023 Cesanta Software Limited
// All rights reserved

#include "net.h"
#include "session_manager.h"
#include "metrics.h"
#include <cstdint>
#include <string>
#include <fstream>
#include <streambuf>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <set>
#include <atomic>
#include <vector>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <memory>
#include <memory>
#include "include/fw.h"
#include "include/motocam_api_l1.h"

// CivetWeb Header (Ensure this path is correct for your project structure)
#include "civetweb.h" 

extern "C"
{
    int8_t do_processing(const uint8_t *req_bytes, const uint8_t req_bytes_size,
                         uint8_t **res_bytes, uint8_t *res_bytes_size);
}

namespace {

constexpr size_t MAX_TOKEN_SIZE = 64;
constexpr double MAX_FILE_SIZE_MB = 2.2;
constexpr long long MAX_FILE_SIZE_BYTES =
    static_cast<long long>(MAX_FILE_SIZE_MB * 1024 * 1024); // 2.2 MB in bytes

constexpr char MISC_SOCK_PATH[] = "/tmp/misc_change.sock";
constexpr char IR_SOCK_PATH[] = "/tmp/ir_change.sock";

} // anonymous namespace

const char current_login_pin[18] = {0};

// constexpr char valid_pin[] = current_login_pin;

constexpr char s_unauthorized_response[] =
    "{\"error\": \"Unauthorized\", \"message\": \"Missing or invalid session\"}\n";

// Settings
struct settings
{
    bool log_enabled;
    int log_level;
    long brightness;
    char device_name[50];
    char eth_ipaddress[50];
    char wifi_hotspot_ipaddress[50];
    char wifi_client_ipaddress[50];
};

static const struct settings s_settings = {true, 1, 57, {0}, {0}, {0}, {0}};

// Global session manager and web server (owned)
const static std::shared_ptr<SessionManager> g_session_manager;

// Forward declaration of WebServer to use in global context
class WebServer;
const static std::unique_ptr<WebServer> g_web_server;



static std::string read_file_content(const std::string &path)
{
    std::ifstream t(path);
    if (!t.is_open())
        return "";
    std::string str;

    t.seekg(0, std::ios::end);
    str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(t)),
               std::istreambuf_iterator<char>());
    return str;
}

// Get session token from cookies
static std::string get_session_token(struct mg_connection *conn)
{
    const char *cookie_header = mg_get_header(conn, "Cookie");
    if (!cookie_header)
        return "";

    std::string cookies(cookie_header);
    size_t pos = cookies.find("session=");
    if (pos == std::string::npos)
        return "";

    pos += 8; // length of "session="
    size_t end = cookies.find(';', pos);
    if (end == std::string::npos)
        end = cookies.length();

    return cookies.substr(pos, end - pos);
}

static bool is_authenticated(struct mg_connection *conn, const struct mg_request_info *ri)
{
    // Skip authentication check for login endpoint
    if (strcmp(ri->local_uri, "/api/login") == 0)
    {
        return true;
    }

    std::string session_token = get_session_token(conn);
    if (session_token.empty())
    {
        return false;
    }

    auto manager = g_session_manager;  // keep shared ownership during this call
    if (!manager)
    {
        return false;
    }

    SessionContext *ctx = nullptr;
    session_validate(manager.get(), session_token.c_str(), &ctx);

    return ctx != nullptr;
}

// Proxy request handler using CivetWeb client connection
static void handle_proxy_request(struct mg_connection *conn, const struct mg_request_info *ri,
                                 const char *backend_url, const char *target_path)
{
    // Parse backend URL to extract host and port
    std::string url(backend_url);
    std::string host;
    int port = 80;

    // Remove protocol if present
    if (url.substr(0, 7) == "http://")
    {
        url = url.substr(7);
    }
    else if (url.substr(0, 8) == "https://")
    {
        url = url.substr(8);
        port = 443;
    }

    // Extract host and port
    size_t colon_pos = url.find(':');
    if (colon_pos != std::string::npos)
    {
        host = url.substr(0, colon_pos);
        port = std::stoi(url.substr(colon_pos + 1));
    }
    else
    {
        host = url;
    }

    printf("Proxying request to: %s:%d%s\n", host.c_str(), port, target_path);

    // Create client connection to backend
    char error_buffer[256];
    struct mg_connection *backend_conn = mg_connect_client(host.c_str(), port, 0, error_buffer, sizeof(error_buffer));
    if (!backend_conn)
    {
        printf("Failed to connect to backend %s:%d: %s\n", host.c_str(), port, error_buffer);
        mg_printf(conn, "HTTP/1.1 502 Bad Gateway\r\n"
                        "Content-Type: application/json\r\n\r\n"
                        "{\"error\": \"Bad Gateway\", \"message\": \"Cannot connect to backend service\"}\n");
        return;
    }

    // Build the request to backend
    std::string request = std::string(ri->request_method) + " " + target_path;
    if (ri->query_string)
    {
        request += "?" + std::string(ri->query_string);
    }
    request += " HTTP/1.1\r\n";

    // Add headers
    request += "Host: " + host + "\r\n";
    request += "Connection: close\r\n";

    // Copy relevant headers from original request
    const char *content_length = mg_get_header(conn, "Content-Length");
    if (content_length)
    {
        request += "Content-Length: " + std::string(content_length) + "\r\n";
    }

    const char *content_type = mg_get_header(conn, "Content-Type");
    if (content_type)
    {
        request += "Content-Type: " + std::string(content_type) + "\r\n";
    }

    request += "\r\n";

    // Send request to backend
    if (mg_write(backend_conn, request.c_str(), request.length()) < 0)
    {
        printf("Failed to send request to backend\n");
        mg_close_connection(backend_conn);
        mg_printf(conn, "HTTP/1.1 502 Bad Gateway\r\n"
                        "Content-Type: application/json\r\n\r\n"
                        "{\"error\": \"Bad Gateway\", \"message\": \"Failed to send request to backend\"}\n");
        return;
    }

    // If it's a POST request, forward the body
    if (strcmp(ri->request_method, "POST") == 0)
    {
        char buffer[4096];
        int bytes_read;
        while ((bytes_read = mg_read(conn, buffer, sizeof(buffer))) > 0)
        {
            if (mg_write(backend_conn, buffer, bytes_read) < 0)
            {
                printf("Failed to forward POST body to backend\n");
                break;
            }
        }
    }

    // Get response from backend
    char response_error_buffer[256];
    int response_status = mg_get_response(backend_conn, response_error_buffer, sizeof(response_error_buffer), 5000); // 5 second timeout

    if (response_status < 0)
    {
        printf("Failed to get response from backend: %s\n", response_error_buffer);
        mg_close_connection(backend_conn);
        mg_printf(conn, "HTTP/1.1 502 Bad Gateway\r\n"
                        "Content-Type: application/json\r\n\r\n"
                        "{\"error\": \"Bad Gateway\", \"message\": \"Failed to get response from backend\"}\n");
        return;
    }

    printf("Response status from backend: %d\n", response_status);

    // Send HTTP response headers to client
    mg_printf(conn, "HTTP/1.1 %d OK\r\n", response_status);
    mg_printf(conn, "Content-Type: application/json\r\n");
    mg_printf(conn, "Access-Control-Allow-Origin: *\r\n");
    mg_printf(conn, "Connection: close\r\n");

    // Read response body from backend
    char response_buffer[4096];
    int total_bytes = 0;
    int bytes_read;

    // Read response body in chunks
    while ((bytes_read = mg_read(backend_conn, response_buffer + total_bytes, sizeof(response_buffer) - total_bytes - 1)) > 0)
    {
        total_bytes += bytes_read;
        if (total_bytes >= (int)sizeof(response_buffer) - 1)
        {
            break; // Buffer full
        }
    }

    if (total_bytes > 0)
    {
        response_buffer[total_bytes] = '\0'; // Null terminate
        printf("Response body from backend (%d bytes): %s\n", total_bytes, response_buffer);

        // Send content length header
        mg_printf(conn, "Content-Length: %d\r\n\r\n", total_bytes);

        // Forward the response body to the client
        mg_write(conn, response_buffer, total_bytes);
    }
    else
    {
        printf("No response body from backend\n");
        mg_printf(conn, "Content-Length: 0\r\n\r\n");
    }

    // Clean up
    mg_close_connection(backend_conn);
}


// Login handler using nlohmann::json
static int handle_login(struct mg_connection *conn, const struct mg_request_info *ri)
{
    printf("Login request received\n");
    if (strcmp(ri->request_method, "POST") != 0)
    {
        const char *body = "{\"error\": \"Method not allowed\"}\n";
        mg_printf(conn, "HTTP/1.1 405 Method Not Allowed\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }

    char post_data[1024];
    int data_len = mg_read(conn, post_data, sizeof(post_data) - 1);
    if (data_len > 0) { 
        post_data[data_len] = '\0';
    }
    printf("Received POST data (length: %d): %s\n", data_len, post_data);
    if (data_len <= 0)
    {
        const char *body = "{\"error\": \"No data received\"}\n";
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }
    
    if ( !mg_get_header(conn,"Content-Type") && (mg_get_header(conn, "Content-Type"), "application/octet-stream") != 0)
    {
        const char *body = "{\"error\": \"Content-Type must be application/octet-stream\"}\n";
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }

    uint8_t byteArray[256];
    int byteArrayLen = 0;

    char *saveptr;
    char *token = strtok_r(post_data, " ", &saveptr);
    while (token != nullptr && byteArrayLen < 256)
    {
        if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0)
        {
            uint8_t value = (uint8_t)strtol(token, nullptr, 16);
            byteArray[byteArrayLen++] = value;
        }
        token = strtok_r(nullptr, " ", &saveptr);
    }

    uint8_t *res_bytes = nullptr;
    uint8_t res_bytes_size = 0;
    
    do_processing(byteArray, byteArrayLen, &res_bytes, &res_bytes_size);

    // print res_bytes
    for (int i = 0; i < res_bytes_size; i++)
    {
        printf("res_bytes[%d]=0x%02X\n", i, res_bytes[i]);
    }

    if (res_bytes[4] == 0)
    {
        auto manager = g_session_manager;
        if (!manager)
        {
            const char *body = "{\"error\": \"Session manager unavailable\"}\n";
            mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %zu\r\n\r\n%s",
                      strlen(body), body);
            return 1;
        }

        SessionContext ctx;
        ctx.username = "user";
        ctx.custom_data = nullptr;
        char session_token[65];
        if (session_create(manager.get(), &ctx, session_token, sizeof(session_token)))
        {
            char *cookie_header = session_generate_cookie_header(manager.get(), session_token);
            const char *body = "{\"message\": \"Login successful\"}\n";
            mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                            "Set-Cookie: %s\r\n"
                            "Content-Type: application/json\r\n"
                            "Cache-Control: no-cache\r\n"
                            "Content-Length: %zu\r\n\r\n%s",
                      cookie_header, strlen(body), body);
            // free(cookie_header);
        }
        else
        {
            const char *body = "{\"error\": \"Session creation failed\"}\n";
            mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %zu\r\n\r\n%s",
                      strlen(body), body);
        }
    }
    else
    {
        if (res_bytes[5] == 0xFF)
        {
            const char *body = "{\"error\": \"PIN length should be 4\"}\n";
            mg_printf(conn, "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", strlen(body), body);
            return 1;
        }
        else if (res_bytes[5] == 0xFE)
        {
            const char *body = "{\"error\": \"PIN must contain only digits\"}\n";
            mg_printf(conn, "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", strlen(body), body);
            return 1;
        }
        else
        {
            const char *body = "{\"error\": \"Invalid PIN\"}\n";
            mg_printf(conn, "HTTP/1.1 403 Forbidden\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", strlen(body), body);
            return 1;
        }
    }
    return 1;
}

// Logout handler
static int handle_logout(struct mg_connection *conn, const struct mg_request_info *ri)
{
    (void)ri;
    auto manager = g_session_manager;
    if (!manager)
    {
        const char *body = "{\"error\": \"Session manager unavailable\"}\n";
        mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }

    std::string session_token = get_session_token(conn);
    if (!session_token.empty())
    {
        session_invalidate(manager.get(), session_token.c_str());
    }

    char *cookie_header = session_generate_invalidation_cookie_header(manager.get());
    const char *body = "{\"message\": \"Logged out successfully\"}\n";
    mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                    "Set-Cookie: %s\r\n"
                    "Content-Type: application/json\r\n"
                    "Cache-Control: no-cache\r\n"
                    "Content-Length: %zu\r\n\r\n%s",
              cookie_header, strlen(body), body);
    // free(cookie_header);

    return 1;
}
static int handle_force_logout_others(struct mg_connection *conn, const struct mg_request_info *ri)
{
    (void)ri;
    
    // Force logout API does not require authorization
    // It accepts POST data, sends it to do_processing, and if successful, logs out all other users
    
    if (strcmp(ri->request_method, "POST") != 0)
    {
        mg_printf(conn, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
        return 1;
    }

    char post_data[4096];
    int data_len = mg_read(conn, post_data, sizeof(post_data) - 1);
    if (data_len <= 0)
    {
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return 1;
    }
    post_data[data_len] = '\0';

    printf("Received force_logout data (length: %d): %s\n", data_len, post_data);

    uint8_t byteArray[256];
    int byteArrayLen = 0;

    char *saveptr;
    char *token = strtok_r(post_data, " ", &saveptr);
    while (token != nullptr && byteArrayLen < 256)
    {
        if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0)
        {
            uint8_t value = (uint8_t)strtol(token, nullptr, 16);
            byteArray[byteArrayLen++] = value;
        }
        token = strtok_r(nullptr, " ", &saveptr);
    }

    if (byteArrayLen == 0)
    {
        const char *body = "{\"error\": \"Invalid data format\"}\n";
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }
    uint8_t prefix[] = {2, 6, 4};
    if(!response_body_prefix_check(byteArray, byteArrayLen, prefix, 3))
    {
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return 1;
    }

    uint8_t *res_bytes = nullptr;
    uint8_t res_bytes_size = 0;
    
    do_processing(byteArray, byteArrayLen, &res_bytes, &res_bytes_size);

    // Check if do_processing responded positively (res_bytes[4] == 0 indicates success)
    if (res_bytes_size > 4 && res_bytes[4] == 0)
    {
        auto manager = g_session_manager;
        if (!manager)
        {
            const char *body = "{\"error\": \"Session manager unavailable\"}\n";
            mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %zu\r\n\r\n%s",
                      strlen(body), body);
            return 1;
        }

        // Get optional session token from request (if provided, we'll keep that user logged in)
        std::string session_token = get_session_token(conn);
        
        // Logout all other users (if session_token is empty, logout everyone)
        if (session_token.empty())
        {
            session_logout_all_others(manager.get(), nullptr);
        }
        else
        {
            session_logout_all_others(manager.get(), session_token.c_str());
        }

        const char *body = "{\"message\": \"Force logged out others successful\"}\n";
        mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Cache-Control: no-cache\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
    }
    else
    {
        const char *body = "{\"error\": \"Processing failed\"}\n";
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
    }

    return 1;
}
// Motocam API handler
static int handle_motocam_api(struct mg_connection *conn, const struct mg_request_info *ri)
{
    if (strcmp(ri->request_method, "POST") != 0)
    {
        mg_printf(conn, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
        return 1;
    }

    char post_data[4096];
    int data_len = mg_read(conn, post_data, sizeof(post_data) - 1);
    if (data_len <= 0)
    {
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return 1;
    }
    post_data[data_len] = '\0';

    printf("Received data (length: %d): %s\n", data_len, post_data);

    uint8_t byteArray[256];
    int byteArrayLen = 0;

    char *saveptr;
    char *token = strtok_r(post_data, " ", &saveptr);
    while (token != nullptr && byteArrayLen < 256)
    {
        if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0)
        {
            uint8_t value = (uint8_t)strtol(token, nullptr, 16);
            byteArray[byteArrayLen++] = value;
        }
        token = strtok_r(nullptr, " ", &saveptr);
    }

    uint8_t *res_bytes = nullptr;
    uint8_t res_bytes_size = 0;
    //int8_t ret = do_processing(byteArray, byteArrayLen, &res_bytes, &res_bytes_size);
    do_processing(byteArray, byteArrayLen, &res_bytes, &res_bytes_size);
    // int8_t ret =12;

    std::string formatted;
    for (int i = 0; i < res_bytes_size; ++i)
    {
        char buf[6];
        snprintf(buf, sizeof(buf), "0x%02X ", res_bytes[i]);
        formatted += buf;
    }

    mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/octet-stream\r\n"
                    "Content-Length: %zu\r\n\r\n%s",
              formatted.length(), formatted.c_str());

    return 1;
}

// Reset PIN API handler (same functionality as motocam_api, no authentication)
static int handle_reset_pin(struct mg_connection *conn, const struct mg_request_info *ri)
{
    if (strcmp(ri->request_method, "POST") != 0)
    {
        mg_printf(conn, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
        return 1;
    }

    char post_data[4096];
    int data_len = mg_read(conn, post_data, sizeof(post_data) - 1);
    if (data_len <= 0)
    {
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return 1;
    }
    post_data[data_len] = '\0';

    printf("Received data (length: %d): %s\n", data_len, post_data);

    uint8_t byteArray[256];
    int byteArrayLen = 0;

    char *saveptr;
    char *token = strtok_r(post_data, " ", &saveptr);
    while (token != nullptr && byteArrayLen < 256)
    {
        if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0)
        {
            uint8_t value = (uint8_t)strtol(token, nullptr, 16);
            byteArray[byteArrayLen++] = value;
        }
        token = strtok_r(nullptr, " ", &saveptr);
    }
    uint8_t prefix[] = {1, 6, 2};
    if(!response_body_prefix_check(byteArray, byteArrayLen, prefix, 3))
    {
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return 1;
    }

    uint8_t *res_bytes = nullptr;
    uint8_t res_bytes_size = 0;
    do_processing(byteArray, byteArrayLen, &res_bytes, &res_bytes_size);

    std::string formatted;
    for (int i = 0; i < res_bytes_size; ++i)
    {
        char buf[6];
        snprintf(buf, sizeof(buf), "0x%02X ", res_bytes[i]);
        formatted += buf;
    }

    mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/octet-stream\r\n"
                    "Content-Length: %zu\r\n\r\n%s",
              formatted.length(), formatted.c_str());

    return 1;
}

// Firmware version API handler (same functionality as motocam_api, no authentication)
static int handle_firmware_version(struct mg_connection *conn, const struct mg_request_info *ri)
{
    if (strcmp(ri->request_method, "POST") != 0)
    {
        mg_printf(conn, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
        return 1;
    }

    char post_data[4096];
    int data_len = mg_read(conn, post_data, sizeof(post_data) - 1);
    if (data_len <= 0)
    {
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return 1;
    }
    post_data[data_len] = '\0';

    printf("Received data (length: %d): %s\n", data_len, post_data);

    uint8_t byteArray[256];
    int byteArrayLen = 0;

    char *saveptr;
    char *token = strtok_r(post_data, " ", &saveptr);
    while (token != nullptr && byteArrayLen < 256)
    {
        if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0)
        {
            uint8_t value = (uint8_t)strtol(token, nullptr, 16);
            byteArray[byteArrayLen++] = value;
        }
        token = strtok_r(nullptr, " ", &saveptr);
    }
    uint8_t prefix[] = {2, 6, 2};
    if(!response_body_prefix_check(byteArray, byteArrayLen, prefix, 3))
    {
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n\r\n");
        return 1;
    }
    uint8_t *res_bytes = nullptr;
    uint8_t res_bytes_size = 0;
    do_processing(byteArray, byteArrayLen, &res_bytes, &res_bytes_size);

    std::string formatted;
    for (int i = 0; i < res_bytes_size; ++i)
    {
        char buf[6];
        snprintf(buf, sizeof(buf), "0x%02X ", res_bytes[i]);
        formatted += buf;
    }

    mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/octet-stream\r\n"
                    "Content-Length: %zu\r\n\r\n%s",
              formatted.length(), formatted.c_str());

    return 1;
}

// Provisioning API handler
static int handle_provision_device(struct mg_connection *conn, const struct mg_request_info *ri)
{
    if (strcmp(ri->request_method, "POST") != 0)
    {
        const char *body = "{\"error\": \"Method not allowed\"}\n";
        mg_printf(conn, "HTTP/1.1 405 Method Not Allowed\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }

    char post_data[1024];
    int data_len = mg_read(conn, post_data, sizeof(post_data) - 1);
    if (data_len <= 0)
    {
        const char *body = "{\"error\": \"No data received\"}\n";
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }
    post_data[data_len] = '\0';

    printf("Received provisioning data (length: %d): %s\n", data_len, post_data);

    // Parse JSON data (simple parsing for the required fields)
    std::string mac_address, serial_number, manufacture_date;

    // Extract MAC address
    char *mac_start = strstr(post_data, "\"mac_address\":");
    if (mac_start)
    {
        mac_start += 15; // Length of "\"mac_address\":"
        while (*mac_start == ' ' || *mac_start == '\t')
            mac_start++;
        if (*mac_start == '"')
        {
            mac_start++;
            char *mac_end = strchr(mac_start, '"');
            if (mac_end)
            {
                mac_address = std::string(mac_start, mac_end - mac_start);
            }
        }
    }

    // Extract serial number
    char *sn_start = strstr(post_data, "\"serial_number\":");
    if (sn_start)
    {
        sn_start += 17; // Length of "\"serial_number\":"
        while (*sn_start == ' ' || *sn_start == '\t')
            sn_start++;
        if (*sn_start == '"')
        {
            sn_start++;
            char *sn_end = strchr(sn_start, '"');
            if (sn_end)
            {
                serial_number = std::string(sn_start, sn_end - sn_start);
            }
        }
    }

    // Extract manufacture date
    char *date_start = strstr(post_data, "\"manufacture_date\":");
    if (date_start)
    {
        date_start += 20; // Length of "\"manufacture_date\":"
        while (*date_start == ' ' || *date_start == '\t')
            date_start++;
        if (*date_start == '"')
        {
            date_start++;
            char *date_end = strchr(date_start, '"');
            if (date_end)
            {
                manufacture_date = std::string(date_start, date_end - date_start);
            }
        }
    }

    // Validate required fields
    if (mac_address.empty() || serial_number.empty() || manufacture_date.empty())
    {
        const char *body = "{\"status\": \"error\", \"message\": \"Missing required fields\"}\n";
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }

    // Validate MAC address format (XX:XX:XX:XX:XX:XX)
    bool valid_mac = true;
    if (mac_address.length() != 17)
    {
        valid_mac = false;
    }
    else
    {
        for (int i = 0; i < 17; i++)
        {
            if (i % 3 == 2)
            {
                if (mac_address[i] != ':')
                    valid_mac = false;
            }
            else
            {
                if (!isxdigit(mac_address[i]))
                    valid_mac = false;
            }
        }
    }

    if (!valid_mac)
    {
        const char *body = "{\"status\": \"error\", \"message\": \"Invalid MAC address format\"}\n";
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }

    // Validate date format (YYYY-MM-DD)
    bool valid_date = true;
    if (manufacture_date.length() != 10)
    {
        valid_date = false;
    }
    else
    {
        if (manufacture_date[4] != '-' || manufacture_date[7] != '-')
        {
            valid_date = false;
        }
        else
        {
            // Basic year validation (reasonable range)
            int year = atoi(manufacture_date.substr(0, 4).c_str());
            if (year < 2000 || year > 2030)
                valid_date = false;
        }
    }

    if (!valid_date)
    {
        const char *body = "{\"status\": \"error\", \"message\": \"Invalid manufacture date format\"}\n";
        mg_printf(conn, "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
        return 1;
    }

    int8_t result = provisioning_mode(mac_address.c_str(), serial_number.c_str(), manufacture_date.c_str());

    if (result > 0)
    {
        // Success
        const char *body = "{\"status\": \"success\", \"message\": \"Provisioning completed successfully\"}\n";
        mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
    }
    else if (result == 0)
    {
        // Error executing the binary
        const char *body = "{\"status\": \"error\", \"message\": \"Mac address is already Provisioned\"}\n";
        mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
    }
    else
    {
        // Error executing the binary
        const char *body = "{\"status\": \"error\", \"message\": \"Failed to execute provisioning command\"}\n";
        mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(body), body);
    }

    return 1;
}

// Modified callback for handling form fields with size checking
int field_found_with_size_check(const char *key,
                                const char *filename,
                                char *path,
                                size_t pathlen,
                                void *user_data)
{
    if (filename && *filename)
    {
        printf("Field found: key=%s, filename=%s\n", key, filename);
        // Only allow files starting with ota.tar.gz
        const char *required_prefix = "ota.tar.gz";
        size_t prefix_len = strlen(required_prefix);
        if (strncmp(filename, required_prefix, prefix_len) != 0)
        {
            printf("ERROR: Filename does not start with ota.tar.gz, rejecting upload.\n");
            // Send HTTP error response if possible
            struct mg_connection *conn = (struct mg_connection *)user_data;
            if (conn)
            {
                const char *body = "{\"error\":\"Invalid file name. Must start with ota.tar.gz\"}\n";
                mg_printf(conn, "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", strlen(body), body);
            }
            return MG_FORM_FIELD_STORAGE_SKIP;
        }
        // Ensure upload directory exists
        struct stat st;
        memset(&st, 0, sizeof(st));

        const char *upload_dir = "/mnt/flash/vienna/firmware/ota";
        if (stat(upload_dir, &st) == -1)
        {
            mkdir(upload_dir, 0775);
        }
        int n = snprintf(path, pathlen, "%s/%s", upload_dir, filename);
        printf("Upload path: %s (len=%d, max=%zu)\n", path, n, pathlen);
        if (n < 0 || (size_t)n >= pathlen)
        {
            printf("ERROR: Upload path too long, fallback to /tmp\n");
            snprintf(path, pathlen, "/tmp/%s", filename);
        }
        return MG_FORM_FIELD_STORAGE_STORE;
    }
    printf("Field skipped: key=%s (no filename)\n", key);
    return MG_FORM_FIELD_STORAGE_SKIP;
}

int field_get_with_size_check(const char *key, const char *value, size_t valuelen, void *user_data)
{
    (void)user_data;
    printf("Received form field: %s = %.*s\n", key, (int)valuelen, value);
    return MG_FORM_FIELD_HANDLE_NEXT;
}

int field_store_with_size_check(const char *path, long long file_size, void *user_data)
{
    (void)user_data;
    printf("File stored: %s (%lld bytes)\n", path, file_size);
    // Check if file is in the correct directory
    if (strncmp(path, "/mnt/flash/vienna/firmware/ota/", 30) != 0)
    {
        printf("WARNING: File not stored in /mnt/flash/vienna/firmware/ota!\n");
    }

    // Check if file size exceeds the limit
    if (file_size > MAX_FILE_SIZE_BYTES)
    {
        printf("File size exceeded limit: %lld bytes > %lld bytes (%.1f MB)\n",
               file_size, MAX_FILE_SIZE_BYTES, MAX_FILE_SIZE_MB);

        // Delete the uploaded file since it's too large
        if (remove(path) == 0)
        {
            printf("Deleted oversized file: %s\n", path);
        }
        else
        {
            printf("Failed to delete oversized file: %s\n", path);
        }

        // Return error to indicate file size exceeded
        return MG_FORM_FIELD_HANDLE_ABORT;
    }

    return MG_FORM_FIELD_HANDLE_NEXT;
}

// Modified request handler for the upload endpoint with size checking
int upload_handler(struct mg_connection *conn, const struct mg_request_info *req_info)
{
    printf("Handling request: %s %s\n",
           req_info->request_method,
           req_info->request_uri);

    // Check Content-Length header first if available
    const char *content_length_str = mg_get_header(conn, "Content-Length");
    if (content_length_str)
    {
        long long content_length = strtoll(content_length_str, nullptr, 10);
        if (content_length > MAX_FILE_SIZE_BYTES)
        {
            printf("Request too large: Content-Length %lld exceeds limit %lld\n",
                   content_length, MAX_FILE_SIZE_BYTES);

            const char *error_response = "<!DOCTYPE html>"
                                         "<html><body>"
                                         "<h2>Upload Error</h2>"
                                         "<p>File size exceeds the maximum limit of 2.2 MB.</p>"
                                         "<a href=\"/\">Back to upload form</a>"
                                         "</body></html>";

            mg_printf(conn, "HTTP/1.1 413 Payload Too Large\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: %zu\r\n\r\n%s",
                      strlen(error_response), error_response);
            return 413;
        }
    }
    remove_ota_files();

    struct mg_form_data_handler fdh = {
        .field_found = field_found_with_size_check,
        .field_get = field_get_with_size_check,
        .field_store = field_store_with_size_check,
        .user_data = (void *)conn};

    // Process the form data
    int ret = mg_handle_form_request(conn, &fdh);
    if (ret < 0)
    {
        printf("Error processing upload: %d\n", ret);
        // Check if it's a file size error (our custom abort)
        if (ret == MG_FORM_FIELD_HANDLE_ABORT)
        {
            const char *error_response = "<!DOCTYPE html>"
                                         "<html><body>"
                                         "<h2>Upload Error</h2>"
                                         "<p>File size exceeds the maximum limit of 2.2 MB.</p>"
                                         "<a href=\"/\">Back to upload form</a>"
                                         "</body></html>";
            mg_printf(conn, "HTTP/1.1 413 Payload Too Large\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: %zu\r\n\r\n%s",
                      strlen(error_response), error_response);
            return 413;
        }
        else
        {
            mg_send_http_error(conn, 500, "Error processing upload");
            return 500;
        }
    }
    // If no fields were processed, assume error already sent (e.g., invalid filename)
    if (ret == 0)
    {
        printf("No valid fields processed, likely due to filename check. No further response sent.\n");
        return 400;
    }
    // Send success response
    std::string response =
    "<!DOCTYPE html>"
    "<html><body>"
    "<h2>File Upload</h2>"
    "<p>" + std::to_string(ret) +
    " fields processed. Upload successful!</p>"
    "<a href=\"/\">Back to upload form</a>"
    "</body></html>";

    size_t response_len = response.size();

    int sent = mg_send_http_ok(conn, "text/html", response_len);
    printf("mg_send_http_ok returned: %d\n", sent);

    sent = mg_write(conn, response.c_str(), response_len);
    printf("mg_write sent %d bytes for upload response\n", sent);

    return 200;
}

// Main request handler
static int request_handler(struct mg_connection *conn)
{
    const struct mg_request_info *ri = mg_get_request_info(conn);

    printf("Request: %s %s\n", ri->request_method, ri->local_uri);

    // If it's a WebSocket upgrade request, let CivetWeb handle it
    const char *upgrade = mg_get_header(conn, "Upgrade");
    if (upgrade && strcasecmp(upgrade, "websocket") == 0)
    {
        return 0;
    }

    // Authentication check for protected endpoints
    // Note: /api/force_logout does not require authorization
    bool needs_auth = (strncmp(ri->local_uri, "/api/getdeviceid", 16) == 0 ||
                       strncmp(ri->local_uri, "/api/getsignalserver", 20) == 0 ||
                       strncmp(ri->local_uri, "/api/upload", 11) == 0 ||
                       strncmp(ri->local_uri, "/api/motocam_api", 16) == 0
                       //    strncmp(ri->local_uri, "/api/provision_device", 22) == 0
                       //    strncmp(ri->local_uri, "/api/force_logout", 17) == 0  // Removed - no auth required

    );

    if (needs_auth && !is_authenticated(conn, ri))
    {
        std::string session_token = get_session_token(conn);
        int reason = -1;
        auto manager = g_session_manager;
        if (manager && !session_token.empty() && session_check_evicted(manager.get(), session_token.c_str(), &reason))
        {
             const char* reason_str = "UNKNOWN";
             switch(reason) {
                 case 0: reason_str = "EXPIRED"; break;
                 case 1: reason_str = "CAPACITY_LIMIT"; break;
                 case 2: reason_str = "CRITICAL_ACTION"; break;
                 case 3: reason_str = "MANUAL"; break;
             }
             char body[256];
             snprintf(body, sizeof(body), "{\"error\": \"Unauthorized\", \"message\": \"Session evicted\", \"reason\": \"%s\"}\n", reason_str);
             
             mg_printf(conn, "HTTP/1.1 401 Unauthorized\r\n"
                            "Content-Type: application/json\r\n"
                            "Cache-Control: no-cache\r\n"
                            "Content-Length: %zu\r\n\r\n%s",
                      strlen(body), body);
             return 1;
        }

        mg_printf(conn, "HTTP/1.1 401 Unauthorized\r\n"
                        "Content-Type: application/json\r\n"
                        "Cache-Control: no-cache\r\n"
                        "Content-Length: %zu\r\n\r\n%s",
                  strlen(s_unauthorized_response), s_unauthorized_response);
        return 1;
    }

    // Route handlers
    if (strcmp(ri->local_uri, "/api/login") == 0)
    {
        return handle_login(conn, ri);
    }
    else if (strcmp(ri->local_uri, "/api/logout") == 0)
    {
        return handle_logout(conn, ri);
    }
    else if (strcmp(ri->local_uri, "/api/motocam_api") == 0)
    {
        return handle_motocam_api(conn, ri);
    }
    else if (strcmp(ri->local_uri, "/api/reset_pin") == 0)
    {
        return handle_reset_pin(conn, ri);
    }
    else if (strcmp(ri->local_uri, "/api/firmware_version") == 0)
    {
        return handle_firmware_version(conn, ri);
    }
    else if (strcmp(ri->local_uri, "/api/provision_device") == 0)
    {
        return handle_provision_device(conn, ri);
    }
    else if (strcmp(ri->local_uri, "/api/upload") == 0)
    {
        // return handle_upload(conn, ri);
        return upload_handler(conn, ri);
        return 1;
    }
    else if(strcmp(ri->local_uri, "/api/force_logout") == 0)
    {
        return handle_force_logout_others(conn, ri);
    }
    else if (strcmp(ri->local_uri, "/api/metrics") == 0)
    {
        return handle_metrics_api(conn, ri);
    }
    else if (strncmp(ri->local_uri, "/getdeviceid", 12) == 0)
    {
        handle_proxy_request(conn, ri, "http://127.0.0.1:8083", "/getdeviceid");
        return 1;
    }
    else if (strncmp(ri->local_uri, "/getsignalserver", 16) == 0)
    {
        handle_proxy_request(conn, ri, "http://127.0.0.1:8083", "/getsignalserver");
        return 1;
    }

    // Static file serving and SPA fallback
    std::string file_path = "dist" + std::string(ri->local_uri);
    struct stat st;

    if (stat(file_path.c_str(), &st) == 0 && S_ISREG(st.st_mode))
    {
        // File exists, let CivetWeb serve it
        return 0; // Let CivetWeb handle static files
    }
    else
    {
        // SPA fallback - serve index.html
        std::string index_content = read_file_content("dist/index.html");
        if (!index_content.empty())
        {
            mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: %zu\r\n\r\n%s",
                      index_content.length(), index_content.c_str());
        }
        else
        {
            mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n"
                            "Content-Type: text/plain\r\n\r\n"
                            "index.html not found\n");
        }
        return 1;
    }
}

// Define websocket sub-protocols.
static const char subprotocol_bin[] = "Company.ProtoName.bin";
static const char subprotocol_json[] = "Company.ProtoName.json";
const static  char *subprotocols[] = {subprotocol_bin, subprotocol_json, nullptr};
static struct mg_websocket_subprotocols wsprot = {2, subprotocols};

/* MUST match sender structure exactly */
typedef struct {
    uint8_t old_misc;
    uint8_t new_misc;
} misc_event_t;

/* MUST match sender structure exactly */
typedef struct {
    uint8_t old_ir_brightness;
    uint8_t new_ir_brightness;
} ir_event_t;

// Helper function to convert MiscEventType enum to string
static const char* misc_event_type_to_string(MiscEventType type) {
    switch (type) {
        case MiscEventType::STARTED_STREAMING:
            return "started streaming";
        case MiscEventType::CHANGING_MISC:
            return "changing misc";
        default:
            return "unknown";
    }
}



// --- WebSocket Callbacks ---

static int ws_connect_handler(const struct mg_connection *conn, void *user_data)
{
    (void)user_data; /* unused */
    const struct mg_request_info *ri = mg_get_request_info(conn);
    printf("Client connecting with subprotocol: %s\n", ri->acceptedWebSocketSubprotocol);
    return 0; // Accept connection
}

static void ws_ready_handler(struct mg_connection *conn, void *user_data)
{
    WebServer *server = (WebServer *)user_data;
    server->add_client(conn);

    printf("Client ready and added to broadcast list\n");
}

static int ws_data_handler(struct mg_connection *conn, int opcode, char *data, size_t datasize, void *user_data)
{
    (void)user_data;
    printf("Received %lu bytes from client\n", (unsigned long)datasize);
    return 1; // Keep connection open
}

static void ws_close_handler(const struct mg_connection *conn, void *user_data)
{
    WebServer *server = (WebServer *)user_data;
    server->remove_client(conn);
    printf("Client closed connection\n");
}

// --- WebServer Implementation ---

WebServer::WebServer() : ctx(nullptr), document_root("dist"), stop_broadcast(false), misc_socket_fd(-1), ir_socket_fd(-1) {}

WebServer::~WebServer()
{
    shutdown();
}

void WebServer::add_client(struct mg_connection *conn)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.insert(conn);
}

void WebServer::remove_client(const struct mg_connection *conn)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    // Cast const away for search/removal (safe because we are just removing the pointer from set)
    clients.erase(conn);
}

void WebServer::broadcast_loop()
{
    while (!stop_broadcast)
    {
        // Read from Unix socket if available
        if (misc_socket_fd >= 0) {
            misc_event_t evt;
            ssize_t n = recv(misc_socket_fd, &evt, sizeof(evt), MSG_DONTWAIT);
            
            if (n == sizeof(evt)) {
                LOG_DEBUG("Misc event received: old_misc=%u, new_misc=%u", evt.old_misc, evt.new_misc);
                // Determine event type based on conditions
                MiscEventType event_type;
                if (evt.old_misc == 0 || evt.new_misc == 0) {
                    event_type = MiscEventType::STARTED_STREAMING;
                } else {
                    event_type = MiscEventType::CHANGING_MISC;
                }
                
                // Format as JSON
                char json_msg[256];
                snprintf(json_msg, sizeof(json_msg), "{\"old_misc\":%u,\"new_misc\":%u,\"event_type\":\"%s\"}", 
                         evt.old_misc, evt.new_misc, misc_event_type_to_string(event_type));
                
                // Broadcast to all connected clients
                std::lock_guard<std::mutex> lock(clients_mutex);
                if (!clients.empty()) {
                    for (auto conn : clients) {
                        // Check if connection is still valid/writeable is handled by mg_websocket_write internally usually,
                        // but we must ensure we don't write to a closed connection if possible.
                        // Note: CivetWeb is thread safe, but we must be careful not to write after mg_stop is called.
                        mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, json_msg, strlen(json_msg));
                    }
                }
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                // Error reading from socket (but not just "no data available")
                perror("recv from misc socket");
            }
        }

        // Read from IR brightness socket if available
        if (ir_socket_fd >= 0) {
            ir_event_t evt;
            ssize_t n = recv(ir_socket_fd, &evt, sizeof(evt), MSG_DONTWAIT);

            if (n == sizeof(evt)) {
                LOG_DEBUG("IR brightness event received: old_ir_brightness=%u, new_ir_brightness=%u", evt.old_ir_brightness, evt.new_ir_brightness);

                char json_msg[256];
                snprintf(json_msg, sizeof(json_msg), "{\"old_ir_brightness\":%u,\"new_ir_brightness\":%u,\"event_type\":\"ir brightness changed\"}",
                         evt.old_ir_brightness, evt.new_ir_brightness);

                std::lock_guard<std::mutex> lock(clients_mutex);
                if (!clients.empty()) {
                    for (auto conn : clients) {
                        mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, json_msg, strlen(json_msg));
                    }
                }
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv from ir socket");
            }
        }

        // Sleep for a short time to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

bool WebServer::init()
{
    // Initialize CivetWeb library
    mg_init_library(0);

    // CivetWeb options - HTTP only for testing
    const char *options[] = {
        // "run_as_user","root"
        "listening_ports", "80", // HTTP and WS on same port
        // "ssl_protocol_version", "4",
        // "ssl_cipher_list", "ECDH+AESGCM+AES256:!aNULL:!MD5:!DSS"
        // "strict_transport_security_max_age", "15552000",

        // "authentication_domain", "ip_camera",
        // "ssl_certificate", "server.pem",
        // "enable_auth_domain_check", "no",
        // "ssl_protocol_version", "4"
        // "ssl_private_key", "server.key",
        "document_root", document_root.c_str(),

        "num_threads", "10",
        "request_timeout_ms", "60000",
        "enable_directory_listing", "no",
        nullptr};

    printf("Starting server with document_root: %s\n", document_root.c_str());
    printf("Ports: %s \n", HTTP_PORT);

    // Check if SSL files exist
    FILE *cert_file = fopen("server.crt", "r");
    if (!cert_file)
    {
        printf("Warning: server.crt not found\n");
    }
    else
    {
        fclose(cert_file);
        printf("SSL certificate found\n");
    }

    struct mg_callbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.begin_request = request_handler;

    ctx = mg_start(&callbacks, NULL, options);

    if (!ctx)
    {
        printf("mg_start failed - check if ports are available\n");
        return false;
    }

    // Register WebSocket handlers for endpoint "/ws" (or use "/wsURL" to match reference)
    // Both HTTP and WS run on port 80. CivetWeb dispatches based on URI.
    mg_set_websocket_handler_with_subprotocols(
        ctx,
        "/wsURL", // WebSocket Endpoint
        &wsprot,
        ws_connect_handler,
        ws_ready_handler,
        ws_data_handler,
        ws_close_handler,
        this // Pass 'this' as user_data to access class members in callbacks
    );

    // Initialize Unix socket for misc events
    misc_socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (misc_socket_fd < 0) {
        perror("socket");
        printf("Warning: Failed to create Unix socket for misc events\n");
    } else {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, MISC_SOCK_PATH, sizeof(addr.sun_path) - 1);

        // Remove old socket file if exists
        unlink(MISC_SOCK_PATH);

        // Bind socket
        if (bind(misc_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(misc_socket_fd);
            misc_socket_fd = -1;
            printf("Warning: Failed to bind Unix socket for misc events\n");
        } else {
            printf("Unix socket listening on %s\n", MISC_SOCK_PATH);
        }
    }

    // Initialize Unix socket for IR brightness events
    ir_socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (ir_socket_fd < 0) {
        perror("socket");
        printf("Warning: Failed to create Unix socket for IR brightness events\n");
    } else {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, IR_SOCK_PATH, sizeof(addr.sun_path) - 1);

        unlink(IR_SOCK_PATH);

        if (bind(ir_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(ir_socket_fd);
            ir_socket_fd = -1;
            printf("Warning: Failed to bind Unix socket for IR brightness events\n");
        } else {
            printf("Unix socket listening on %s\n", IR_SOCK_PATH);
        }
    }

    // Start broadcast thread
    stop_broadcast = false;
    broadcast_thread = std::thread(&WebServer::broadcast_loop, this);

    printf("WebSocket server registered on /ws\n");

    return true;
}

void WebServer::shutdown()
{
    // Stop broadcast thread first
    stop_broadcast = true;
    if (broadcast_thread.joinable())
    {
        broadcast_thread.join();
    }

    if (ctx)
    {
        mg_stop(ctx);
        ctx = nullptr;
    }
    
    // Clear clients list
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.clear();
    
    // Clean up Unix socket
    if (misc_socket_fd >= 0) {
        close(misc_socket_fd);
        misc_socket_fd = -1;
        unlink(MISC_SOCK_PATH);
    }

    if (ir_socket_fd >= 0) {
        close(ir_socket_fd);
        ir_socket_fd = -1;
        unlink(IR_SOCK_PATH);
    }

    mg_exit_library();
}

// Public API functions
bool web_init()
{
    // Initialize session manager
    static SessionConfig cfg = {
        5,     // max_sessions
        10,    // eviction_queue_size
        0,     // session_timeout
        nullptr,  // cookie_domain
        "/",   // cookie_path
        false, // secure_cookies
        true   // http_only_cookies
    };

    std::shared_ptr<SessionManager> manager(
        session_manager_create(&cfg),
        session_manager_destroy);
    if (!manager)
    {
        printf("Failed to create session manager\n");
        return false;
    }
    g_session_manager = std::move(manager);

    // Initialize web server
    g_web_server = std::make_unique<WebServer>();
    if (!g_web_server->init())
    {
        printf("Failed to start web server\n");
        g_web_server.reset();
        return false;
    }

    printf("Web server started on port %s (HTTP & WS)\n", HTTP_PORT);
    return true;
}

void web_cleanup()
{
    if (g_web_server)
    {
        g_web_server.reset();
    }

    if (g_session_manager)
    {
        g_session_manager.reset();
    }
}