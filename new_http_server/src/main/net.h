#pragma once

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <set>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "civetweb.h"
#include "session_manager.h"
#include "web_server.h"

#include <memory>
#include <string>

enum class MiscEventType { STARTED_STREAMING, CHANGING_MISC };

class Net {
public:
  Net() = default;
  ~Net() = default;

  bool init();
  void cleanup();

private:
  std::unique_ptr<WebServer> g_web_server;
  std::shared_ptr<SessionManager> g_session_manager;
};

inline bool response_body_prefix_check(const uint8_t *req_bytes,
                                       const uint8_t req_bytes_size,
                                       const uint8_t *prefix,
                                       const uint8_t prefix_size) {
  printf("do_processing req_bytes=");
  if (req_bytes_size < 3 || prefix_size < 3) {
    return false;
  }
  for (int i = 0; i < 3; i++) {
    if (req_bytes[i] != prefix[i]) {
      return false;
    }
  }
  return true;
}
