// include/concurrent_hashmap/thread_partitioned.hpp

#pragma once

#include "base_hashmap.hpp"
#include <vector>
#include <array>
#include <atomic>
#include <thread>
#include <functional>
#include <algorithm>

namespace concurrent {

/**
 * Thread-partitioned concurrent hashmap implementation.
 * 
 * Each thread is assigned specific submaps for writing, but any thread
 * can read from any submap without synchronization.
 */
template <typename Key, typename Value, 
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class thread_partitioned_hashmap : public base_hashmap<Key, Value> {
private:
    // Forward declarations
    struct Entry;
    struct Submap;
    
    // Number of submaps - typically a power of 2
    static constexpr size_t NUM_SUBMAPS = 32;
    
    // Entry in the hashmap
    struct Entry {
        std::atomic<bool> occupied{false};
        Key key;
        std::atomic<Value> value;
    };
    
    // Submap (partition of the full map)
    struct Submap {
        std::vector<Entry> entries;
        std::atomic<size_t> size{0};
        std::atomic<bool> is_resizing{false};
        float max_load_factor = 0.7f;
        
        Submap(size_t initial_capacity = 8) {
            entries.resize(initial_capacity);
        }
        
        size_t capacity() const { return entries.size(); }
        float load_factor() const { 
            return size.load() > 0 ? 
                static_cast<float>(size) / capacity() : 0.0f; 
        }
        
        bool needs_resize() const {
            return load_factor() > max_load_factor;
        }
    };
    
    // Array of submaps
    std::array<Submap, NUM_SUBMAPS> submaps;
    std::atomic<size_t> total_size{0};
    
    // Hash function and key equality
    Hash hasher;
    KeyEqual key_equal;
    float overall_max_load_factor = 0.7f;
    
    // Calculate which submap a key belongs to
    size_t get_submap_index(const Key& key) const {
        return hasher(key) % NUM_SUBMAPS;
    }
    
    // Calculate entry index within a submap
    size_t get_entry_index(const Key& key, size_t submap_size) const {
        return hasher(key) % submap_size;
    }
    
    // Get thread's assigned submaps
    std::vector<size_t> get_thread_submaps() const {
        std::vector<size_t> result;
        size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
        size_t num_threads = std::thread::hardware_concurrency();
        num_threads = std::max(num_threads, static_cast<unsigned>(1));
        
        // Each thread is responsible for NUM_SUBMAPS / num_threads submaps
        size_t submaps_per_thread = NUM_SUBMAPS / num_threads;
        if (submaps_per_thread == 0) submaps_per_thread = 1;
        
        size_t start_idx = (thread_id % num_threads) * submaps_per_thread;
        
        for (size_t i = 0; i < submaps_per_thread && start_idx + i < NUM_SUBMAPS; ++i) {
            result.push_back((start_idx + i) % NUM_SUBMAPS);
        }
        return result;
    }
    
    // Resize a submap
    void resize_submap(size_t submap_idx) {
        Submap& submap = submaps[submap_idx];
        
        // Check if already resizing or no longer needs resize
        if (submap.is_resizing.exchange(true) || !submap.needs_resize()) {
            submap.is_resizing.store(false);
            return;
        }
        
        // Create new entries with double capacity
        size_t new_capacity = submap.capacity() * 2;
        std::vector<Entry> new_entries(new_capacity);
        
        // Rehash all entries
        for (const auto& entry : submap.entries) {
            if (!entry.occupied.load()) continue;
            
            // Find new position
            size_t idx = get_entry_index(entry.key, new_capacity);
            size_t probe = 0;
            
            while (new_entries[idx].occupied.load()) {
                probe++;
                idx = (idx + probe) % new_capacity;
            }
            
            // Copy to new position
            new_entries[idx].key = entry.key;
            new_entries[idx].value.store(entry.value.load());
            new_entries[idx].occupied.store(true);
        }
        
        // Swap in new entries
        submap.entries = std::move(new_entries);
        submap.is_resizing.store(false);
    }

public:
    thread_partitioned_hashmap() {
        // Initialize submaps with staggered sizes to avoid synchronized resizing
        for (size_t i = 0; i < NUM_SUBMAPS; ++i) {
            submaps[i] = Submap(8 + i % 8);  // Slight variation in initial sizes
        }
    }
    
    // Insert with lvalue references
    bool insert(const Key& key, const Value& value) override {
        return insert_impl(key, value);
    }
    
    // Insert with rvalue references
    bool insert(Key&& key, Value&& value) override {
        return insert_impl(std::forward<Key>(key), std::forward<Value>(value));
    }
    
    // Implementation of insert
    template <typename K, typename V>
    bool insert_impl(K&& key, V&& value) {
        size_t submap_idx = get_submap_index(key);
        auto thread_submaps = get_thread_submaps();
        
        // Check if this thread is responsible for the submap
        if (std::find(thread_submaps.begin(), thread_submaps.end(), submap_idx) 
                == thread_submaps.end()) {
            return false;  // Not this thread's responsibility
        }
        
        Submap& submap = submaps[submap_idx];
        
        // Handle resizing if needed
        if (submap.needs_resize()) {
            resize_submap(submap_idx);
        }
        
        // Find slot using linear probing
        size_t idx = get_entry_index(key, submap.capacity());
        size_t initial_idx = idx;
        size_t probe = 0;
        
        while (true) {
            // Try to find an empty slot or matching key
            if (!submap.entries[idx].occupied.load()) {
                // Found empty slot - try to claim it
                bool expected = false;
                if (submap.entries[idx].occupied.compare_exchange_strong(expected, true)) {
                    // Successfully claimed this slot, now initialize it
                    submap.entries[idx].key = std::forward<K>(key);
                    submap.entries[idx].value.store(std::forward<V>(value));
                    
                    // Increment sizes
                    submap.size.fetch_add(1);
                    total_size.fetch_add(1);
                    return true;
                }
                // Someone else claimed this slot, continue probing
            }
            
            // Check if this is an update to existing key
            if (submap.entries[idx].occupied.load() && 
                key_equal(submap.entries[idx].key, key)) {
                // Update existing entry
                submap.entries[idx].value.store(std::forward<V>(value));
                return true;
            }
            
            // Continue probing
            probe++;
            idx = (initial_idx + probe) % submap.capacity();
            
            // Avoid infinite loop
            if (probe >= submap.capacity()) {
                // Emergency resize if we've probed the entire table
                resize_submap(submap_idx);
                // Restart insertion
                return insert_impl(std::forward<K>(key), std::forward<V>(value));
            }
        }
    }
    
    // Find a value by key
    std::optional<Value> find(const Key& key) const override {
        size_t submap_idx = get_submap_index(key);
        const Submap& submap = submaps[submap_idx];
        
        // Find slot using linear probing
        size_t idx = get_entry_index(key, submap.capacity());
        size_t initial_idx = idx;
        size_t probe = 0;
        
        while (probe < submap.capacity()) {
            if (!submap.entries[idx].occupied.load()) {
                // Found empty slot, key doesn't exist
                return std::nullopt;
            }
            
            if (key_equal(submap.entries[idx].key, key)) {
                // Found the key
                return submap.entries[idx].value.load();
            }
            
            // Continue probing
            probe++;
            idx = (initial_idx + probe) % submap.capacity();
        }
        
        return std::nullopt;  // Not found after checking entire submap
    }
    
    // Check if key exists
    bool contains(const Key& key) const override {
        return find(key).has_value();
    }
    
    // Erase a key
    bool erase(const Key& key) override {
        size_t submap_idx = get_submap_index(key);
        auto thread_submaps = get_thread_submaps();
        
        // Check if this thread is responsible for the submap
        if (std::find(thread_submaps.begin(), thread_submaps.end(), submap_idx) 
                == thread_submaps.end()) {
            return false;  // Not this thread's responsibility
        }
        
        Submap& submap = submaps[submap_idx];
        
        // Find slot using linear probing
        size_t idx = get_entry_index(key, submap.capacity());
        size_t initial_idx = idx;
        size_t probe = 0;
        
        while (probe < submap.capacity()) {
            if (!submap.entries[idx].occupied.load()) {
                // Found empty slot, key doesn't exist
                return false;
            }
            
            if (key_equal(submap.entries[idx].key, key)) {
                // Found the key, mark as unoccupied
                bool expected = true;
                if (submap.entries[idx].occupied.compare_exchange_strong(expected, false)) {
                    // Successfully marked as unoccupied
                    submap.size.fetch_sub(1);
                    total_size.fetch_sub(1);
                    return true;
                }
                return false;  // Someone else modified this slot
            }
            
            // Continue probing
            probe++;
            idx = (initial_idx + probe) % submap.capacity();
        }
        
        return false;  // Not found after checking entire submap
    }
    
    // Get total size
    size_t size() const override {
        return total_size.load();
    }
    
    // Clear the hashmap
    void clear() override {
        for (auto& submap : submaps) {
            for (auto& entry : submap.entries) {
                entry.occupied.store(false);
            }
            submap.size.store(0);
        }
        total_size.store(0);
    }
    
    // Get total number of buckets
    size_t bucket_count() const override {
        size_t count = 0;
        for (const auto& submap : submaps) {
            count += submap.capacity();
        }
        return count;
    }
    
    // Get overall load factor
    float load_factor() const override {
        size_t count = bucket_count();
        return count > 0 ? static_cast<float>(size()) / count : 0.0f;
    }
    
    // Set max load factor
    void max_load_factor(float ml) override {
        overall_max_load_factor = ml;
        for (auto& submap : submaps) {
            submap.max_load_factor = ml;
        }
    }
};

} // namespace concurrent