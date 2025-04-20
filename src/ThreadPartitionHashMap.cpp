

#include "ThreadPartitionHashMap.h" // Use the final fixed header
#include <vector>
#include <string>
#include <atomic>
#include <thread>       // <<< Added for std::this_thread::get_id
#include <mutex>        // For locks
#include <shared_mutex> // For locks
#include <stdexcept>    // For std::runtime_error
#include <cmath>
#include <functional>   // For std::hash
#include <limits>       // For std::numeric_limits
#include <iostream>     // <<< Added for std::cout
#include <sstream>      // <<< Added for std::stringstream

// Helper to get thread id as string
namespace { // Use anonymous namespace for internal linkage
    std::string tid_to_string() {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    }
} // anonymous namespace

// Helper methods implementation
size_t ThreadPartitionHashMap::get_submap_index(const std::string& key) const {
     // Assuming std::hash<std::string> hasher is a member variable
     return hasher(key) % NUM_SUBMAPS;
}

// Resize function - acquires unique lock internally
void ThreadPartitionHashMap::resize_submap(size_t submap_idx) {
    Submap& submap = submaps[submap_idx];
    std::string tid = tid_to_string(); // Get thread ID string

    std::unique_lock write_lock(submap.rw_mutex);

    size_t current_capacity = submap.capacity_unsafe(); // Read capacity under lock

    float parent_load_factor = 0.7f; // Default load factor
    AbstractHashMap* p_map = submap.parent;
    if (p_map != nullptr) {
         parent_load_factor = p_map->getLoadFactor();
    } else {
         parent_load_factor = this->getLoadFactor();
    }

    // --- Resize logic ---
    size_t new_capacity = (current_capacity == 0) ? 8 : current_capacity * 2;
    std::vector<Entry> new_entries(new_capacity);
    size_t new_size = 0;

    // Rehash all entries
    for (const auto& entry : submap.entries) {
        if (entry.state.load(std::memory_order_relaxed) == EntryState::OCCUPIED) {
            size_t initial_idx = hasher(entry.key) % new_capacity;
            size_t probe = 0;
            size_t idx = initial_idx;

            while (new_entries[idx].state.load(std::memory_order_relaxed) != EntryState::EMPTY) {
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
    submap.size.store(new_size, std::memory_order_relaxed);
}

// Constructor
ThreadPartitionHashMap::ThreadPartitionHashMap(float loadFactorParam)
    : AbstractHashMap(loadFactorParam), total_size(0) {
    std::string tid = tid_to_string();
    for (size_t i = 0; i < NUM_SUBMAPS; ++i) {
         submaps[i] = Submap(this);
    }
}

// Insert uses unique lock for the target submap
bool ThreadPartitionHashMap::insert(std::string key) {
    size_t submap_idx = get_submap_index(key);
    Submap& submap = submaps[submap_idx];
    std::string tid = tid_to_string();
    std::string key_copy_for_log = key;

    int attempt = 0;
    while (true) {
        attempt++;
        std::unique_lock write_lock(submap.rw_mutex);

        // --- Check resize necessity (under lock) ---
        size_t current_capacity = submap.capacity_unsafe();
        size_t current_size = submap.size.load(std::memory_order_relaxed);

        float parent_load_factor = 0.7f;
        AbstractHashMap* p_map = submap.parent;
         if (p_map != nullptr) {
            parent_load_factor = p_map->getLoadFactor();
        } else {
             parent_load_factor = this->getLoadFactor();
        }

        bool needs_resize = (current_capacity == 0 || static_cast<float>(current_size + 1) / current_capacity > parent_load_factor);

        if (needs_resize) {
            write_lock.unlock(); 

            resize_submap(submap_idx);
            continue; 
        }

        std::vector<Entry>& current_entries = submap.entries;
        size_t initial_idx = hasher(key) % current_capacity; // Use key before potential move
        size_t probe = 0;
        size_t first_deleted_slot = std::numeric_limits<size_t>::max();

        while (true) {
            size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
            Entry& current_entry_ref = current_entries[idx];
            EntryState current_state = current_entry_ref.state.load(std::memory_order_relaxed);

            if (current_state == EntryState::EMPTY || current_state == EntryState::DELETED) {
                size_t insert_idx = idx;
                if (current_state == EntryState::EMPTY && first_deleted_slot != std::numeric_limits<size_t>::max()) {
                    insert_idx = first_deleted_slot;
                } else if (current_state == EntryState::DELETED && first_deleted_slot == std::numeric_limits<size_t>::max()) {
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
                target_entry_ref.state.store(EntryState::OCCUPIED, std::memory_order_relaxed);

                submap.size.fetch_add(1, std::memory_order_relaxed);
                total_size.fetch_add(1, std::memory_order_relaxed);
                return true; 

            } else { 
                if (current_entry_ref.key == key) { 
                    return false; 
                }
            }

            probe++;
            if (probe >= current_capacity) {
                 throw std::runtime_error("Table became unexpectedly full during insert probe.");
            }
        } 
    } 
}


bool ThreadPartitionHashMap::search(std::string key) const {
    size_t submap_idx = get_submap_index(key);
    const Submap& submap = submaps[submap_idx];
    std::string tid = tid_to_string();

    std::shared_lock read_lock(submap.rw_mutex);

    size_t current_capacity = submap.capacity_unsafe();
    if (current_capacity == 0) {
         return false;
    }

    const std::vector<Entry>& current_entries = submap.entries;
    size_t initial_idx = hasher(key) % current_capacity;
    size_t probe = 0;

    while (true) {
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        EntryState current_state = current_entries[idx].state.load(std::memory_order_relaxed);

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

bool ThreadPartitionHashMap::remove(std::string key) {
    size_t submap_idx = get_submap_index(key);
    Submap& submap = submaps[submap_idx];
    std::string tid = tid_to_string();
    std::string key_copy_for_log = key; 

    std::unique_lock write_lock(submap.rw_mutex);

    size_t current_capacity = submap.capacity_unsafe();
    if (current_capacity == 0) {
        return false;
    }

    std::vector<Entry>& current_entries = submap.entries;
    size_t initial_idx = hasher(key) % current_capacity;
    size_t probe = 0;

    while (true) {
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        Entry& current_entry_ref = current_entries[idx];
        EntryState current_state = current_entry_ref.state.load(std::memory_order_relaxed);

        if (current_state == EntryState::EMPTY) {
            return false; 
        }

        if (current_state == EntryState::OCCUPIED && current_entry_ref.key == key) {
            current_entry_ref.state.store(EntryState::DELETED, std::memory_order_relaxed);


            submap.size.fetch_sub(1, std::memory_order_relaxed);
            total_size.fetch_sub(1, std::memory_order_relaxed);
            return true; 
        }

        probe++;
        if (probe >= current_capacity) {
            return false; 
        }
    }
}

int ThreadPartitionHashMap::size() const {
    std::string tid = tid_to_string();
    size_t current_total_size = total_size.load(std::memory_order_relaxed);
    return static_cast<int>(current_total_size);
}

void ThreadPartitionHashMap::rehash() {
     std::string tid = tid_to_string();
    for (size_t i = 0; i < NUM_SUBMAPS; ++i) {
         resize_submap(i); 
    }
}

// Destructor
ThreadPartitionHashMap::~ThreadPartitionHashMap() {   
}
