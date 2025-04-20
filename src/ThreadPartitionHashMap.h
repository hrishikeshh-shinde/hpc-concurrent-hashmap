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
#include <string>
#include <shared_mutex> 

enum class EntryState : uint8_t {
    EMPTY,
    OCCUPIED,
    DELETED // Tombstone
};


class ThreadPartitionHashMap : public AbstractHashMap {
private:
    static constexpr size_t NUM_SUBMAPS = 32; 

    struct Entry {
        std::atomic<EntryState> state;  
        std::string key;

        Entry() : state(EntryState::EMPTY) {}


        Entry(const Entry& other) = delete;
        Entry& operator=(const Entry& other) = delete;

        Entry(Entry&& other) noexcept : state(other.state.load(std::memory_order_relaxed)), key(std::move(other.key)) {}

        Entry& operator=(Entry&& other) noexcept {
            if (this != &other) {
                state.store(other.state.load(std::memory_order_relaxed));
                key = std::move(other.key);
            }
            return *this;
        }
    };

   
    class Submap {
    

    public: 
        ThreadPartitionHashMap* parent; 
        std::vector<Entry> entries;
        std::atomic<size_t> size;
        mutable std::shared_mutex rw_mutex;

         Submap() : parent(nullptr), size(0) {
             entries.resize(8);
         }


        Submap(ThreadPartitionHashMap* p, size_t initial_capacity = 8) 
            : parent(p), size(0) {
             entries.resize(initial_capacity > 0 ? initial_capacity : 8);
        }

        Submap(const Submap& other) = delete;
        Submap& operator=(const Submap& other) = delete;


        Submap(Submap&& other) noexcept
        : parent(other.parent),                
          entries(std::move(other.entries)),    
          size(other.size.load(std::memory_order_relaxed)) 
          {
            other.parent = nullptr;
        }

        Submap& operator=(Submap&& other) noexcept {
            if (this != &other) {
                std::unique_lock lhs_lock(rw_mutex, std::defer_lock);
                std::unique_lock rhs_lock(other.rw_mutex, std::defer_lock);
                std::lock(lhs_lock, rhs_lock); 

                parent = other.parent;
                entries = std::move(other.entries);
                size.store(other.size.load(std::memory_order_relaxed));

                other.parent = nullptr;
            }
            return *this;
        }


         size_t capacity_unsafe() const {
             return entries.size();
         }

    };

    std::array<Submap, NUM_SUBMAPS> submaps;
    std::atomic<size_t> total_size; 

    std::hash<std::string> hasher;


    size_t get_submap_index(const std::string& key) const;
    void resize_submap(size_t submap_idx);

public:

    ThreadPartitionHashMap(float loadFactor = 0.7f); 

    bool insert(std::string key) override;

    bool search(const std::string& key) const override;

    bool remove(const std::string& key) override; 

    int size() const override;

    void rehash() override;

    ~ThreadPartitionHashMap() override;

};

#endif // THREAD_PARTITION_HASH_MAP_H