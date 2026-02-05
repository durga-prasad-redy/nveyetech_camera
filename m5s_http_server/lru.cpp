#include "lru.h"
#include <iostream>
#include <functional>
#include <cstring>

// --- Internal utility functions ---
static unsigned int hash_function(const std::string &key)
{
    std::hash<std::string> hasher;
    return hasher(key) % HASH_TABLE_SIZE;
}

static CacheNode *create_cache_node(const std::string &key, SessionContext *value)
{
    CacheNode *node = new (std::nothrow) CacheNode();
    if (!node)
        return nullptr;

    node->key = key;
    node->value = value;
    node->prev = node->next = nullptr;
    return node;
}

static void free_cache_node(CacheNode *node)
{
    if (node)
    {
        delete node;
    }
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
    CacheNode *last_node = cache->tail->prev;
    remove_node(last_node);
    return last_node;
}

static void add_to_eviction_queue(LRUCache *cache, const std::string &key, SessionContext *value,EvictionReason reason)
{
    EvictedItem *item = new (std::nothrow) EvictedItem();
    if (!item)
        return;

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

    if (cache->eviction_count > cache->eviction_queue_size)
    {
        EvictedItem *old = cache->eviction_head;
        cache->eviction_head = old->next;
        if (!cache->eviction_head)
            cache->eviction_tail = nullptr;
        delete old;
        cache->eviction_count--;
    }
}

static void hash_put(LRUCache *cache, const std::string &key, CacheNode *node)
{
    unsigned int index = hash_function(key);
    HashEntry *entry = new (std::nothrow) HashEntry();
    if (!entry)
        return;

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
            delete entry;
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

    LRUCache *cache = new (std::nothrow) LRUCache();
    if (!cache)
        return nullptr;

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

    cache->head = new (std::nothrow) CacheNode();
    cache->tail = new (std::nothrow) CacheNode();

    if (!cache->head || !cache->tail)
    {
        delete cache->head;
        delete cache->tail;
        delete cache;
        return nullptr;
    }

    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;
    cache->head->prev = nullptr;
    cache->tail->next = nullptr;

    return cache;
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
        CacheNode *new_node = create_cache_node(key, value);
        if (!new_node)
            return;

        if (cache->size == cache->capacity)
        {
            CacheNode *tail = pop_tail(cache);
            add_to_eviction_queue(cache, tail->key, tail->value,CAPACITY_LIMIT);
            hash_remove(cache, tail->key);
            free_cache_node(tail);
            cache->size--;
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
            add_to_eviction_queue(cache, tail->key, tail->value,CRITICAL_ACTION);
            printf("evicting key:%s\n", tail->key.c_str());
            hash_remove(cache, tail->key);
            free_cache_node(tail);
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

    CacheNode *current = cache->head->next;
    while (current != cache->tail)
    {
        CacheNode *next = current->next;
        free_cache_node(current);
        current = next;
    }

    delete cache->head;
    delete cache->tail;

    for (int i = 0; i < HASH_TABLE_SIZE; ++i)
    {
        HashEntry *entry = cache->hash_table[i];
        while (entry)
        {
            HashEntry *next = entry->next;
            delete entry;
            entry = next;
        }
    }

    EvictedItem *evicted = cache->eviction_head;
    while (evicted)
    {
        EvictedItem *next = evicted->next;
        delete evicted;
        evicted = next;
    }

    delete cache;
}
