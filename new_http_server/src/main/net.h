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
#include "session_manager.h"

#include <memory>
#include <string>



enum class MiscEventType {
    STARTED_STREAMING,
    CHANGING_MISC
};







// WebServer class definition
// WebServer class has been moved to web_server.h
// class WebServer; 


inline bool response_body_prefix_check(const uint8_t *req_bytes, const uint8_t req_bytes_size, const uint8_t *prefix, const uint8_t prefix_size)
{
  printf("do_processing req_bytes=");
  if(req_bytes_size < 3 || prefix_size < 3)
  {
    return false;
  }
  for (int i = 0; i < 3; i++)
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
