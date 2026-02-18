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

// Global instances
std::unique_ptr<WebServer> g_web_server;
// g_session_manager is needed to be created and passed to WebServer.
// We can keep a static reference here if needed, but WebServer owns a
// shared_ptr to it. However, `web_cleanup` requires us to reset it.
std::shared_ptr<SessionManager> g_session_manager;

bool web_init() {
  // Initialize session manager
  SessionConfig cfg;
  cfg.max_sessions = 5;
  cfg.eviction_queue_size = 10;
  cfg.session_timeout = 0;
  cfg.storage_type = SessionStorageType::QUEUE;
  // Cookie settings are part of SessionConfig in the header I saw?
  // In session_manager.h view:
  // SessionConfig struct has: max_sessions, eviction_queue_size,
  // session_timeout, storage_type, cookie_domain, cookie_path, secure_cookies,
  // http_only_cookies.
  cfg.cookie_domain = nullptr;
  cfg.cookie_path = "/";
  cfg.secure_cookies = false;
  cfg.http_only_cookies = true;

  // SessionManager constructor takes SessionConfig
  // And it's a class, not C factory.
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

void web_cleanup() {
  if (g_web_server) {
    g_web_server->shutdown(); // Explicit shutdown
    g_web_server.reset();
  }

  if (g_session_manager) {
    g_session_manager.reset();
  }
}