#ifndef DEVICE_HANDLER_H
#define DEVICE_HANDLER_H

#include "civetweb.h"
#include <memory>
#include "session_manager.h"

class DeviceHandler {
public:
    static int handle_motocam_api(struct mg_connection *conn, const struct mg_request_info *ri);
    static int handle_reset_pin(struct mg_connection *conn, const struct mg_request_info *ri);
    static int handle_firmware_version(struct mg_connection *conn, const struct mg_request_info *ri);
};

#endif // DEVICE_HANDLER_H
