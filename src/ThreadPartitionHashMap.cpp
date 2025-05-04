#include "ThreadPartitionHashMap.h"
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <stdexcept>
#include <cmath>
#include <functional>
#include <limits>
#include <iostream>
#include <sstream>

// --- Helper Function Implementations ---

size_t ThreadPartitionHashMap::get_submap_index(const std::string& key) const {
     return hasher(key) % NUM_SUBMAPS;
}

size_t ThreadPartitionHashMap::get_owner_thread_index(size_t submap_idx) const {
    if (num_worker_threads_ == 0) {
        throw std::logic_error("ThreadPartitionHashMap constructed with zero worker threads.");
    }
    return submap_idx % num_worker_threads_;
}

// --- Constructor ---
ThreadPartitionHashMap::ThreadPartitionHashMap(size_t num_worker_threads, float loadFactorParam)
    : AbstractHashMap(loadFactorParam),
      num_worker_threads_(num_worker_threads),
      total_size(0)
{
    if (num_worker_threads_ == 0) {
         throw std::invalid_argument("Number of worker threads cannot be zero.");
    }
    for (size_t i = 0; i < NUM_SUBMAPS; ++i) {
         submaps[i] = Submap(this);
    }
}

ThreadPartitionHashMap::~ThreadPartitionHashMap() = default; 

void ThreadPartitionHashMap::resize_submap(size_t submap_idx) {
    Submap& submap = submaps[submap_idx]; 

    size_t current_capacity = submap.capacity();

    size_t new_capacity = (current_capacity == 0) ? 8 : current_capacity * 2;
    std::vector<Entry> new_entries(new_capacity);
    size_t new_size_after_rehash = 0; 

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
            new_size_after_rehash++;
        }
    }

    submap.entries = std::move(new_entries);
    submap.size = new_size_after_rehash;
}



bool ThreadPartitionHashMap::insert(std::string key) {
    size_t submap_idx = get_submap_index(key);
    Submap& submap = submaps[submap_idx]; 

    // --- Check resize necessity ---
    size_t current_capacity = submap.capacity();
    size_t current_size = submap.size; 
    float required_load_factor = this->getLoadFactor();

    // Trigger resize if load factor exceeded OR if capacity is 0
    if (current_capacity == 0 || (static_cast<float>(current_size + 1) / current_capacity > required_load_factor)) {
        resize_submap(submap_idx); 
        current_capacity = submap.capacity(); 
    }

    std::vector<Entry>& current_entries = submap.entries; 

    std::string key_copy_for_hash = key;
    size_t initial_idx = hasher(key_copy_for_hash) % current_capacity;
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
                if (probe >= current_capacity) { insert_idx = first_deleted_slot; }
                else { continue; } 
            }

            Entry& target_entry_ref = current_entries[insert_idx];
             if (target_entry_ref.state.load(std::memory_order_relaxed) == EntryState::OCCUPIED && target_entry_ref.key == key_copy_for_hash) {
                 return false;
             }

            target_entry_ref.key = std::move(key); 
            target_entry_ref.state.store(EntryState::OCCUPIED, std::memory_order_relaxed);

            submap.size++; 
            total_size.fetch_add(1, std::memory_order_relaxed);

        } else {
            if (current_entry_ref.key == key_copy_for_hash) {
                return false;
            }
        }

        probe++;
        if (probe >= current_capacity) {
             if (first_deleted_slot != std::numeric_limits<size_t>::max()) {
                 Entry& target_entry_ref = current_entries[first_deleted_slot];
                 target_entry_ref.key = std::move(key); 
                 target_entry_ref.state.store(EntryState::OCCUPIED, std::memory_order_relaxed);
                 submap.size++;
                 total_size.fetch_add(1, std::memory_order_relaxed);
                 return true;
             } else {
                 throw std::runtime_error("Insertion failed: Hash table is unexpectedly full or probing failed.");
             }
        }
    } 
}


bool ThreadPartitionHashMap::search(std::string key) const {
    size_t submap_idx = get_submap_index(key);

    size_t current_capacity = submap.capacity();
    if (current_capacity == 0) return false;

    const std::vector<Entry>& current_entries = submap.entries;
    size_t initial_idx = hasher(key) % current_capacity;
    size_t probe = 0;

    while (true) {
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        // Use relaxed memory order for state read
        EntryState current_state = current_entries[idx].state.load(std::memory_order_relaxed);

        if (current_state == EntryState::EMPTY) {
             return false; // Found empty slot, key cannot be further
        }
        if (current_state == EntryState::OCCUPIED && current_entries[idx].key == key) {
             return true; // Found the key
        }
        // If state is DELETED or OCCUPIED but key doesn't match, continue probing

        probe++;
        if (probe >= current_capacity) {
             return false; // Probed entire table, key not found
        }
    }
}


bool ThreadPartitionHashMap::remove(std::string key) {
    // Application MUST ensure this is called only by the owner thread.
    size_t submap_idx = get_submap_index(key);
    Submap& submap = submaps[submap_idx]; // Direct access ok

    size_t current_capacity = submap.capacity();
    if (current_capacity == 0) return false;

    std::vector<Entry>& current_entries = submap.entries;
    size_t initial_idx = hasher(key) % current_capacity;
    size_t probe = 0;

    while (true) {
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        Entry& current_entry_ref = current_entries[idx];
        EntryState current_state = current_entry_ref.state.load(std::memory_order_relaxed);

        if (current_state == EntryState::EMPTY) {
            return false; // Found empty slot, key cannot be further
        }

        if (current_state == EntryState::OCCUPIED && current_entry_ref.key == key) {
            // Found the key, mark as deleted (tombstone)
            current_entry_ref.state.store(EntryState::DELETED, std::memory_order_relaxed);


            submap.size--; // Decrement non-atomic submap size
            total_size.fetch_sub(1, std::memory_order_relaxed); // Decrement atomic total size
            return true; // Removal successful
        }

        probe++;
        if (probe >= current_capacity) {
            return false; 
        }
    }
}

int ThreadPartitionHashMap::size() const {
    size_t current_total_size = total_size.load(std::memory_order_relaxed);
    if (current_total_size > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(current_total_size);
}

void ThreadPartitionHashMap::rehash() {
    // Global rehash is not supported in this model.
    // Local resizing happens automatically within insert().
    // This function does nothing for now, as per the AbstractHashMap interface requirement.
}