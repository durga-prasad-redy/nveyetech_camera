
#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H
#include "session_storage.h"

const std::string SESSION_TOKEN_BASE_PATH =
    "/mnt/flash/vienna/m5s_config/session_tokens/";

class FIFOQueue : public SessionStorage {
private:
  int8_t storage_capacity;
  std::list<CacheNode> cache_list; // List to maintain LRU order
  std::unordered_map<std::string, std::list<CacheNode>::iterator>
      cache_map; // Hash map for O(1) access
public:
  explicit FIFOQueue(int cap);
  std::shared_ptr<SessionContext> get(const std::string &key) override;
  void put(const std::string &key,
           std::shared_ptr<SessionContext> value) override;
  void remove(const std::string &key, EvictionReason reason) override;
  void clear() override;
  std::list<CacheNode> get_cache() override;
  std::string get_session_file_path(const std::string &key);
  void write_session_to_file(const std::string &key, const SessionContext *ctx);
  void delete_session_file(const std::string &key);
  void print_session() override;
};
#endif // FIFO_QUEUE_H