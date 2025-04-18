// ThreadPartitionHashMap.h - Fixed version

#ifndef THREAD_PARTITION_HASH_MAP_H
#define THREAD_PARTITION_HASH_MAP_H

#include "AbstractHashMap.h"
#include <vector>
#include <array>
#include <atomic>
#include <thread>
#include <functional>
#include <algorithm>
#include <memory>

class ThreadPartitionHashMap : public AbstractHashMap {
private:
    // Number of submaps - typically a power of 2
    static constexpr size_t NUM_SUBMAPS = 32;
    
    // Entry in the hashmap - needs to be copyable/movable
    struct Entry {
        std::atomic<bool> occupied;
        std::string key;
        
        // Default constructor
        Entry() : occupied(false) {}
        
        // Copy constructor
        Entry(const Entry& other) : occupied(other.occupied.load()), key(other.key) {}
        
        // Move constructor
        Entry(Entry&& other) noexcept : occupied(other.occupied.load()), key(std::move(other.key)) {}
        
        // Copy assignment
        Entry& operator=(const Entry& other) {
            occupied.store(other.occupied.load());
            key = other.key;
            return *this;
        }
        
        // Move assignment
        Entry& operator=(Entry&& other) noexcept {
            occupied.store(other.occupied.load());
            key = std::move(other.key);
            return *this;
        }
    };
    
    // Forward declaration
    class Submap;
    
    // Submap (partition of the full map)
    class Submap {
    private:
        ThreadPartitionHashMap* parent;
        
    public:
        std::vector<Entry> entries;
        std::atomic<size_t> size;
        std::atomic<bool> is_resizing;
        
        // Default constructor
        Submap() : parent(nullptr), size(0), is_resizing(false) {}
        
        // Constructor with parent and initial capacity
        Submap(ThreadPartitionHashMap* p, size_t initial_capacity) 
            : parent(p), size(0), is_resizing(false) {
            entries.resize(initial_capacity);
        }
        
        // Copy constructor
        Submap(const Submap& other) 
            : parent(other.parent),
              size(other.size.load()),
              is_resizing(other.is_resizing.load()) {
            entries = other.entries;
        }
        
        // Move constructor 
        Submap(Submap&& other) noexcept
        : parent(other.parent),
          entries(std::move(other.entries)),  // match declaration order
          size(other.size.load()),
          is_resizing(other.is_resizing.load()) {
        other.parent = nullptr;
    }
        
        // Copy assignment operator
        Submap& operator=(const Submap& other) {
            if (this != &other) {
                parent = other.parent;
                size.store(other.size.load());
                is_resizing.store(other.is_resizing.load());
                entries = other.entries;
            }
            return *this;
        }
        
        // Move assignment operator
        Submap& operator=(Submap&& other) noexcept {
            if (this != &other) {
                parent = other.parent;
                size.store(other.size.load());
                is_resizing.store(other.is_resizing.load());
                entries = std::move(other.entries);
                other.parent = nullptr;
            }
            return *this;
        }
        
        size_t capacity() const {
            return entries.size();
        }
        
        float load_factor() const {
            return size.load() > 0 ? static_cast<float>(size.load()) / capacity() : 0.0f;
        }
        
        bool needs_resize() const {
            return load_factor() > (parent ? parent->getLoadFactor() : 0.7f);
        }
    };
    
    // Array of submaps
    std::array<Submap, NUM_SUBMAPS> submaps;
    std::atomic<size_t> total_size;
    
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