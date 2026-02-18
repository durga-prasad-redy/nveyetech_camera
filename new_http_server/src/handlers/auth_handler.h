#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H

#include "civetweb.h"
#include <memory>
#include "session_manager.h"

// Declaration of external processing function (assuming it's linked from elsewhere)
extern "C" {
    int8_t do_processing(const uint8_t *req_bytes, const uint8_t req_bytes_size,
                         uint8_t **res_bytes, uint8_t *res_bytes_size);
}

class AuthHandler {
public:
    static int handle_login(struct mg_connection *conn, const struct mg_request_info *ri, std::shared_ptr<SessionManager> session_manager);
    static int handle_logout(struct mg_connection *conn, const struct mg_request_info *ri, std::shared_ptr<SessionManager> session_manager);
    static int handle_force_logout_others(struct mg_connection *conn, const struct mg_request_info *ri, std::shared_ptr<SessionManager> session_manager);
};

#endif // AUTH_HANDLER_H
