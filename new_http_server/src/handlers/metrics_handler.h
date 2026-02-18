#ifndef METRICS_HANDLER_H
#define METRICS_HANDLER_H

#include "civetweb.h"

extern "C" {
    int handle_metrics_api(struct mg_connection *conn, const struct mg_request_info *ri);
}

#endif // METRICS_HANDLER_H
