#include "net.h"
#include "server_config.h"
#include "session_manager.h"
#include "web_server.h"
#include <cstdio>
#include <memory>

// Define global settings
namespace ServerConfig {
const Settings g_settings = {true, 1, 57, {}, {}, {}, {}};
}

// Global instance of Net
static Net g_net;

bool Net::init() {
  // Initialize session manager
  SessionConfig cfg;
  cfg.max_sessions = 5;
  cfg.eviction_queue_size = 10;
  cfg.session_timeout = 0;
  cfg.storage_type = SessionStorageType::QUEUE;
  cfg.cookie_domain = nullptr;
  cfg.cookie_path = "/";
  cfg.secure_cookies = false;
  cfg.http_only_cookies = true;

  g_session_manager = std::make_shared<SessionManager>(cfg);

  if (!g_session_manager) {
    printf("Failed to create session manager\n");
    return false;
  }

  // Initialize web server
  g_web_server = std::make_unique<WebServer>(g_session_manager);
  if (!g_web_server->init()) {
    printf("Failed to start web server\n");
    g_web_server.reset();
    g_session_manager.reset();
    return false;
  }

  printf("Web server started on port %s (HTTP & WS)\n",
         ServerConfig::HTTP_PORT);
  return true;
}

void Net::cleanup() {
  if (g_web_server) {
    g_web_server->shutdown(); // Explicit shutdown
    g_web_server.reset();
  }

  if (g_session_manager) {
    g_session_manager.reset();
  }
}
