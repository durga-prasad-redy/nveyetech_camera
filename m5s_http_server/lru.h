#ifndef LEAST_RECENTLY_USED_H
#define LEAST_RECENTLY_USED_H

#include <cstdint>
#include <ctime>
#include <string>
#include <memory>
#include <list>
#include <unordered_map>

static const int HASH_TABLE_SIZE = 1009;

// Session context containing user information
struct SessionContext {
    std::uint32_t user_id;     // Unique user identifier
    std::time_t created_at;    // Session creation timestamp
    std::time_t last_accessed; // Last access timestamp
    std::string username;      // Username (using std::string instead of char*)
    void* custom_data;         // Optional custom session data
    
    SessionContext() : user_id(0), created_at(0), last_accessed(0), custom_data(nullptr) {}
    
    SessionContext(std::uint32_t uid, std::time_t created, std::time_t accessed, 
                   const std::string& uname = "", void* data = nullptr)
        : user_id(uid), created_at(created), last_accessed(accessed), 
          username(uname), custom_data(data) {}
};

// Cache node representing each key-value pair
struct CacheNode {
    std::string key;
    SessionContext* value;
    CacheNode* prev;
    CacheNode* next;
    
    CacheNode() : value(nullptr), prev(nullptr), next(nullptr) {}
};

struct HashEntry {
    CacheNode* node;
    HashEntry* next;
    
    HashEntry() : node(nullptr), next(nullptr) {}
};

enum EvictionReason {
    EXPIRED,
    CAPACITY_LIMIT,
    CRITICAL_ACTION,
    MANUAL
};

// Item in the eviction queue
struct EvictedItem {
    std::string key;
    SessionContext value;
    EvictedItem* next;
    EvictionReason reason;

    
    EvictedItem() : next(nullptr) {}
};

// LRU cache structure
struct LRUCache {
    int capacity;
    int size;
    CacheNode* head;
    CacheNode* tail;
    HashEntry* hash_table[HASH_TABLE_SIZE];

    // Eviction queue
    int eviction_queue_size;
    int eviction_count;
    EvictedItem* eviction_head;
    EvictedItem* eviction_tail;

    // Internal ownership for dynamically allocated nodes and metadata
    std::list<std::unique_ptr<CacheNode>> nodes_storage;
    std::unordered_map<CacheNode*, std::list<std::unique_ptr<CacheNode>>::iterator> nodes_index;

    std::list<std::unique_ptr<HashEntry>> hashentry_storage;
    std::unordered_map<HashEntry*, std::list<std::unique_ptr<HashEntry>>::iterator> hashentry_index;

    std::list<std::unique_ptr<EvictedItem>> evicted_storage;
    std::unordered_map<EvictedItem*, std::list<std::unique_ptr<EvictedItem>>::iterator> evicted_index;
    
    LRUCache() : capacity(0), size(0), head(nullptr), tail(nullptr),
                 eviction_queue_size(0), eviction_count(0), 
                 eviction_head(nullptr), eviction_tail(nullptr) {
        for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
            hash_table[i] = nullptr;
        }
    }
};

// Public API functions
LRUCache* lru_create(int capacity, int eviction_queue_size);
void lru_put(LRUCache* cache, const std::string& key, SessionContext* value);
void lru_evict_all(LRUCache *cache, const std::string &key, SessionContext *value);
bool lru_get(LRUCache* cache, const std::string& key, SessionContext*& out_value);
EvictedItem* lru_get_evicted_items(LRUCache* cache);
EvictedItem* lru_find_evicted(LRUCache* cache, const std::string& key);
void lru_free(LRUCache* cache);
void hash_remove(LRUCache* cache, const std::string& key);

#endif // LEAST_RECENTLY_USED_H