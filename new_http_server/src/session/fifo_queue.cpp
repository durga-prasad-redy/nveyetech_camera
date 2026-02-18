#include "fifo_queue.h"
#include <algorithm> // For std::sort
#include <chrono>
#include <cstdio>   // For std::remove
#include <dirent.h> // For directory handling if needed
#include <fstream>
#include <sstream>
#include <sys/stat.h> // For stat (check file existence)

// Helper to check if file exists without std::filesystem
bool file_exists(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

// Helper to create directory (POSIX style)
void ensure_directory_exists(const std::string &path) {
  // Note: This creates a single level.
  // On Windows, use _mkdir(path.c_str()) from <direct.h>
  mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

FIFOQueue::FIFOQueue(int cap) : storage_capacity(cap) {
  ensure_directory_exists(SESSION_TOKEN_BASE_PATH);

  DIR *dir = opendir(SESSION_TOKEN_BASE_PATH.c_str());
  if (dir == nullptr)
    return;

  struct dirent *entry;
  std::vector<CacheNode> all_found_sessions;

  // 1. Scan directory for "stream_" files
  while ((entry = readdir(dir)) != nullptr) {
    std::string filename = entry->d_name;
    if (filename == "." || filename == "..") {
      continue;
    }

    std::string full_path = SESSION_TOKEN_BASE_PATH + filename;
    std::ifstream file(full_path);
    if (!file.is_open())
      continue;

    std::string key;
    std::int32_t session_id;
    std::int64_t created_epoch;

    if (!(file >> session_id >> created_epoch >> key))
      continue;

    auto ctx = std::make_shared<SessionContext>();
    ctx->session_id = session_id;
    ctx->created_at = std::chrono::system_clock::time_point(
        std::chrono::seconds(created_epoch));
    ctx->last_accessed = std::chrono::system_clock::now();

    all_found_sessions.push_back({key, ctx});
  }
  closedir(dir);

  // 2. Sort by created_at (Descending: newest first)
  std::sort(all_found_sessions.begin(), all_found_sessions.end(),
            [](const CacheNode &a, const CacheNode &b) {
              return a.value->created_at > b.value->created_at;
            });

  // 3. Load up to capacity, delete the rest
  for (size_t i = 0; i < all_found_sessions.size(); ++i) {
    if (i < (size_t)storage_capacity) {
      // Keep: Add to internal data structures
      cache_list.push_back(all_found_sessions[i]);
      cache_map[all_found_sessions[i].key] = --cache_list.end();
    } else {
      // Over capacity: Delete the file
      delete_session_file(all_found_sessions[i].key);
    }
  }
}

std::shared_ptr<SessionContext> FIFOQueue::get(const std::string &key) {
  auto it = cache_map.find(key);
  if (it == cache_map.end()) {
    return nullptr;
  }
  it->second->value->last_accessed = std::chrono::system_clock::now();
  return it->second->value;
}

void FIFOQueue::put(const std::string &key,
                    std::shared_ptr<SessionContext> value) {
  auto it = cache_map.find(key);
  value->last_accessed = std::chrono::system_clock::now();

  if (it != cache_map.end()) {
    it->second->value = value;
    write_session_to_file(key, value.get());
    return;
  }

  cache_list.emplace_front(CacheNode{key, value});
  cache_map[key] = cache_list.begin();

  write_session_to_file(key, value.get());

  if (cache_map.size() > (size_t)storage_capacity) {
    auto last = --cache_list.end();
    eviction_queue.push(
        {last->key, last->value, EvictionReason::CAPACITY_LIMIT});
    delete_session_file(last->key);
    cache_map.erase(last->key);
    cache_list.pop_back();
  }
}

void FIFOQueue::remove(const std::string &key, EvictionReason reason) {
  auto it = cache_map.find(key);
  if (it != cache_map.end()) {
    eviction_queue.push({key, it->second->value, reason});
    delete_session_file(key);
    cache_list.erase(it->second);
    cache_map.erase(it);
  }
}

void FIFOQueue::clear() {
  for (const auto &node : cache_list) {
    delete_session_file(node.key);
    eviction_queue.push({node.key, node.value, EvictionReason::FORCE_LOGOUT});
  }
  cache_list.clear();
  cache_map.clear();
  // Assuming eviction_queue is a custom object or std::queue (which doesn't
  // have .clear())
  while (!eviction_queue.empty())
    eviction_queue.pop();
}

std::string FIFOQueue::get_session_file_path(const std::string &key) {
  return SESSION_TOKEN_BASE_PATH + key;
}

std::list<CacheNode> FIFOQueue::get_cache() { return cache_list; }

void FIFOQueue::write_session_to_file(const std::string &key,
                                      const SessionContext *ctx) {
  ensure_directory_exists(SESSION_TOKEN_BASE_PATH);

  std::string path = get_session_file_path(key);
  std::ofstream file(path.c_str(), std::ios::trunc);

  if (!file.is_open())
    return;

  auto created_epoch = std::chrono::duration_cast<std::chrono::seconds>(
                           ctx->created_at.time_since_epoch())
                           .count();

  file << ctx->session_id << "\n";
  file << created_epoch << "\n";
  file << key
       << "\n"; // Added missing key write for consistency with constructor
}

void FIFOQueue::delete_session_file(const std::string &key) {
  std::string path = get_session_file_path(key);
  if (file_exists(path)) {
    std::remove(path.c_str());
  }
}

void FIFOQueue::print_session() {
  for (const auto &node : cache_list) {
    printf("Key: %s, Value: %p\n", node.key.c_str(), node.value.get());
  }
}