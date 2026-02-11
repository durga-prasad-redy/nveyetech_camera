#include "lru.h"
#include <iostream>
#include <functional>
#include <cstring>
#include <memory>

// --- Internal utility functions ---
static unsigned int hash_function(const std::string &key)
{
    std::hash<std::string> hasher;
    return hasher(key) % HASH_TABLE_SIZE;
}

static CacheNode *create_cache_node(LRUCache *cache, const std::string &key, SessionContext *value)
{
    if (!cache)
        return nullptr;

    cache->nodes_storage.emplace_back(std::make_unique<CacheNode>());
    auto it = --cache->nodes_storage.end();
    CacheNode *node = it->get();
    cache->nodes_index[node] = it;

    node->key = key;
    node->value = value;
    node->prev = node->next = nullptr;
    return node;
}

static void add_to_head(LRUCache *cache, CacheNode *node)
{
    node->prev = cache->head;
    node->next = cache->head->next;
    cache->head->next->prev = node;
    cache->head->next = node;
}

static void remove_node(CacheNode *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static void move_to_head(LRUCache *cache, CacheNode *node)
{
    remove_node(node);
    add_to_head(cache, node);
}

static CacheNode *pop_tail(LRUCache *cache)
{
    if (!cache || !cache->tail)
        return nullptr;
    
    CacheNode *last_node = cache->tail->prev;
    if (!last_node || last_node == cache->head)
        return nullptr;
    
    // Ensure the node has valid prev/next pointers before removing
    if (!last_node->prev || !last_node->next)
        return nullptr;
    
    remove_node(last_node);
    return last_node;
}

static void add_to_eviction_queue(LRUCache *cache, const std::string &key, SessionContext *value,EvictionReason reason)
{
    if (!cache || !value)
        return;

    cache->evicted_storage.emplace_back(std::make_unique<EvictedItem>());
    auto it = --cache->evicted_storage.end();
    EvictedItem *item = it->get();
    cache->evicted_index[item] = it;

    item->key = key;
    item->value = *value;
    item->reason = reason;
    item->next = nullptr;

    if (cache->eviction_tail)
    {
        cache->eviction_tail->next = item;
    }
    else
    {
        cache->eviction_head = item;
    }
    cache->eviction_tail = item;
    cache->eviction_count++;

    if (cache->eviction_count > cache->eviction_queue_size && cache->eviction_head)
    {
        EvictedItem *old = cache->eviction_head;
        cache->eviction_head = old->next;
        if (!cache->eviction_head)
            cache->eviction_tail = nullptr;

        auto idxIt = cache->evicted_index.find(old);
        if (idxIt != cache->evicted_index.end())
        {
            cache->evicted_storage.erase(idxIt->second);
            cache->evicted_index.erase(idxIt);
        }

        cache->eviction_count--;
    }
}

static void hash_put(LRUCache *cache, const std::string &key, CacheNode *node)
{
    unsigned int index = hash_function(key);
    cache->hashentry_storage.emplace_back(std::make_unique<HashEntry>());
    auto it = --cache->hashentry_storage.end();
    HashEntry *entry = it->get();
    cache->hashentry_index[entry] = it;

    entry->node = node;
    entry->next = cache->hash_table[index];
    cache->hash_table[index] = entry;
}

static CacheNode *hash_get(LRUCache *cache, const std::string &key)
{
    unsigned int index = hash_function(key);
    HashEntry *entry = cache->hash_table[index];
    while (entry)
    {
        if (entry->node->key == key)
        {
            return entry->node;
        }
        entry = entry->next;
    }
    return nullptr;
}

void hash_remove(LRUCache *cache, const std::string &key)
{
    unsigned int index = hash_function(key);
    HashEntry *entry = cache->hash_table[index];
    HashEntry *prev = nullptr;

    while (entry)
    {
        if (entry->node->key == key)
        {
            if (prev)
            {
                prev->next = entry->next;
            }
            else
            {
                cache->hash_table[index] = entry->next;
            }

            auto it = cache->hashentry_index.find(entry);
            if (it != cache->hashentry_index.end())
            {
                cache->hashentry_storage.erase(it->second);
                cache->hashentry_index.erase(it);
            }
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

// --- Public API ---
LRUCache *lru_create(int capacity, int eviction_queue_size)
{
    if (capacity <= 0 || eviction_queue_size < 0)
        return nullptr;

    auto cache = std::make_unique<LRUCache>();

    cache->capacity = capacity;
    cache->size = 0;
    cache->eviction_queue_size = eviction_queue_size;
    cache->eviction_count = 0;
    cache->eviction_head = nullptr;
    cache->eviction_tail = nullptr;

    for (int i = 0; i < HASH_TABLE_SIZE; ++i)
    {
        cache->hash_table[i] = nullptr;
    }

    cache->head = create_cache_node(cache.get(), std::string(), nullptr);
    cache->tail = create_cache_node(cache.get(), std::string(), nullptr);

    if (!cache->head || !cache->tail)
    {
        return nullptr;
    }

    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;
    cache->head->prev = nullptr;
    cache->tail->next = nullptr;

    return cache.release();
}

void lru_put(LRUCache *cache, const std::string &key, SessionContext *value)
{
    if (!cache || key.empty())
        return;

    CacheNode *node = hash_get(cache, key);

    if (node)
    {
        node->value = value;
        move_to_head(cache, node);
    }
    else
    {
        CacheNode *new_node = create_cache_node(cache, key, value);
        if (!new_node)
            return;

        if (cache->size == cache->capacity)
        {
            CacheNode *tail = pop_tail(cache);
            if (tail)
            {
                add_to_eviction_queue(cache, tail->key, tail->value,CAPACITY_LIMIT);
                hash_remove(cache, tail->key);
                auto it = cache->nodes_index.find(tail);
                if (it != cache->nodes_index.end())
                {
                    cache->nodes_storage.erase(it->second);
                    cache->nodes_index.erase(it);
                }
                cache->size--;
            }
        }

        add_to_head(cache, new_node);
        hash_put(cache, key, new_node);
        cache->size++;
    }
}

void lru_evict_all(LRUCache *cache, const std::string &key, SessionContext *value)
{

    if (!cache || key.empty())
        return;

    CacheNode *node = hash_get(cache, key);
    if (!node)
    {
        printf("node with the key:%s dosent exist\n", key.c_str());
        return;
    }
    else
    {
        node->value = value;
        move_to_head(cache, node);

        while (cache->size > 1)
        {
            CacheNode *tail = pop_tail(cache);
            if (!tail)
                break;
            add_to_eviction_queue(cache, tail->key, tail->value,CRITICAL_ACTION);
            printf("evicting key:%s\n", tail->key.c_str());
            hash_remove(cache, tail->key);
            auto it = cache->nodes_index.find(tail);
            if (it != cache->nodes_index.end())
            {
                cache->nodes_storage.erase(it->second);
                cache->nodes_index.erase(it);
            }
            cache->size--;
        }
    }
    return ;
}

bool lru_get(LRUCache *cache, const std::string &key, SessionContext *&out_value)
{
    if (!cache || key.empty())
        return false;

    CacheNode *node = hash_get(cache, key);
    if (node)
    {
        out_value = node->value;
        move_to_head(cache, node);
        return true;
    }

    return false;
}

EvictedItem *lru_get_evicted_items(LRUCache *cache)
{
    if (!cache)
        return nullptr;
    return cache->eviction_head;
}

EvictedItem *lru_find_evicted(LRUCache *cache, const std::string &key)
{
    if (!cache || key.empty())
        return nullptr;

    EvictedItem *current = cache->eviction_head;
    while (current)
    {
        if (current->key == key)
        {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

void lru_free(LRUCache *cache)
{
    if (!cache)
        return;
    std::default_delete<LRUCache>{}(cache);
}
