#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "session_storage.h"
#include <memory>
#include <openssl/rand.h>
#include <vector>

// Session configuration structure
struct SessionConfig {
  size_t max_sessions{};
  size_t eviction_queue_size{};
  time_t session_timeout{}; // in seconds (0 means no timeout)
  SessionStorageType storage_type{};
  // Cookie settings
  const char *cookie_domain{};
  const char *cookie_path{};
  bool secure_cookies{};
  bool http_only_cookies{};
};

class SessionManager {
private:
  SessionConfig config;
  std::unique_ptr<SessionStorage> sessions_cache;
  const size_t SESSION_TOKEN_LENGTH = 32; // 256-bit session tokens
  const std::string SESSION_SECRET_KEY = "0123456789ABCDEF";

public:
  explicit SessionManager(const SessionConfig &session_config);
  ~SessionManager() = default;
  bool create_session(const SessionContext &context,
                      std::string &session_token);
  bool validate_session(const std::string &session_token,
                        SessionContext &context);
  bool invalidate_session(const std::string &session_token,
                          EvictionReason reason);
  void cleanup_expired_sessions();

  bool force_logout(const std::string &session_token);
  EvictedItem *check_in_evicted_sessions(const std::string &session_token);

  bool get_session_context(const std::string &session_token,
                           SessionContext &context);

  std::vector<EvictedItem> get_evicted_sessions();
  std::string generate_session_token(size_t length);

  void clear_evicted_sessions();
  void invalidate_all_sessions(const std::string &except_token = "");
};

#endif // SESSION_MANAGER_H