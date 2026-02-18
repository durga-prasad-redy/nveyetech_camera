
#ifndef LRU_CACHE_H
#define LRU_CACHE_H
#include <list>
#include <unordered_map>
#include "SessionStorage.h"



class LRUCache: public SessionStorage 
{
public:
     explicit LRUCache(int cap);
     std::shared_ptr<SessionContext> get(const std::string &key) override;
     void put(const std::string &key, std::shared_ptr<SessionContext> value) override;
     void remove(const std::string &key,EvictionReason reason) override;
     void clear() override;
     vector<CacheNode> get_cache() override;
};
#endif // LRU_CACHE_H
