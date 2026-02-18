#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <string>
#include <vector>
#include <cstdint>
#include "civetweb.h"
#include "server_config.h"

// Forward declaration if needed, or include session_manager.h if strictly coupled
// For utils, we might just need string manipulation.

std::string read_file_content(const std::string &path);
std::string get_session_token(struct mg_connection *conn);
std::string extract_json_string_field(const std::string &post_data, const char *key);
bool is_valid_mac_address(const std::string &mac);
bool is_valid_manufacture_date(const std::string &date);

// Helper to send JSON response
void send_json_response(struct mg_connection *conn, int status_code, const char *status_reason, const char *json_body);

// Cookie helpers
std::string generate_cookie_header(const std::string &session_token);
std::string generate_expired_cookie_header();

// Helper for prefix check (from original net.cpp)
bool response_body_prefix_check(const uint8_t* byte_array, int array_len, const uint8_t* prefix, int prefix_len);

#endif // HTTP_UTILS_H
