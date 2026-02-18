#ifndef PROVISION_HANDLER_H
#define PROVISION_HANDLER_H

#include "civetweb.h"
#include "fw_system.h"

class ProvisionHandler {
public:
  static int handle_provision_device(struct mg_connection *conn,
                                     const struct mg_request_info *ri);
};

#endif // PROVISION_HANDLER_H
