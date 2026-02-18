#ifndef UPLOAD_HANDLER_H
#define UPLOAD_HANDLER_H

#include "civetweb.h"

// External function to remove OTA files
extern "C" {
    void remove_ota_files(); 
    // If this is C++, it might be mangled if not extern C. 
    // In net.cpp it was called directly. Assuming it's available.
}

class UploadHandler {
public:
    static int handle_upload(struct mg_connection *conn, const struct mg_request_info *ri);
};

#endif // UPLOAD_HANDLER_H
