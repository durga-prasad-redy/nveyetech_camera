#ifndef PROXY_HANDLER_H
#define PROXY_HANDLER_H

#include "civetweb.h"

class ProxyHandler {
public:
    static void handle_proxy_request(struct mg_connection *conn, const struct mg_request_info *ri,
                                     const char *backend_url, const char *target_path);
};

#endif // PROXY_HANDLER_H
