
#include "generic_lru.h"

LRUCache::LRUCache(int cap) : storage_capacity(cap) {}

std::shared_ptr<SessionContext> LRUCache::get(const std::string &key) {
    auto it = cache_map.find(key);
    if (it == cache_map.end()) {
        return nullptr; // Not found
    }
    // update last accessed
    it->second->value->last_accessed = std::chrono::system_clock::now();
    
    // Move the accessed node to the front of the list
    cache_list.splice(cache_list.begin(), cache_list, it->second);
    return it->second->value;
}

void LRUCache::put(const std::string &key, std::shared_ptr<SessionContext> value) {
    // Set last accessed time
    value->last_accessed = std::chrono::system_clock::now();

    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        // Update existing node and move to front
        it->second->value = value;
        cache_list.splice(cache_list.begin(), cache_list, it->second);
    } else {
        // Insert new node at the front
        cache_list.emplace_front(CacheNode{key, value});
        cache_map[key] = cache_list.begin();

        // Evict least recently used item if capacity exceeded
        if (cache_map.size() > storage_capacity) {
            auto last = cache_list.end();
            --last;
            eviction_queue.push({last->key, last->value,EvictionReason::CAPACITY_LIMIT});
            cache_map.erase(last->key);
            cache_list.pop_back();
        }
    }
}

void LRUCache::remove(const std::string &key,EvictionReason reason) {
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        eviction_queue.push({key, it->second->value,reason});
        cache_list.erase(it->second);
        cache_map.erase(it);
    }
}

void LRUCache::clear() {
    for (const auto& node : cache_list) {
        eviction_queue.push({node.key, node.value,EvictionReason::FORCE_LOGOUT});
    }
    cache_list.clear();
    cache_map.clear();
    eviction_queue.clear();
}

vector<CacheNode> LRUCache::get_cache() {
    return cache_list;  
}

