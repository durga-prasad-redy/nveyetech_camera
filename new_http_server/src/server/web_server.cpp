#include "web_server.h"
#include "auth_handler.h"
#include "device_handler.h"
#include "http_utils.h"
#include "provision_handler.h"
#include "proxy_handler.h"
#include "server_config.h"
#include "upload_handler.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

// Include metrics header if possible, or declare extern
extern "C" {
int handle_metrics_api(struct mg_connection *conn,
                       const struct mg_request_info *ri);
}

// MUST match sender structure exactly for binary compatibility
struct misc_event {
  uint8_t old_misc;
  uint8_t new_misc;
};
using misc_event_t = misc_event;

// MUST match sender structure exactly
struct ir_event {
  uint8_t old_ir_brightness;
  uint8_t new_ir_brightness;
};
using ir_event_t = ir_event;

enum class MiscEventType { STARTED_STREAMING, CHANGING_MISC };

static const char *misc_event_type_to_string(MiscEventType type) {
  switch (type) {
  case MiscEventType::STARTED_STREAMING:
    return "started streaming";
  case MiscEventType::CHANGING_MISC:
    return "changing misc";
  default:
    return "unknown";
  }
}

// Log macro wrapper
#define LOG_DEBUG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)

WebServer::WebServer(std::shared_ptr<SessionManager> mgr)
    : ctx(nullptr), session_manager(mgr) {}
WebServer::~WebServer() { shutdown(); }

void WebServer::add_client(struct mg_connection *conn) {
  std::lock_guard<std::mutex> lock(clients_mutex);
  clients.insert(conn);
}

void WebServer::remove_client(const struct mg_connection *conn) {
  std::lock_guard<std::mutex> lock(clients_mutex);
  clients.erase((struct mg_connection *)conn); // cast const away
}

void WebServer::broadcast_message(const char *json_msg) {
  std::lock_guard<std::mutex> lock(clients_mutex);
  for (auto conn : clients) {
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, json_msg,
                       strlen(json_msg));
  }
}

void WebServer::handle_misc_event() {
  if (misc_socket_fd < 0)
    return;

  misc_event_t evt;
  ssize_t n = recv(misc_socket_fd, &evt, sizeof(evt), MSG_DONTWAIT);

  if (n == static_cast<ssize_t>(sizeof(evt))) {
    LOG_DEBUG("Misc event received: old_misc=%u, new_misc=%u", evt.old_misc,
              evt.new_misc);

    MiscEventType event_type = (evt.old_misc == 0 || evt.new_misc == 0)
                                   ? MiscEventType::STARTED_STREAMING
                                   : MiscEventType::CHANGING_MISC;

    std::string json_msg = R"({"old_misc":)" + std::to_string(evt.old_misc) +
                           R"(,"new_misc":)" + std::to_string(evt.new_misc) +
                           R"(,"event_type":")" +
                           misc_event_type_to_string(event_type) + R"("})";

    broadcast_message(json_msg.c_str());
  }
}

void WebServer::handle_ir_event() {
  if (ir_socket_fd < 0)
    return;

  ir_event_t evt;
  ssize_t n = recv(ir_socket_fd, &evt, sizeof(evt), MSG_DONTWAIT);

  if (n == static_cast<ssize_t>(sizeof(evt))) {
    LOG_DEBUG("IR brightness event received: old_ir_brightness=%u, "
              "new_ir_brightness=%u",
              evt.old_ir_brightness, evt.new_ir_brightness);

    std::string json_msg =
        R"({"old_ir_brightness":)" + std::to_string(evt.old_ir_brightness) +
        R"(,"new_ir_brightness":)" + std::to_string(evt.new_ir_brightness) +
        R"(,"event_type":"ir brightness changed"})";

    broadcast_message(json_msg.c_str());
  }
}

void WebServer::broadcast_loop() {
  while (!stop_broadcast) {
    handle_misc_event();
    handle_ir_event();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

int WebServer::request_handler(struct mg_connection *conn) {
  // Retrieve the 'this' pointer from the context
  struct mg_context *ctx = mg_get_context(conn);
  auto *self = static_cast<WebServer *>(mg_get_user_data(ctx));

  const struct mg_request_info *ri = mg_get_request_info(conn);

  // ... rest of your logic using 'self' ...
  printf("Request: %s %s\n", ri->request_method, ri->local_uri);

  // 1. Handle WebSocket Upgrade
  const char *upgrade = mg_get_header(conn, "Upgrade");
  if (upgrade && strcasecmp(upgrade, "websocket") == 0)
    return 0;

  // 2. Auth Logic
  if (!self->is_authorized(conn, ri))
    return 1;

  // 3. Routing
  return self->route_request(conn, ri);
}

bool WebServer::is_authorized(struct mg_connection *conn,
                              const struct mg_request_info *ri) {
  // Define protected routes
  static const std::array<const char *, 4> protected_routes = {
      "/api/getdeviceid", "/api/getsignalserver", "/api/upload",
      "/api/motocam_api"};

  bool needs_auth = false;
  for (const auto *route : protected_routes) {
    if (strncmp(ri->local_uri, route, strlen(route)) == 0) {
      needs_auth = true;
      break;
    }
  }

  if (!needs_auth)
    return true;

  if (!session_manager) {
    mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
    return false;
  }

  std::string session_token = get_session_token(conn);
  SessionContext context;
  if (session_token.empty() ||
      !session_manager->validate_session(session_token, context)) {
    const char *body =
        R"({"error": "Unauthorized", "message": "Missing or invalid session"}
)";
    send_json_response(conn, 401, "Unauthorized", body);
    return false;
  }

  return true;
}
int WebServer::serve_static_or_spa(struct mg_connection *conn,
                                   const struct mg_request_info *ri) {
  // Static file serving and SPA fallback
  std::string file_path = "dist" + std::string(ri->local_uri);
  struct stat st;

  if (stat(file_path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
    // File exists, let CivetWeb serve it
    return 0; // Let CivetWeb handle static files
  } else {
    // SPA fallback - serve index.html
    std::string index_content = read_file_content("dist/index.html");
    if (!index_content.empty()) {
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %zu\r\n\r\n%s",
                index_content.length(), index_content.c_str());
    } else {
      mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n"
                      "Content-Type: text/plain\r\n\r\n"
                      "index.html not found\n");
    }
    return 1;
  }
}

int WebServer::route_request(struct mg_connection *conn,
                             const struct mg_request_info *ri) {
  // Auth
  if (strcmp(ri->local_uri, "/api/login") == 0)
    return AuthHandler::handle_login(conn, ri, session_manager);
  if (strcmp(ri->local_uri, "/api/logout") == 0)
    return AuthHandler::handle_logout(conn, ri, session_manager);

  // Device/System
  if (strcmp(ri->local_uri, "/api/motocam_api") == 0)
    return DeviceHandler::handle_motocam_api(conn, ri);
  if (strcmp(ri->local_uri, "/api/provision_device") == 0)
    return ProvisionHandler::handle_provision_device(conn, ri);
  if (strcmp(ri->local_uri, "/api/metrics") == 0)
    return handle_metrics_api(conn, ri);
  if (strcmp(ri->local_uri, "/api/upload") == 0)
    return UploadHandler::handle_upload(conn, ri);
  if (strcmp(ri->local_uri, "/api/firmware_version") == 0)
    return DeviceHandler::handle_firmware_version(conn, ri);
  if (strcmp(ri->local_uri, "/api/reset_pin") == 0)
    return DeviceHandler::handle_reset_pin(conn, ri);
  // Proxy
  if (strncmp(ri->local_uri, "/getdeviceid", 12) == 0) {
    ProxyHandler::handle_proxy_request(conn, ri, "http://127.0.0.1:8083",
                                       "/getdeviceid");
    return 1;
  }

  if (strncmp(ri->local_uri, "/getsignalserver", 16) == 0) {
    ProxyHandler::handle_proxy_request(conn, ri, "http://127.0.0.1:8083",
                                       "/getsignalserver");
    return 1;
  }

  // Fallback to static files
  return serve_static_or_spa(conn, ri);
}

// WebSocket Callbacks
int WebServer::ws_connect_handler(const struct mg_connection *conn,
                                  void *user_data) {
  // Cast to the specific class type to provide context and meaning
  auto *self = static_cast<WebServer *>(user_data);

  // Now 'self' can be used to access WebServer members if needed
  const auto *ri = mg_get_request_info(conn);

  printf("Client connecting to server [%p] with subprotocol: %s\n",
         static_cast<void *>(self), ri->acceptedWebSocketSubprotocol);

  return 0;
}

void WebServer::ws_ready_handler(struct mg_connection *conn, void *user_data) {
  auto *server = (WebServer *)user_data;
  server->add_client(conn);
  printf("Client ready and added to broadcast list\n");
}

int WebServer::ws_data_handler(struct mg_connection *conn, int opcode,
                               char *data, size_t datasize, void *user_data) {
  (void)user_data;
  printf("Received %lu bytes from client\n", (unsigned long)datasize);
  return 1;
}

void WebServer::ws_close_handler(const struct mg_connection *conn,
                                 void *user_data) {
  auto *server = (WebServer *)user_data;
  server->remove_client(conn);
  printf("Client closed connection\n");
}

bool WebServer::init() {
  mg_init_library(0);

  std::array<const char *, 11> options = {{
      "listening_ports", ServerConfig::HTTP_PORT, "document_root",
      document_root.c_str(), "num_threads", "10", "request_timeout_ms", "60000",
      "enable_directory_listing", "no",
      nullptr // Maintain null termination for the C API
  }};

  printf("Starting server with document_root: %s\n", document_root.c_str());

  struct mg_callbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.begin_request = request_handler;

  ctx = mg_start(&callbacks, this, options.data()); // Pass 'this' as user_data

  if (!ctx) {
    printf("mg_start failed\n");
    return false;
  }

  char subprotocol_bin[] = "Outdu.Nveyetech_camera.bin";
  char subprotocol_json[] = "Outdu.Nveyetech_camera.json";
  std::array<const char *, 3> subprotocol_list = {subprotocol_bin,
                                                  subprotocol_json, nullptr};

  struct mg_websocket_subprotocols wsprot = {
      static_cast<int>(subprotocol_list.size() - 1), subprotocol_list.data()};

  mg_set_websocket_handler_with_subprotocols(
      ctx, "/wsURL", &wsprot, ws_connect_handler, ws_ready_handler,
      ws_data_handler, ws_close_handler, this);

  // Init sockets (Misc & IR) - similar to original code
  // ... (Code omitted for brevity, assuming similar to original but using
  // ServerConfig::MISC_SOCK_PATH) Actually I should copy the socket init code
  // from original net.cpp

  misc_socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (misc_socket_fd >= 0) {
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ServerConfig::MISC_SOCK_PATH,
            sizeof(addr.sun_path) - 1);
    unlink(ServerConfig::MISC_SOCK_PATH);
    if (bind(misc_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      close(misc_socket_fd);
      misc_socket_fd = -1;
    }
  }

  ir_socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (ir_socket_fd >= 0) {
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ServerConfig::IR_SOCK_PATH,
            sizeof(addr.sun_path) - 1);
    unlink(ServerConfig::IR_SOCK_PATH);
    if (bind(ir_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      close(ir_socket_fd);
      ir_socket_fd = -1;
    }
  }

  stop_broadcast = false;
  broadcast_thread = std::thread(&WebServer::broadcast_loop, this);

  printf("WebSocket server registered on /wsURL\n");
  return true;
}

void WebServer::shutdown() {
  stop_broadcast = true;
  if (broadcast_thread.joinable())
    broadcast_thread.join();

  if (ctx) {
    mg_stop(ctx);
    ctx = nullptr;
  }

  if (misc_socket_fd >= 0) {
    close(misc_socket_fd);
    misc_socket_fd = -1;
    unlink(ServerConfig::MISC_SOCK_PATH);
  }
  if (ir_socket_fd >= 0) {
    close(ir_socket_fd);
    ir_socket_fd = -1;
    unlink(ServerConfig::IR_SOCK_PATH);
  }

  mg_exit_library();
}
