#ifndef SESSION_STORAGE_H
#define SESSION_STORAGE_H

#include <chrono>
#include <string>
#include <forward_list>
#include <memory>

struct SessionContext {
    // Placeholder for session context data
    // You can add relevant fields here
    std::int32_t session_id;
    std::chrono::system_clock::time_point created_at;    // Session creation timestamp
    std::chrono::system_clock::time_point last_accessed; // Last access timestamp
};

struct CacheNode {
    std::string key;
    std::shared_ptr<SessionContext> value;

};
enum EvictionReason {
    EXPIRED,
    CAPACITY_LIMIT,
    CRITICAL_ACTION,
    FORCE_LOGOUT,
    SESSION_TIMEOUT,
    MANUAL
};

enum SessionStorageType {
    LRU,
    QUEUE
};
// Item in the eviction queue
struct EvictedItem {
    std::string key;
    std::shared_ptr<SessionContext> value;
    EvictionReason reason;
};

template<typename T>
class EvictionQueue {
private:
    std::forward_list<EvictedItem> queue;
    size_t queue_max_size;
    size_t current_size;

public:
    EvictionQueue(size_t max_size)
        : queue_max_size(max_size), current_size(0) {}

    void push(const EvictedItem& item) {

        if (queue_max_size == 0)
            return;  // Prevent edge case crash

        if (current_size >= queue_max_size) {

            if (queue.empty())
                return;

            auto prev = queue.before_begin();
            auto curr = queue.begin();

            // Traverse to last element
            while (std::next(curr) != queue.end()) {
                ++prev;
                ++curr;
            }

            queue.erase_after(prev);
            --current_size;
        }

        queue.push_front(item);
        ++current_size;
    }

    bool is_full() const {
        return current_size >= queue_max_size;
    }

    void print_queue() const {
        for (EvictedItem & item : queue) {
            std::cout << item.key<<"\t"<<item.reason << std::endl;
        }
    }
    EvictedItem* find(const std::string& key) {
        for (auto& item : queue) {
            if (item.key == key) {
                return &item; // Return pointer to the found item
            }
        }
        return nullptr; // Not found
    }


    size_t size() const {
        return current_size;
    }
    void clear() {
        queue.clear();
        current_size = 0;
    }
};



class SessionStorage {
protected:
    int8_t eviction_queue_threshold = 5; // Threshold for eviction (0-100)
    EvictionQueue<EvictedItem> eviction_queue{eviction_queue_threshold}; // Queue to track evicted items
    int8_t storage_capacity;
    std::list<CacheNode> cache_list; // List to maintain LRU order
    std::unordered_map<std::string, std::list<CacheNode>::iterator> cache_map; // Hash map for O(1) access

public:
    virtual ~SessionStorage() = default;
    virtual std::shared_ptr<SessionContext> get(const std::string& key) = 0;
    virtual void put(const std::string& key, std::shared_ptr<SessionContext> value) = 0;
    virtual void remove(const std::string& key, EvictionReason reason) = 0;
    virtual void clear() = 0;
    virtual vector<CacheNode> get_cache() = 0;
    virtual EvictedItem* get_evicted_item(const std::string& key) {
            EvictedItem* item = eviction_queue.find(key);
            if (item != nullptr) {
                return item; // Return pointer to the evicted item
            }
        return nullptr; // Not found
    }
};

#endif // SESSION_STORAGE_H
    