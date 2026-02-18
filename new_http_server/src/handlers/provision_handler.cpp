#include "provision_handler.h"
#include "http_utils.h"
#include <cstring>
#include <string>
#include <cstdio>

int ProvisionHandler::handle_provision_device(struct mg_connection *conn, const struct mg_request_info *ri)
{
    if (strcmp(ri->request_method, "POST") != 0)
    {
        send_json_response(conn, 405, "Method Not Allowed", "{\"error\": \"Method not allowed\"}\n");
        return 1;
    }

    std::string post_data(1024, '\0');
    int data_len = mg_read(conn, &post_data[0], post_data.size() - 1);
    if (data_len <= 0)
    {
        send_json_response(conn, 400, "Bad Request", "{\"error\": \"No data received\"}\n");
        return 1;
    }
    post_data[data_len] = '\0';
    printf("Received provisioning data (length: %d): %s\n", data_len, post_data.c_str());

    std::string mac_address = extract_json_string_field(post_data, "mac_address");
    std::string serial_number = extract_json_string_field(post_data, "serial_number");
    std::string manufacture_date = extract_json_string_field(post_data, "manufacture_date");

    if (mac_address.empty() || serial_number.empty() || manufacture_date.empty())
    {
        send_json_response(conn, 400, "Bad Request",
                                     "{\"status\": \"error\", \"message\": \"Missing required fields\"}\n");
        return 1;
    }
    if (!is_valid_mac_address(mac_address))
    {
        send_json_response(conn, 400, "Bad Request",
                                     "{\"status\": \"error\", \"message\": \"Invalid MAC address format\"}\n");
        return 1;
    }
    if (!is_valid_manufacture_date(manufacture_date))
    {
        send_json_response(conn, 400, "Bad Request",
                                     "{\"status\": \"error\", \"message\": \"Invalid manufacture date format\"}\n");
        return 1;
    }

    int8_t result = provisioning_mode(mac_address.c_str(), serial_number.c_str(), manufacture_date.c_str());

    if (result > 0)
        send_json_response(conn, 200, "OK",
                                     "{\"status\": \"success\", \"message\": \"Provisioning completed successfully\"}\n");
    else if (result == 0)
        send_json_response(conn, 500, "Internal Server Error",
                                     "{\"status\": \"error\", \"message\": \"Mac address is already Provisioned\"}\n");
    else
        send_json_response(conn, 500, "Internal Server Error",
                                     "{\"status\": \"error\", \"message\": \"Failed to execute provisioning command\"}\n");

    return 1;
}
