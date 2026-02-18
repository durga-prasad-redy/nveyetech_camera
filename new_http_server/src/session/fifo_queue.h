
#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H
#include <list>
#include <unordered_map>
#include "SessionStorage.h"

const std::string SESSION_TOKEN_BASE_PATH = "/mnt/flash/vienna/m5s_config/session_tokens/";

class FIFOQueue: public SessionStorage 
{
public:
    FIFOQueue(int cap);
     std::shared_ptr<SessionContext> get(std::string key);
     void put(const std::string &key, std::shared_ptr<SessionContext> value);
     void remove(const std::string &key,EvictionReason reason);
     void clear();
     vector<CacheNode> get_cache();
     std::string get_session_file_path(const std::string& key);
     void write_session_to_file(const std::string& key, const SessionContext* ctx);
     void delete_session_file(const std::string& key);
};
#endif // FIFO_QUEUE_H