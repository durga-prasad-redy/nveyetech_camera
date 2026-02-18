#include "proxy_handler.h"
#include <string>
#include <cstring>
#include <cstdio>

void ProxyHandler::handle_proxy_request(struct mg_connection *conn, const struct mg_request_info *ri,
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
    std::string error_buffer(256, '\0');
    struct mg_connection *backend_conn = mg_connect_client(host.c_str(), port, 0, &error_buffer[0], error_buffer.size());
    if (!backend_conn)
    {
        printf("Failed to connect to backend %s:%d: %s\n", host.c_str(), port, error_buffer.c_str());
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
        std::string buffer(4096, '\0');
        int bytes_read;
        while ((bytes_read = mg_read(conn, &buffer[0], buffer.size())) > 0)
        {
            if (mg_write(backend_conn, buffer.data(), bytes_read) < 0)
            {
                printf("Failed to forward POST body to backend\n");
                break;
            }
        }
    }

    // Get response from backend
    std::string response_error_buffer(256, '\0');
    int response_status = mg_get_response(backend_conn, &response_error_buffer[0], response_error_buffer.size(), 5000); // 5 second timeout

    if (response_status < 0)
    {
        printf("Failed to get response from backend: %s\n", response_error_buffer.c_str());
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
    std::string response_buffer(4096, '\0');
    int total_bytes = 0;
    int bytes_read_resp;

    // Read response body in chunks
    // Note: reused variable name mismatch from original code (bytes_read vs bytes_read_resp)
    // In original code it was re-declaring bytes_read? No, it used bytes_read.
    // I will use bytes_read_resp to be safe.
    
    while ((bytes_read_resp = mg_read(backend_conn, &response_buffer[0] + total_bytes, response_buffer.size() - total_bytes - 1)) > 0)
    {
        total_bytes += bytes_read_resp;
        if (total_bytes >= (int)response_buffer.size() - 1)
        {
            break; // Buffer full
        }
    }

    if (total_bytes > 0)
    {
        response_buffer[total_bytes] = '\0'; // Null terminate
        printf("Response body from backend (%d bytes): %s\n", total_bytes, response_buffer.c_str());

        // Send content length header
        mg_printf(conn, "Content-Length: %d\r\n\r\n", total_bytes);

        // Forward the response body to the client
        mg_write(conn, response_buffer.data(), total_bytes);
    }
    else
    {
        printf("No response body from backend\n");
        mg_printf(conn, "Content-Length: 0\r\n\r\n");
    }

    // Clean up
    mg_close_connection(backend_conn);
}
