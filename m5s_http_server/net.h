// Copyright (c) 2023 Cesanta Software Limited
// All rights reserved
#pragma once


#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <mutex>
#include <set>
#include <atomic>
#include <vector>

#include "civetweb.h"
#include "lru.h"
#include "session_manager.h"

#include <memory>
#include <string>
#include <print>

static const char HTTP_PORT[] = "80";
static const char HTTPS_PORT[] = "443";

// Event type enum for misc events
enum class MiscEventType {
    STARTED_STREAMING,
    CHANGING_MISC
};

static const int MAX_DEVICE_NAME = 40;
static const int MAX_EVENTS_NO = 400;
static const int MAX_EVENT_TEXT_SIZE = 10;
static const int EVENTS_PER_PAGE = 20;

struct proxy_data {
  struct mg_connection
      *client_conn; // Will be replaced with CivetWeb equivalent
  char *original_uri;
  char *original_method;
  char *original_body;
  size_t original_body_len;
};


// CivetWeb context wrapper
// WebServer class definition
class WebServer {
public:
    WebServer();
    ~WebServer();
    bool init();
    void shutdown();

    // WebSocket helpers
    void add_client(struct mg_connection *conn);
    void remove_client(const struct mg_connection *conn);
    void broadcast_loop();

private:
    struct mg_context *ctx;
    std::string document_root;
    
    // WebSocket thread safety
    std::mutex clients_mutex;
    std::set<const struct mg_connection*> clients;
    std::thread broadcast_thread;
    std::atomic<bool> stop_broadcast;
    
    // Broadcast helpers (reduce nesting in broadcast_loop)
    void broadcast_message(const char *json_msg);
    void handle_misc_event();
    void handle_ir_event();

    // Unix socket for misc events
    int misc_socket_fd;
    // Unix socket for IR brightness events
    int ir_socket_fd;
};

inline bool response_body_prefix_check(const uint8_t *req_bytes, const uint8_t req_bytes_size, const uint8_t *prefix, const uint8_t prefix_size)
{
  std::print("do_processing req_bytes=");
  int i;
  if(req_bytes_size < 3 || prefix_size < 3)
  {
    return false;
  }
  for (i = 0; i < 3; i++)
  {
    if(req_bytes[i] != prefix[i])
    {
      return false;
    }
  }
  return true;
}

extern "C" {
bool web_init();
void web_cleanup();
}
