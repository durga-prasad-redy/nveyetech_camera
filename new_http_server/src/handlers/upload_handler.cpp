#include "upload_handler.h"
#include "server_config.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h> // for remove, mkdir

static int field_found_with_size_check(const char *key,
                                       const char *filename,
                                       char *path,
                                       size_t pathlen,
                                       void *user_data)
{
    if (filename && *filename)
    {
        printf("Field found: key=%s, filename=%s\n", key, filename);

        /* Only allow files starting with ota.tar.gz */
        const char *required_prefix = "ota.tar.gz";
        size_t prefix_len = strlen(required_prefix);

        if (strncmp(filename, required_prefix, prefix_len) != 0)
        {
            printf("ERROR: Filename does not start with ota.tar.gz, rejecting upload.\n");

            struct mg_connection *conn = (struct mg_connection *)user_data;
            if (conn)
            {
                const char *body =
                    "{\"error\":\"Invalid file name. Must start with ota.tar.gz\"}\n";

                mg_printf(conn,
                          "HTTP/1.1 400 Bad Request\r\n"
                          "Content-Type: application/json\r\n"
                          "Content-Length: %zu\r\n\r\n%s",
                          strlen(body), body);
            }

            return MG_FORM_FIELD_STORAGE_SKIP;
        }

        const char *upload_dir = "/mnt/flash/vienna/firmware/ota";

        /*
         * Remove TOCTOU:
         * Do NOT stat() first.
         * Directly call mkdir() and handle EEXIST.
         */
        if (mkdir(upload_dir, 0775) == -1)
        {
            if (errno != EEXIST)
            {
                printf("ERROR: Failed to create upload directory: %s\n",
                       strerror(errno));
                return MG_FORM_FIELD_STORAGE_SKIP;
            }
        }

        /*
         * Now verify safely that the path exists
         * and is a real directory (not a symlink).
         */
        struct stat st;
        if (lstat(upload_dir, &st) == -1)
        {
            printf("ERROR: lstat failed on upload directory: %s\n",
                   strerror(errno));
            return MG_FORM_FIELD_STORAGE_SKIP;
        }

        if (!S_ISDIR(st.st_mode))
        {
            printf("ERROR: Upload path exists but is not a directory!\n");
            return MG_FORM_FIELD_STORAGE_SKIP;
        }

        /*
         * Build final path
         */
        int n = snprintf(path, pathlen, "%s/%s", upload_dir, filename);
        printf("Upload path: %s (len=%d, max=%zu)\n", path, n, pathlen);

        if (n < 0 || (size_t)n >= pathlen)
        {
            printf("ERROR: Upload path too long, fallback to /tmp\n");

            n = snprintf(path, pathlen, "/tmp/%s", filename);
            if (n < 0 || (size_t)n >= pathlen)
            {
                printf("ERROR: Fallback path too long\n");
                return MG_FORM_FIELD_STORAGE_SKIP;
            }
        }

        return MG_FORM_FIELD_STORAGE_STORE;
    }

    printf("Field skipped: key=%s (no filename)\n", key);
    return MG_FORM_FIELD_STORAGE_SKIP;
}
static int field_get_with_size_check(const char *key, const char *value, size_t valuelen, void *user_data)
{
    (void)user_data;
    printf("Received form field: %s = %.*s\n", key, (int)valuelen, value);
    return MG_FORM_FIELD_HANDLE_NEXT;
}

static int field_store_with_size_check(const char *path, long long file_size, void *user_data)
{
    (void)user_data;
    printf("File stored: %s (%lld bytes)\n", path, file_size);
    // Check if file is in the correct directory
    if (strncmp(path, "/mnt/flash/vienna/firmware/ota/", 30) != 0)
    {
        printf("WARNING: File not stored in /mnt/flash/vienna/firmware/ota!\n");
    }

    // Check if file size exceeds the limit
    if (file_size > ServerConfig::MAX_FILE_SIZE_BYTES)
    {
        printf("File size exceeded limit: %lld bytes > %lld bytes (%.1f MB)\n",
               file_size, ServerConfig::MAX_FILE_SIZE_BYTES, ServerConfig::MAX_FILE_SIZE_MB);

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

int UploadHandler::handle_upload(struct mg_connection *conn, const struct mg_request_info *req_info)
{
    printf("Handling request: %s %s\n",
           req_info->request_method,
           req_info->request_uri);

    // Check Content-Length header first if available
    const char *content_length_str = mg_get_header(conn, "Content-Length");
    if (content_length_str)
    {
        long long content_length = strtoll(content_length_str, nullptr, 10);
        if (content_length > ServerConfig::MAX_FILE_SIZE_BYTES)
        {
            printf("Request too large: Content-Length %lld exceeds limit %lld\n",
                   content_length, ServerConfig::MAX_FILE_SIZE_BYTES);

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
            return 413; // Changed logic from 413 return to return status
        }
    }
    
    // remove_ota_files(); // Call external function - assuming it's available and linked
    // Since we don't know where it is defined, and it was in net.cpp (implied), we might need to find it.
    // In net.cpp, it was called in line 970.
    // It's not defined in net.cpp (checked view). Maybe imported from "fw.h"?
    // "include/fw.h" is included in net.cpp.
    // We will assume it's there.
    // Un-commenting call.
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
            mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nError processing upload");
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
