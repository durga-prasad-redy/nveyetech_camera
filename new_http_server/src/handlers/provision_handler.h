#ifndef PROVISION_HANDLER_H
#define PROVISION_HANDLER_H

#include "civetweb.h"

// External declaration
extern "C" {
    int8_t provisioning_mode(const char *mac_address, const char *serial_number, const char *manufacture_date);
}

class ProvisionHandler {
public:
    static int handle_provision_device(struct mg_connection *conn, const struct mg_request_info *ri);
};

#endif // PROVISION_HANDLER_H
