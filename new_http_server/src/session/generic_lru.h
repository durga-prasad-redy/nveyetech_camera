
#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include "session_storage.h"

class LRUCache : public SessionStorage {
private:
  int8_t storage_capacity;
  std::list<CacheNode> cache_list; // List to maintain LRU order
  std::unordered_map<std::string, std::list<CacheNode>::iterator>
      cache_map; // Hash map for O(1) access
public:
  explicit LRUCache(int cap);
  std::shared_ptr<SessionContext> get(const std::string &key) override;
  void put(const std::string &key,
           std::shared_ptr<SessionContext> value) override;
  void remove(const std::string &key, EvictionReason reason) override;
  void clear() override;
  std::list<CacheNode> get_cache() override;
  void print_session() override;
};
#endif // LRU_CACHE_H
