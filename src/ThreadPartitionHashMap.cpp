
#include "ThreadPartitionHashMap.h" 
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>   
#include <cmath>
#include <functional> 

size_t ThreadPartitionHashMap::get_submap_index(const std::string& key) const {
     return hasher(key) % NUM_SUBMAPS;
}

void ThreadPartitionHashMap::resize_submap(size_t submap_idx) {
    Submap& submap = submaps[submap_idx];

    std::unique_lock write_lock(submap.rw_mutex);

    size_t current_size = submap.size.load();
    size_t current_capacity = submap.capacity_unsafe(); 


    float parent_load_factor = 0.7f;
    if (submap.parent) {
         parent_load_factor = submap.parent->getLoadFactor(); // <<< Use getter
    }
    if (current_capacity > 0 && static_cast<float>(current_size) / current_capacity < parent_load_factor) {
       return; 
    }

    size_t new_capacity = (current_capacity == 0) ? 8 : current_capacity * 2;
    std::vector<Entry> new_entries(new_capacity);
    size_t new_size = 0;

    for (const auto& entry : submap.entries) {
        if (entry.state.load() == EntryState::OCCUPIED) {
            size_t initial_idx = hasher(entry.key) % new_capacity;
            size_t probe = 0;
            size_t idx = initial_idx;

            while (new_entries[idx].state.load() != EntryState::EMPTY) {
                probe++;
                idx = (initial_idx + (probe * probe + probe) / 2) % new_capacity;
                if (probe > new_capacity) {
                   throw std::runtime_error("Resize failed: Could not find empty slot during rehash."); 
                }
            }
            new_entries[idx] = std::move(const_cast<Entry&>(entry)); 

            new_size++;
        }
    }

    submap.entries = std::move(new_entries);
    submap.size.store(new_size); 
}

// Constructor
ThreadPartitionHashMap::ThreadPartitionHashMap(float loadFactorParam) {}
    : AbstractHashMap(loadFactorParam), total_size(0) { 
         submaps[i] = Submap(this); 
    }
}

bool ThreadPartitionHashMap::insert(std::string key) {
    size_t submap_idx = get_submap_index(key);
    Submap& submap = submaps[submap_idx];

    std::unique_lock write_lock(submap.rw_mutex);

    size_t current_size = submap.size.load();
    size_t current_capacity = submap.capacity_unsafe();

    float parent_load_factor = 0.7f;
    if (submap.parent) {
         parent_load_factor = submap.parent->getLoadFactor(); 
    }
    if (current_capacity == 0 || static_cast<float>(current_size + 1) / current_capacity > parent_load_factor) {
        resize_submap(submap_idx);
        current_capacity = submap.capacity_unsafe();
        current_size = submap.size.load();
        if (current_capacity == 0) {
             throw std::runtime_error("Resize failed during insert."); 
        }
    }

    // --- Perform insertion (under lock) ---
    std::vector<Entry>& current_entries = submap.entries;
    size_t initial_idx = hasher(key) % current_capacity; 
    size_t probe = 0;
    size_t first_deleted_slot = SIZE_MAX; 

    while (true) {
        // Quadratic probing
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        Entry& current_entry_ref = current_entries[idx]; 
        EntryState current_state = current_entry_ref.state.load();

        if (current_state == EntryState::EMPTY || current_state == EntryState::DELETED) {
            size_t insert_idx = idx;
            if (current_state == EntryState::EMPTY && first_deleted_slot != SIZE_MAX) {
                insert_idx = first_deleted_slot;
            } else if (current_state == EntryState::DELETED && first_deleted_slot == SIZE_MAX) {
                 first_deleted_slot = idx;
                 probe++; 
                 if (probe >= current_capacity) {
                      insert_idx = first_deleted_slot; 
                 } else {
                     continue;
                 }
            }

            Entry& target_entry_ref = current_entries[insert_idx];
            target_entry_ref.key = std::move(key);
            target_entry_ref.state.store(EntryState::OCCUPIED);

            submap.size.fetch_add(1, std::memory_order_relaxed);
            total_size.fetch_add(1, std::memory_order_relaxed);
            return true; // Inserted

        } else { 
            if (current_entry_ref.key == key) { 
                return false; 
            }
        }

        probe++;
        if (probe >= current_capacity) {
             throw std::runtime_error("Table became unexpectedly full during insert."); 
        }
    }
}

// Search uses shared lock for the target submap
bool ThreadPartitionHashMap::search(std::string key) const {
    size_t submap_idx = get_submap_index(key);
    const Submap& submap = submaps[submap_idx];

    // Acquire shared lock for the read operation
    std::shared_lock read_lock(submap.rw_mutex);

    size_t current_capacity = submap.capacity_unsafe();
    if (current_capacity == 0) return false;

    const std::vector<Entry>& current_entries = submap.entries;
    size_t initial_idx = hasher(key) % current_capacity;
    size_t probe = 0;

    while (true) {
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        EntryState current_state = current_entries[idx].state.load();

        if (current_state == EntryState::EMPTY) {
            return false; 
        }
        if (current_state == EntryState::OCCUPIED && current_entries[idx].key == key) { 
            return true; 
        }

        probe++;
        if (probe >= current_capacity) {
            return false; 
        }
    }
}

// Remove uses unique lock for the target submap
bool ThreadPartitionHashMap::remove(std::string key) {
    size_t submap_idx = get_submap_index(key);
    Submap& submap = submaps[submap_idx];

    std::unique_lock write_lock(submap.rw_mutex);

    size_t current_capacity = submap.capacity_unsafe();
    if (current_capacity == 0) return false;

    std::vector<Entry>& current_entries = submap.entries;
    size_t initial_idx = hasher(key) % current_capacity; 
    size_t probe = 0;

    while (true) {
        // Quadratic probing
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        Entry& current_entry_ref = current_entries[idx];
        EntryState current_state = current_entry_ref.state.load();

        if (current_state == EntryState::EMPTY) {
            return false;
        }

        if (current_state == EntryState::OCCUPIED && current_entry_ref.key == key) { 
            current_entry_ref.state.store(EntryState::DELETED);

            submap.size.fetch_sub(1, std::memory_order_relaxed);
            total_size.fetch_sub(1, std::memory_order_relaxed);
            return true; 
        }

        probe++;
        if (probe >= current_capacity) {
            return false; 
    }
}

int ThreadPartitionHashMap::size() const {
    return static_cast<int>(total_size.load(std::memory_order_relaxed));
}

void ThreadPartitionHashMap::rehash() {
    for (size_t i = 0; i < NUM_SUBMAPS; ++i) {
        resize_submap(i);
    }
}

// Destructor
ThreadPartitionHashMap::~ThreadPartitionHashMap() {
}
