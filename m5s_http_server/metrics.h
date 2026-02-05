#pragma once

#include "civetweb.h"

// Handler for /api/metrics endpoint
int handle_metrics_api(struct mg_connection *conn, const struct mg_request_info *ri);
