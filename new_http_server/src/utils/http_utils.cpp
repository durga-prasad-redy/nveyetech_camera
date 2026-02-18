#include "http_utils.h"
#include <fstream>
#include <streambuf>
#include <cstring>
#include <cstdlib>
#include <cctype>

std::string read_file_content(const std::string &path)
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

std::string get_session_token(struct mg_connection *conn)
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

std::string extract_json_string_field(const std::string &post_data, const char *key)
{
    std::string pattern = std::string("\"") + key + "\":";
    
    // Replace strstr with find
    size_t pos = post_data.find(pattern);

    if (pos == std::string::npos)
        return std::string();

    // Advance position past the pattern
    pos += pattern.size();

    // Skip whitespace using the string's index
    while (pos < post_data.size() && (post_data[pos] == ' ' || post_data[pos] == '\t'))
        pos++;

    // Ensure we are at the start of a quoted string
    if (pos >= post_data.size() || post_data[pos] != '"')
        return std::string();

    pos++; // Move past the opening quote

    // Find the closing quote
    size_t end_pos = post_data.find('"', pos);

    if (end_pos == std::string::npos)
        return std::string();

    return post_data.substr(pos, end_pos - pos);
}

bool is_valid_mac_address(const std::string &mac)
{
    if (mac.length() != 17)
        return false;
    for (int i = 0; i < 17; i++)
    {
        if (i % 3 == 2 && mac[i] != ':')
            return false;
        if (i % 3 != 2 && !isxdigit(static_cast<unsigned char>(mac[i])))
            return false;
    }
    return true;
}

bool is_valid_manufacture_date(const std::string &date)
{
    if (date.length() != 10 || date[4] != '-' || date[7] != '-')
        return false;
    int year = atoi(date.substr(0, 4).c_str());
    return (year >= 2000 && year <= 2030);
}

void send_json_response(struct mg_connection *conn, int status_code, const char *status_reason, const char *json_body)
{
    mg_printf(conn, "HTTP/1.1 %d %s\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %zu\r\n"
                    "Cache-Control: no-cache\r\n\r\n%s",
              status_code, status_reason, strlen(json_body), json_body);
}

std::string generate_cookie_header(const std::string &session_token)
{
    // Basic cookie generation matched to what net.cpp was doing (or intending)
    // "session=<token>; Path=/; HttpOnly" + Secure if enabled
    std::string cookie = "session=" + session_token + "; Path=/; HttpOnly";
    // Check global settings if needed, but for now hardcode standard secure defaults or simple
    return cookie;
}

std::string generate_expired_cookie_header()
{
    return "session=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly";
}

bool response_body_prefix_check(const uint8_t* byte_array, int array_len, const uint8_t* prefix, int prefix_len)
{
    if (array_len < prefix_len) return false;
    for (int i = 0; i < prefix_len; ++i) {
        if (byte_array[i] != prefix[i]) return false;
    }
    return true;
}
