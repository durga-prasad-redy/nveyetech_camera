

#include "fifo_queue.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

FIFOQueue::FIFOQueue(int cap) : storage_capacity(cap) 
{
    namespace fs = std::filesystem;

    for (int i = 0; i < capacity; ++i) 
    {
        std::string filename = SESSION_TOKEN_BASE_PATH + "session_" + std::to_string(i);

        if (!fs::exists(filename))
            continue;

        std::ifstream file(filename);
        if (!file.is_open())
            continue;

        std::string key;
        std::int32_t session_id;
        std::int64_t created_epoch;
        std::int64_t last_accessed_epoch;

        if (!(file >> session_id))
            continue;

        if (!(file >> created_epoch))
            continue;

        if (!(file >> last_accessed_epoch))
            continue;

        if (!(file >> key))
            continue;

        auto ctx = std::make_shared<SessionContext>();
        ctx->session_id = session_id;
        ctx->created_at =
            std::chrono::system_clock::time_point(
                std::chrono::seconds(created_epoch));

        ctx->last_accessed =
            std::chrono::system_clock::time_point(
                std::chrono::seconds(last_accessed_epoch));

        CacheNode node{key, ctx};

        cache_list.push_back(node);
        
        cache_map[key] = --cache_list.end();
    }
}

std::shared_ptr<SessionContext> FIFOQueue::get(std::string key) {
    auto it = cache_map.find(key);
    if (it == cache_map.end()) {
        return nullptr; // Not found
    }
    // update last accessed
    it->second->value->last_accessed = std::chrono::system_clock::now();
    
    // Move the accessed node to the front of the list
    return it->second->value;
}
void FIFOQueue::put(const std::string &key, std::shared_ptr<SessionContext> value)
{
    auto it = cache_map.find(key);
    
    // Set last accessed time for the new/updated value
    value->last_accessed = std::chrono::system_clock::now();

    if (it != cache_map.end()) {
        // Update existing node
        it->second->value = value;

        // Overwrite file
        write_session_to_file(key, value.get());
        return;
    }

    // Insert new node
    cache_list.emplace_front(CacheNode{key, value});
    cache_map[key] = cache_list.begin();

    write_session_to_file(key, value.get());

    // FIFO eviction
    if (cache_map.size() > storage_capacity) {
        auto last = cache_list.end();
        --last;
        //add the element to the eviction queue
        eviction_queue.push({last->key, last->value, EvictionReason::CAPACITY_LIMIT});
        // Delete file before removing
        delete_session_file(last->key);

        cache_map.erase(last->key);
        cache_list.pop_back();
    }
}
void FIFOQueue::remove(const std::string& key,EvictionReason reason)
{
    auto it = cache_map.find(key);
    //add the element to the eviction queue
    eviction_queue.push({key, it->second->value,reason});

    if (it != cache_map.end()) {
        delete_session_file(key);
        cache_list.erase(it->second);
        cache_map.erase(it);
    }
}

void FIFOQueue::clear()
{
    for (const auto& node : cache_list) {
        delete_session_file(node.key);
        eviction_queue.push({node.key, node.value, EvictionReason::FORCE_LOGOUT});
    }

    cache_list.clear();
    cache_map.clear();
    eviction_queue.clear();
}
namespace fs = std::filesystem;

std::string FIFOQueue::get_session_file_path(const std::string& key) {
    return SESSION_TOKEN_BASE_PATH + key;
}

vector<CacheNode> FIFOQueue::get_cache() {
    return cache_list;
}


void FIFOQueue::write_session_to_file(const std::string& key,
                                      const SessionContext* ctx)
{
    fs::create_directories(SESSION_TOKEN_BASE_PATH);

    std::string path = get_session_file_path(key);
    std::ofstream file(path, std::ios::trunc);

    if (!file.is_open())
        return;

    auto created_epoch =
        std::chrono::duration_cast<std::chrono::seconds>(
            ctx->created_at.time_since_epoch()).count();

    auto last_accessed_epoch =
        std::chrono::duration_cast<std::chrono::seconds>(
            ctx->last_accessed.time_since_epoch()).count();

    file << ctx->session_id << "\n";
    file << created_epoch << "\n";
    file << last_accessed_epoch << "\n";
}


void FIFOQueue::delete_session_file(const std::string& key)
{
    std::string path = get_session_file_path(key);

    if (fs::exists(path))
        fs::remove(path);
}
