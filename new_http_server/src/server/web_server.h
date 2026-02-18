#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "civetweb.h"
#include <set>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include "session_manager.h"

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

private:

    int misc_socket_fd{-1};
    int ir_socket_fd{-1};
    bool stop_broadcast{false};
    std::string document_root{"dist"};

    struct mg_context *ctx;
    std::shared_ptr<SessionManager> session_manager;
    std::string document_root;
    
    // WebSocket clients
    std::set<struct mg_connection *> clients;
    std::mutex clients_mutex;

    // Broadcasting thread and sockets
    bool stop_broadcast;
    std::thread broadcast_thread;
    int misc_socket_fd;
    int ir_socket_fd;

    void handle_misc_event();
    void handle_ir_event();
    void broadcast_loop();

    // Static request handlers routed to instance
    static int request_handler(struct mg_connection *conn, void *cbdata);
    bool is_authorized(struct mg_connection *conn, const struct mg_request_info *ri);
    int route_request(struct mg_connection *conn, const struct mg_request_info *ri);
    
    // WebSocket handlers
    static int ws_connect_handler(const struct mg_connection *conn, void *user_data);
    static void ws_ready_handler(struct mg_connection *conn, void *user_data);
    static int ws_data_handler(struct mg_connection *conn, int opcode, char *data, size_t datasize, void *user_data);
    static void ws_close_handler(const struct mg_connection *conn, void *user_data);
};

#endif // WEB_SERVER_H
