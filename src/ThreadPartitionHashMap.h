#ifndef THREAD_PARTITION_HASH_MAP_H
#define THREAD_PARTITION_HASH_MAP_H

#include "AbstractHashMap.h"
#include <vector>
#include <array>
#include <atomic>
#include <thread>
#include <functional>
#include <algorithm>

/**
 * Thread-partitioned hashmap implementation that extends AbstractHashMap.
 * 
 * Each thread operates on its own mini hashmap partition, enabling concurrent
 * operations with minimal synchronization overhead.
 */
class ThreadPartitionHashMap : public AbstractHashMap {
private:
    // Number of submaps - typically a power of 2
    static constexpr size_t NUM_SUBMAPS = 32;
    
    // Entry in the hashmap
    struct Entry {
        std::atomic<bool> occupied{false};
        std::string key;
    };
    
    // Submap (partition of the full map)
    struct Submap {
        std::vector<Entry> entries;
        std::atomic<size_t> size{0};
        std::atomic<bool> is_resizing{false};
        
        Submap(size_t initial_capacity = 8);
        size_t capacity() const;
        float load_factor() const;
        bool needs_resize() const;
    };
    
    // Array of submaps
    std::array<Submap, NUM_SUBMAPS> submaps;
    std::atomic<size_t> total_size{0};
    
    // Hash function
    std::hash<std::string> hasher;
    
    // Calculate which submap a key belongs to
    size_t get_submap_index(const std::string& key) const;
    
    // Calculate entry index within a submap
    size_t get_entry_index(const std::string& key, size_t submap_size) const;
    
    // Get thread's assigned submaps
    std::vector<size_t> get_thread_submaps() const;
    
    // Resize a specific submap
    void resize_submap(size_t submap_idx);

public:
    // Constructor with default load factor
    ThreadPartitionHashMap(float loadFactor = 0.7);
    
    // Insert a key into the hashmap
    bool insert(std::string key) override;
    
    // Search for a key in the hashmap
    bool search(std::string key) const override;
    
    // Remove a key from the hashmap
    bool remove(std::string key) override;
    
    // Get the total number of keys in the hashmap
    int size() const override;
    
    // Rehash the entire hashmap
    void rehash() override;
    
    // Destructor
    ~ThreadPartitionHashMap() override;
};

#endif // THREAD_PARTITION_HASH_MAP_H