#include "session_manager.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <vector>
#include <memory>
#include <openssl/rand.h>

static const size_t SESSION_TOKEN_LENGTH = 32;  // 256-bit session tokens
static const char HEX_CHARS[] = "0123456789ABCDEF";

namespace {

bool generate_session_token(char* buffer, size_t length) {
    if (length < SESSION_TOKEN_LENGTH*2 + 1) return false;
    
    unsigned char random_bytes[SESSION_TOKEN_LENGTH];
    if (RAND_bytes(random_bytes, SESSION_TOKEN_LENGTH) != 1) {
        return false;
    }
    
    for (int i = 0; i < SESSION_TOKEN_LENGTH; i++) {
        buffer[i*2]   = HEX_CHARS[(random_bytes[i] >> 4) & 0x0F];
        buffer[i*2+1] = HEX_CHARS[random_bytes[i] & 0x0F];
    }
    buffer[SESSION_TOKEN_LENGTH*2] = '\0';
    return true;
}

} // anonymous namespace

SessionManager* session_manager_create(const SessionConfig* config) {
    if (!config || config->max_sessions == 0) return nullptr;
    
    auto manager = std::make_unique<SessionManager>();
    memcpy(&manager->config, config, sizeof(SessionConfig));
    
    manager->sessions_cache = lru_create(static_cast<int>(config->max_sessions),
                                         static_cast<int>(config->eviction_queue_size));
    if (!manager->sessions_cache) {
        return nullptr;
    }
    
    return manager.release();
}

void session_manager_destroy(SessionManager* manager) {
    if (!manager) return;
    
    if (manager->sessions_cache) {
        lru_free(manager->sessions_cache);
    }
    
    std::default_delete<SessionManager>{}(manager);
}

bool session_create(SessionManager* manager, const SessionContext* context, 
                   char* session_token_out, size_t token_buf_size) {
    if (!manager || !context || !session_token_out || token_buf_size < SESSION_TOKEN_LENGTH*2+1) {
        return false;
    }
    
    generate_session_token(session_token_out, token_buf_size);
    
    auto cached_context = std::make_unique<SessionContext>(*context);
    

    
    time_t now = time(nullptr);
    printf("time now: %ld\n", now);
    cached_context->created_at = now;
    cached_context->last_accessed = now;
    
    SessionContext* raw_context = cached_context.get();
    lru_put(manager->sessions_cache, session_token_out, raw_context);
    // Intentionally release ownership; lifetime is managed via LRU/invalidation
    cached_context.release();
    
    return true;
}


bool session_force_logout(SessionManager* manager,const char* session_token)
{
  if(!manager  || ! session_token){
    return false;
  }

  SessionContext *context_value=nullptr;
  printf("manager time : %lu\n",manager->config.session_timeout);

    if(!lru_get(manager->sessions_cache,session_token,context_value)){
    printf("Session not found for token: %s\n",session_token);
    return false;
  }

//   *context=context_value;

   if (manager->config.session_timeout > 0) {
        time_t now = time(nullptr);
        if (difftime(now, context_value->last_accessed) > manager->config.session_timeout) {
            session_invalidate(manager, session_token);
            // *context = NULL;
            return false;
        }
        context_value->last_accessed = now;
        printf("time now: %lu\n", context_value->last_accessed);
    }


  lru_evict_all(manager->sessions_cache, session_token, context_value);


  return true;
}
bool session_validate(SessionManager* manager, const char* session_token, 
                     SessionContext** context_out) {
    if (!manager || !session_token || !context_out) return false;

    SessionContext *context_value=nullptr;
    printf("manager time : %lu\n", manager->config.session_timeout);

    if (!lru_get(manager->sessions_cache, session_token, context_value)) {
        printf("Session not found for token: %s\n", session_token);
        return false;
    }
    

    *context_out = context_value;

    if (manager->config.session_timeout > 0) {
        time_t now = time(nullptr);
        if (difftime(now, context_value->last_accessed) > manager->config.session_timeout) {
            session_invalidate(manager, session_token);
            *context_out = nullptr;
            return false;
        }
        context_value->last_accessed = now;
        printf("time now: %lu\n", context_value->last_accessed);
    }

    return true;
}

bool session_invalidate(SessionManager* manager, const char* session_token) {
    if (!manager || !session_token) return false;
    
    SessionContext *context_value=nullptr;
    std::string str(session_token);
const std::string& ref = str;

    if (lru_get(manager->sessions_cache, ref, context_value)) {
        // SessionContext* context = &context_value;
        // if (!context_value->username.empty()) { /* cleanup username storage if needed */ }
        // if (context_value->custom_data) { /* cleanup custom_data storage if owned here */ }
    }
    
    hash_remove(manager->sessions_cache, session_token);
    return true;
}

void session_cleanup_expired(SessionManager* manager) {
    if (!manager || manager->config.session_timeout == 0) return;
    
    time_t now = time(nullptr);
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashEntry* entry = manager->sessions_cache->hash_table[i];
        while (entry) {
            SessionContext* context = static_cast<SessionContext*>(entry->node->value);
            if (difftime(now, context->last_accessed) > manager->config.session_timeout) {
                const std::string& key_to_remove = entry->node->key;
                entry = entry->next;
                session_invalidate(manager, key_to_remove.c_str());
            } else {
                entry = entry->next;
            }
        }
    }
}

char* session_generate_cookie_header(const SessionManager* manager,
    const char* session_token)
{
if (!manager || !session_token)
return nullptr;

const char* path =
manager->config.cookie_path ? manager->config.cookie_path : "/";

const char* domain =
manager->config.cookie_domain ? manager->config.cookie_domain : "";

const char* secure =
manager->config.secure_cookies ? "; Secure" : "";

const char* http_only =
manager->config.http_only_cookies ? "; HttpOnly" : "";

const char* domain_prefix =
manager->config.cookie_domain ? "; Domain=" : "";

// First pass: compute required size
int size = std::snprintf(
nullptr,
0,
"session=%s; Path=%s; SameSite=Strict%s%s%s%s",
session_token,
path,
secure,
http_only,
domain_prefix,
domain
);

if (size <= 0)
return nullptr;

// Allocate exact memory (+1 for null terminator)
char* header = new char[size + 1];

std::snprintf(
header,
size + 1,
"session=%s; Path=%s; SameSite=Strict%s%s%s%s",
session_token,
path,
secure,
http_only,
domain_prefix,
domain
);

return header;  // Caller must delete[] this
}

char* session_generate_invalidation_cookie_header(const SessionManager* manager)
{
    if (!manager)
        return nullptr;

    const char* path =
        manager->config.cookie_path ? manager->config.cookie_path : "/";

    const char* domain =
        manager->config.cookie_domain ? manager->config.cookie_domain : "";

    const char* secure =
        manager->config.secure_cookies ? "; Secure" : "";

    const char* http_only =
        manager->config.http_only_cookies ? "; HttpOnly" : "";

    const char* domain_prefix =
        manager->config.cookie_domain ? "; Domain=" : "";

    // First pass: compute required size
    int size = std::snprintf(
        nullptr,
        0,
        "session=; Path=%s; Expires=Thu, 01 Jan 1970 00:00:00 GMT; SameSite=Strict%s%s%s%s",
        path,
        secure,
        http_only,
        domain_prefix,
        domain
    );

    if (size <= 0)
        return nullptr;

    // Allocate exact size (+1 for null terminator)
    char* header = new char[size + 1];

    std::snprintf(
        header,
        size + 1,
        "session=; Path=%s; Expires=Thu, 01 Jan 1970 00:00:00 GMT; SameSite=Strict%s%s%s%s",
        path,
        secure,
        http_only,
        domain_prefix,
        domain
    );

    return header;  // Caller must delete[] this
}


void session_logout_all_others(SessionManager* manager, const char* exclude_token) {
    if (!manager || !manager->sessions_cache) return;
    
    // Collect all session tokens
    std::vector<std::string> session_tokens;
    
    // Iterate through hash table to get all session tokens
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        HashEntry* entry = manager->sessions_cache->hash_table[i];
        while (entry) {
            if (entry->node && !entry->node->key.empty()) {
                std::string token = entry->node->key;
                // If exclude_token is provided, skip it; otherwise logout all
                if (!exclude_token || token != exclude_token) {
                    session_tokens.push_back(token);
                }
            }
            entry = entry->next;
        }
    }
    
    // Invalidate all collected sessions
    for (const auto& token : session_tokens) {
        printf("Invalidating session: %s\n", token.c_str());
        session_invalidate(manager, token.c_str());
    }
}

bool session_check_evicted(SessionManager* manager, const char* session_token, int* reason_out) {
    if (!manager || !session_token) return false;

    EvictedItem* item = lru_find_evicted(manager->sessions_cache, session_token);
    if (item) {
        if (reason_out) {
            *reason_out = (int)item->reason;
        }
        return true;
    }
    return false;
}
