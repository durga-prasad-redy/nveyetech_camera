#include "device_handler.h"
#include "http_utils.h"
#include <cstring>
#include <vector>
#include <string>
#include <cstdio>

// Declaration of external processing function (assuming it's reusable)
extern "C" {
    int8_t do_processing(const uint8_t *req_bytes, const uint8_t req_bytes_size,
                         uint8_t **res_bytes, uint8_t *res_bytes_size);
}

static int handle_common_post_processing(struct mg_connection *conn, const struct mg_request_info *ri, const uint8_t* prefix_check = nullptr, int prefix_len = 0)
{
    if (strcmp(ri->request_method, "POST") != 0)
    {
        send_json_response(conn, 405, "Method Not Allowed", "");
        return 1;
    }

    std::string post_data(4096, '\0');
    int data_len = mg_read(conn, &post_data[0], post_data.size() - 1);
    if (data_len <= 0)
    {
        send_json_response(conn, 400, "Bad Request", "");
        return 1;
    }
    post_data[data_len] = '\0';
    printf("Received data (length: %d): %s\n", data_len, post_data.c_str());

    uint8_t byteArray[256];
    int byteArrayLen = 0;

    char *saveptr;
    std::string data_copy = post_data;
    char *token = strtok_r(&data_copy[0], " ", &saveptr);
    while (token != nullptr && byteArrayLen < 256)
    {
        if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0)
        {
            auto value = (uint8_t)strtol(token, nullptr, 16);
            byteArray[byteArrayLen++] = value;
        }
        token = strtok_r(nullptr, " ", &saveptr);
    }
    
    if (prefix_check && prefix_len > 0 && !response_body_prefix_check(byteArray, byteArrayLen, prefix_check, prefix_len))
    {
        send_json_response(conn, 400, "Bad Request", "");
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

int DeviceHandler::handle_motocam_api(struct mg_connection *conn, const struct mg_request_info *ri)
{
    return handle_common_post_processing(conn, ri);
}

int DeviceHandler::handle_reset_pin(struct mg_connection *conn, const struct mg_request_info *ri)
{
    uint8_t prefix[] = {1, 6, 2};
    return handle_common_post_processing(conn, ri, prefix, 3);
}

int DeviceHandler::handle_firmware_version(struct mg_connection *conn, const struct mg_request_info *ri)
{
    uint8_t prefix[] = {2, 6, 2};
    return handle_common_post_processing(conn, ri, prefix, 3);
}
