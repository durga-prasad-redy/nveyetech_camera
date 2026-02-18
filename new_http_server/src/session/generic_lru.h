
#ifndef LRU_CACHE_H
#define LRU_CACHE_H
#include <list>
#include <unordered_map>
#include "SessionStorage.h"



class LRUCache: public SessionStorage 
{
public:
    LRUCache(int cap);
     std::shared_ptr<SessionContext> get(const std::string &key);
     void put(const std::string &key, std::shared_ptr<SessionContext> value);
     void remove(const std::string &key,EvictionReason reason);
     void clear();
     vector<CacheNode> get_cache();
};
#endif // LRU_CACHE_H
