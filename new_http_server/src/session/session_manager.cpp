
#include "session_manager.h"
#include "generic_lru.h"
#include "fifo_queue.h"
SessionManager::SessionManager(SessionConfig config) : config(config) {

    if (config.type == SessionStorageType::LRU) {
        sessions_cache = std::make_unique<LRUCache>(config.capacity);
    } else if (config.type == SessionStorageType::QUEUE) {
        sessions_cache = std::make_unique<FIFOQueue>(config.capacity);
    }


}

SessionManager::~SessionManager() {
    
}

std::string SessionManager::generate_session_token(size_t length)
{
    if (length != 32)
        return {};

    constexpr size_t bytes_needed = 16; // 16 bytes → 32 hex chars
    std::array<unsigned char, bytes_needed> buffer{};

    if (RAND_bytes(buffer.data(), buffer.size()) != 1)
        return {};


    std::string token;
    token.reserve(32);

    for (unsigned char byte : buffer)
    {
        token.push_back(hex_chars[(byte >> 4) & 0x0F]);
        token.push_back(hex_chars[byte & 0x0F]);
    }

    return token;
}

bool SessionManager::create_session(const SessionContext & context,std::string &session_token){
    // if(!context) // Reference cannot be null
    //    return false;
    session_token = generate_session_token(32);


    auto session_context = std::make_shared<SessionContext>(context);

    auto now=std::chrono::system_clock::now();
    session_context->created_at=now;
    session_context->last_accessed=now;
    
    sessions_cache->put(session_token, session_context);
    return true;
}




bool SessionManager::validate_session(const std::string &session_token,SessionContext &context){
    if(session_token.empty())
        return false;
    
    std::shared_ptr<SessionContext> session_context = sessions_cache->get(session_token);
    if(!session_context)
    {
        printf("session not found for token: %s\n",session_token.c_str());
        return false;
    }

    *context=session_context;

    if (config.session_timeout > 0)
    {
        auto now =std::chrono::system_clock::now();
        auto elapsed=std::chrono::duration_cast<std::chrono::seconds>(now-session_context->last_accessed).count();
        if(elapsed>config.session_timeout)
        {
            invalidate_session(session_token,EvictionReason::SESSION_TIMEOUT);
            printf("session expired for token: %s\n",session_token.c_str());
            return false;
        }
        session_context->last_accessed=now;
        
    }
    return true;


}


bool SessionManager::invalidate_session(const std::string &session_token,EvictionReason reason)
{
    if(session_token.empty())
        return false;
    
    sessions_cache->remove(session_token,reason);
    printf("session invalidated for token: %s\n",session_token.c_str());
    return true;
}


void SessionManager::cleanup_expired_sessions(){
    if(config.session_timeout==0)
        return;

    auto now=std::chrono::system_clock::now();
    auto sessions=sessions_cache->get_cache();
    for(auto& session:sessions)
    {
        auto elapsed=std::chrono::duration_cast<std::chrono::seconds>(now-session.value->last_accessed).count();
        if(elapsed>config.session_timeout)
        {
            invalidate_session(session.key,EvictionReason::SESSION_TIMEOUT);
            printf("session expired for token: %s\n",session.key.c_str());
        }
    }
}

EvictedItem* SessionManager::check_in_evicted_sessions(const std::string &session_token)
{
    if(session_token.empty())
        return nullptr;
    
 
    auto evicted_item=sessions_cache->get_evicted_item(session_token);
    if(!evicted_item)
        return nullptr;
    
   
    return evicted_item;
}

void SessionManager::invalidate_all_sessions(const std::string& except_token) {
    auto sessions = sessions_cache->get_cache();
    for (const auto& session : sessions) {
        if (session.key != except_token) {
            invalidate_session(session.key, EvictionReason::FORCE_LOGOUT);
        }
    }
}




