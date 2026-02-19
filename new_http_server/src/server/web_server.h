#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "civetweb.h"
#include "session_manager.h"
#include <atomic>
#include <mutex>
#include <set>
#include <string>
#include <thread>

// Forward declaration
struct mg_context;
struct mg_connection;

class WebServer {
public:
  explicit WebServer(std::shared_ptr<SessionManager> session_manager);
  ~WebServer();

  bool init();
  void shutdown();

  // WebSocket management
  void add_client(struct mg_connection *conn);
  void remove_client(const struct mg_connection *conn);
  void broadcast_message(const char *json_msg);
  int serve_static_or_spa(struct mg_connection *conn,
                          const struct mg_request_info *ri);

private:
  int misc_socket_fd{-1};
  int ir_socket_fd{-1};
  bool stop_broadcast{false};
  std::string document_root{"dist"};

  struct mg_context *ctx;
  std::shared_ptr<SessionManager> session_manager;

  // WebSocket clients
  std::set<struct mg_connection *> clients;
  std::mutex clients_mutex;

  // Broadcasting thread and sockets
  std::thread broadcast_thread;

  void handle_misc_event();
  void handle_ir_event();
  void broadcast_loop();

  // Static request handlers routed to instance
  static int request_handler(struct mg_connection *conn);
  bool is_authorized(struct mg_connection *conn,
                     const struct mg_request_info *ri);
  int route_request(struct mg_connection *conn,
                    const struct mg_request_info *ri);

  // WebSocket handlers
  static int ws_connect_handler(const struct mg_connection *conn,
                                WebServer *self);
  static void ws_ready_handler(struct mg_connection *conn, WebServer *self);
  static int ws_data_handler(struct mg_connection *conn, int opcode, char *data,
                             size_t datasize, WebServer *self);
  static void ws_close_handler(const struct mg_connection *conn,
                               WebServer *self);
};

#endif // WEB_SERVER_H
