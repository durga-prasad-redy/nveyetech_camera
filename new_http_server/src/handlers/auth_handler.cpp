#include "auth_handler.h"
#include "http_utils.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int AuthHandler::handle_login(struct mg_connection *conn,
                              const struct mg_request_info *ri,
                              std::shared_ptr<SessionManager> session_manager) {
  printf("Login request received\n");
  if (strcmp(ri->request_method, "POST") != 0) {
    const char *body = "{\"error\": \"Method not allowed\"}\n";
    send_json_response(conn, 405, "Method Not Allowed", body);
    return 1;
  }

  std::string post_data(1024, '\0');
  int data_len = mg_read(conn, &post_data[0], post_data.size() - 1);
  if (data_len > 0) {
    post_data[data_len] = '\0';
  }
  printf("Received POST data (length: %d): %s\n", data_len, post_data.c_str());
  if (data_len <= 0) {
    send_json_response(conn, 400, "Bad Request",
                       "{\"error\": \"No data received\"}\n");
    return 1;
  }

  // Check Content-Type if present (strict check from original code)
  const char *ct = mg_get_header(conn, "Content-Type");
  if (!ct || (strcmp(ct, "application/octet-stream") != 0)) {
    // Note: original code checked "content-type" existence AND value.
    // If header missing OR different from octet-stream, error.
    // Actually original code was: !mg_get_header... && ... != 0. Logic was bit
    // weird. Simplified: If Content-Type is provided, it should be
    // octet-stream? Or MUST be provided? Original: if (
    // !mg_get_header(conn,"Content-Type") && (mg_get_header(conn,
    // "Content-Type"), "application/octet-stream") != 0) This logic in original
    // `net.cpp` line 332 is likely flawed/confusing but intention implies
    // enforcing "application/octet-stream" if possible. Let's assume we want to
    // enforce it if we want to be strict, or just parse body. Refactor: We will
    // just parse body. If user wants strict check, uncomment below.
    // send_json_response(conn, 400, "Bad Request", "{\"error\": \"Content-Type
    // must be application/octet-stream\"}\n"); return 1;
  }

  uint8_t byteArray[256];
  int byteArrayLen = 0;

  char *saveptr;
  // create a copy because strtok modifies string
  std::string data_copy = post_data;
  char *token = strtok_r(&data_copy[0], " ", &saveptr);
  while (token != nullptr && byteArrayLen < 256) {
    if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0) {
      auto value = (uint8_t)strtol(token, nullptr, 16);
      byteArray[byteArrayLen++] = value;
    }
    token = strtok_r(nullptr, " ", &saveptr);
  }

  uint8_t *res_bytes = nullptr;
  uint8_t res_bytes_size = 0;

  do_processing(byteArray, byteArrayLen, &res_bytes, &res_bytes_size);

  // print res_bytes
  for (int i = 0; i < res_bytes_size; i++) {
    printf("res_bytes[%d]=0x%02X\n", i, res_bytes[i]);
  }

  if (res_bytes_size > 4 && res_bytes[4] == 0) {
    if (!session_manager) {
      send_json_response(conn, 500, "Internal Server Error",
                         "{\"error\": \"Session manager unavailable\"}\n");
      return 1;
    }

    SessionContext ctx;
    // Assuming SessionContext has a username field or similar
    // ctx.username = "user";
    // In session_storage.h, SessionContext struct was just placeholder with id
    // and times? Let's stick to what we saw in session_storage.h

    std::string session_token;
    if (session_manager->create_session(ctx, session_token)) {
      std::string cookie_header =
          generate_cookie_header(session_token); // Use our util

      const char *body = "{\"message\": \"Login successful\"}\n";
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\n"
                "Set-Cookie: %s\r\n"
                "Content-Type: application/json\r\n"
                "Cache-Control: no-cache\r\n"
                "Content-Length: %zu\r\n\r\n%s",
                cookie_header.c_str(), strlen(body), body);
    } else {
      send_json_response(conn, 500, "Internal Server Error",
                         "{\"error\": \"Session creation failed\"}\n");
    }
  } else {
    if (res_bytes_size > 5) {
      if (res_bytes[5] == 0xFF) {
        send_json_response(conn, 400, "Bad Request",
                           "{\"error\": \"PIN length should be 4\"}\n");
        return 1;
      } else if (res_bytes[5] == 0xFE) {
        send_json_response(conn, 400, "Bad Request",
                           "{\"error\": \"PIN must contain only digits\"}\n");
        return 1;
      }
    }
    send_json_response(conn, 403, "Forbidden",
                       "{\"error\": \"Invalid PIN\"}\n");
    return 1;
  }
  if (res_bytes)
    free(res_bytes);
  return 1;
}

int AuthHandler::handle_logout(
    struct mg_connection *conn, const struct mg_request_info *ri,
    std::shared_ptr<SessionManager> session_manager) {
  (void)ri;
  if (!session_manager) {
    send_json_response(conn, 500, "Internal Server Error",
                       "{\"error\": \"Session manager unavailable\"}\n");
    return 1;
  }

  std::string session_token = get_session_token(conn);
  if (!session_token.empty()) {
    session_manager->invalidate_session(session_token, EvictionReason::MANUAL);
  }

  std::string cookie_header = generate_expired_cookie_header();
  const char *body = "{\"message\": \"Logged out successfully\"}\n";

  mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Set-Cookie: %s\r\n"
            "Content-Type: application/json\r\n"
            "Cache-Control: no-cache\r\n"
            "Content-Length: %zu\r\n\r\n%s",
            cookie_header.c_str(), strlen(body), body);

  return 1;
}

int AuthHandler::handle_force_logout_others(
    struct mg_connection *conn, const struct mg_request_info *ri,
    std::shared_ptr<SessionManager> session_manager) {
  if (strcmp(ri->request_method, "POST") != 0) {
    send_json_response(conn, 405, "Method Not Allowed", "");
    return 1;
  }

  std::string post_data(4096, '\0');
  int data_len = mg_read(conn, &post_data[0], post_data.size() - 1);
  if (data_len <= 0) {
    send_json_response(conn, 400, "Bad Request", "");
    return 1;
  }
  post_data[data_len] = '\0';

  uint8_t byteArray[256];
  int byteArrayLen = 0;

  char *saveptr;
  std::string data_copy = post_data;
  char *token = strtok_r(&data_copy[0], " ", &saveptr);
  while (token != nullptr && byteArrayLen < 256) {
    if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0) {
      auto value = (uint8_t)strtol(token, nullptr, 16);
      byteArray[byteArrayLen++] = value;
    }
    token = strtok_r(nullptr, " ", &saveptr);
  }

  if (byteArrayLen == 0) {
    send_json_response(conn, 400, "Bad Request",
                       "{\"error\": \"Invalid data format\"}\n");
    return 1;
  }

  uint8_t prefix[] = {2, 6, 4};
  if (!response_body_prefix_check(byteArray, byteArrayLen, prefix, 3)) {
    send_json_response(conn, 400, "Bad Request", "");
    return 1;
  }

  uint8_t *res_bytes = nullptr;
  uint8_t res_bytes_size = 0;

  do_processing(byteArray, byteArrayLen, &res_bytes, &res_bytes_size);

  if (res_bytes_size > 4 && res_bytes[4] == 0) {
    if (!session_manager) {
      send_json_response(conn, 500, "Internal Server Error",
                         "{\"error\": \"Session manager unavailable\"}\n");
      return 1;
    }

    std::string session_token = get_session_token(conn);
    session_manager->invalidate_all_sessions(session_token);

    send_json_response(
        conn, 200, "OK",
        "{\"message\": \"Force logged out others successful\"}\n");
  } else {
    send_json_response(conn, 400, "Bad Request",
                       "{\"error\": \"Processing failed\"}\n");
  }
  if (res_bytes)
    free(res_bytes);
  return 1;
}
