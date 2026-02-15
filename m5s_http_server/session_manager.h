#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>
#include <memory>

#include <cstddef>
#include <ctime>
#include "lru.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "lru.h"

// Forward declarations for C-style structs
struct SessionConfig;
struct SessionContext;
struct SessionManager;
struct HashEntry;
struct LRUCache;

// Session configuration structure
typedef struct SessionConfig {
    size_t max_sessions;
    size_t eviction_queue_size;
    time_t session_timeout;  // in seconds (0 means no timeout)
    
    // Cookie settings
    const char* cookie_domain;
    const char* cookie_path;
    bool secure_cookies;
    bool http_only_cookies;
} SessionConfig;



// Session manager structure
typedef struct SessionManager {
    SessionConfig config;
    LRUCache* sessions_cache;
} SessionManager;



// Session management API
SessionManager* session_manager_create(const SessionConfig* config);
void session_manager_destroy(SessionManager* manager);

bool session_create(SessionManager* manager, const SessionContext* context,
                  char* session_token_out, size_t token_buf_size);

bool session_validate(SessionManager* manager, const char* session_token,
                    SessionContext** context_out);

bool session_invalidate(SessionManager* manager, const char* session_token);
void session_cleanup_expired(SessionManager* manager);

char* session_generate_cookie_header(const SessionManager* manager,
                                   const char* session_token);

char* session_generate_invalidation_cookie_header(const SessionManager* manager);
bool session_force_logout(SessionManager* manager,const char* session_token);
void session_logout_all_others(SessionManager* manager, const char* exclude_token);
bool session_check_evicted(SessionManager* manager, const char* session_token, int* reason_out);

#ifdef __cplusplus
}
#endif

#endif // SESSION_MANAGER_H