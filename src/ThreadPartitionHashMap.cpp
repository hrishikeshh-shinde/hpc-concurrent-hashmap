// src/ThreadPartitionHashMap.cpp

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
     // Ensure NUM_SUBMAPS is not zero if it wasn't constexpr or checked elsewhere
     // if (NUM_SUBMAPS == 0) throw std::logic_error("NUM_SUBMAPS cannot be zero.");
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

// --- Destructor ---
ThreadPartitionHashMap::~ThreadPartitionHashMap() = default; // Default destructor is sufficient

// --- Private Helper: Resize Submap ---
// Assumes called only by the owner thread for this submap_idx
void ThreadPartitionHashMap::resize_submap(size_t submap_idx) {
    Submap& submap = submaps[submap_idx]; // Direct access ok - owner thread calls

    size_t current_capacity = submap.capacity();
    // size_t current_size = submap.size; // Not needed directly for resize logic

    size_t new_capacity = (current_capacity == 0) ? 8 : current_capacity * 2;
    std::vector<Entry> new_entries(new_capacity);
    size_t new_size_after_rehash = 0; // Track size during rehash

    // Rehash elements from old vector to new vector
    for (const auto& entry : submap.entries) {
        // Only move occupied entries
        if (entry.state.load(std::memory_order_relaxed) == EntryState::OCCUPIED) {
            size_t initial_idx = hasher(entry.key) % new_capacity;
            size_t probe = 0;
            size_t idx = initial_idx;

            // Find empty slot in new table using quadratic probing
            while (new_entries[idx].state.load(std::memory_order_relaxed) != EntryState::EMPTY) {
                probe++;
                // Quadratic probing: idx = (initial_idx + (probe^2 + probe) / 2) % new_capacity
                idx = (initial_idx + (probe * probe + probe) / 2) % new_capacity;
                if (probe > new_capacity) { // Safety check
                   // This should ideally not happen if load factor < 1
                   throw std::runtime_error("Resize failed: Could not find empty slot during rehash. Capacity: " + std::to_string(new_capacity));
                }
            }
            // Move the entry to the new slot. Use const_cast carefully.
            new_entries[idx] = std::move(const_cast<Entry&>(entry));
            // Important: Ensure the moved entry state remains OCCUPIED
            // Although move constructor should handle this, explicit store might be safer if state wasn't atomic before
            // new_entries[idx].state.store(EntryState::OCCUPIED, std::memory_order_relaxed);
            new_size_after_rehash++;
        }
    }

    // Replace old vector with the new one
    submap.entries = std::move(new_entries);
    // Update the submap's non-atomic size
    submap.size = new_size_after_rehash;
    // Note: total_size is NOT adjusted here, only element count within submap changes.
}


// --- Public Interface Methods ---
// Application MUST ensure these are called only by the owner thread.

bool ThreadPartitionHashMap::insert(std::string key) {
    // Application MUST ensure this is called only by the owner thread.
    size_t submap_idx = get_submap_index(key);
    Submap& submap = submaps[submap_idx]; // Direct access ok - owner thread calls

    // --- Check resize necessity ---
    size_t current_capacity = submap.capacity();
    size_t current_size = submap.size; // Non-atomic read
    float required_load_factor = this->getLoadFactor();

    // Trigger resize if load factor exceeded OR if capacity is 0
    if (current_capacity == 0 || (current_size >= current_capacity * required_load_factor)) {
         // Check if adding one more element would exceed load factor
        if (current_capacity == 0 || (static_cast<float>(current_size + 1) / current_capacity > required_load_factor)) {
             resize_submap(submap_idx); // Call private helper
             current_capacity = submap.capacity(); // Update capacity after resize
        }
    }
     // Defensive check: If capacity is still 0 after attempting resize, something is wrong.
     if (current_capacity == 0) {
         throw std::runtime_error("Insertion failed: Capacity is zero even after resize attempt.");
     }


    // --- Insertion logic (quadratic probing) ---
    std::vector<Entry>& current_entries = submap.entries; // Get reference
    // Use key_copy for hashing if key might be moved later
    const std::string key_copy_for_hash = key; // Make const
    size_t initial_idx = hasher(key_copy_for_hash) % current_capacity;
    size_t probe = 0;
    size_t first_deleted_slot = std::numeric_limits<size_t>::max(); // Sentinel

    while (probe < current_capacity) { // Limit probing to capacity to prevent infinite loops
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        Entry& current_entry_ref = current_entries[idx];
        EntryState current_state = current_entry_ref.state.load(std::memory_order_relaxed);

        if (current_state == EntryState::EMPTY) {
            // Found an empty slot. Use it, or the first deleted slot if found earlier.
            size_t insert_idx = (first_deleted_slot != std::numeric_limits<size_t>::max()) ? first_deleted_slot : idx;

            Entry& target_entry_ref = current_entries[insert_idx];
            // Ensure the target slot isn't somehow occupied (defensive check)
            if (target_entry_ref.state.load(std::memory_order_relaxed) == EntryState::OCCUPIED) {
                 // This indicates a logic error if it happens
                 throw std::logic_error("Insertion error: Target slot unexpectedly occupied.");
            }

            target_entry_ref.key = std::move(key); // Move the original key
            target_entry_ref.state.store(EntryState::OCCUPIED, std::memory_order_relaxed);
            submap.size++; // Increment non-atomic submap size
            total_size.fetch_add(1, std::memory_order_relaxed); // Increment atomic total size
            return true; // Insertion successful

        } else if (current_state == EntryState::DELETED) {
            // Found a deleted slot. Record the first one encountered.
            if (first_deleted_slot == std::numeric_limits<size_t>::max()) {
                first_deleted_slot = idx;
            }
            // Continue probing to check for duplicates or an empty slot.

        } else { // EntryState::OCCUPIED
            // Check if the key already exists at this occupied slot
            if (current_entry_ref.key == key_copy_for_hash) {
                return false; // Key already exists, insertion failed
            }
            // Otherwise, continue probing.
        }

        // Increment probe count for the next iteration
        probe++;

    } // End while loop

    // --- Loop finished without finding an EMPTY slot ---
    // This means we probed all 'current_capacity' slots.
    // If we found a deleted slot during the probe, we MUST use it now.
    if (first_deleted_slot != std::numeric_limits<size_t>::max()) {
        Entry& target_entry_ref = current_entries[first_deleted_slot];
        // Ensure it's still marked DELETED (defensive check)
        if (target_entry_ref.state.load(std::memory_order_relaxed) != EntryState::DELETED) {
             throw std::logic_error("Insertion error: Target deleted slot state changed unexpectedly.");
        }
        target_entry_ref.key = std::move(key); // Move the original key
        target_entry_ref.state.store(EntryState::OCCUPIED, std::memory_order_relaxed);
        submap.size++;
        total_size.fetch_add(1, std::memory_order_relaxed);
        return true; // Insertion successful into deleted slot
    }

    // --- If we reach here, the loop finished, and no EMPTY or DELETED slot was usable ---
    // This implies the table is genuinely full according to the probing sequence.
    throw std::runtime_error("Insertion failed: Hash table is full or probing failed to find a usable slot. Capacity: " + std::to_string(current_capacity) + ", Size: " + std::to_string(current_size));
}


bool ThreadPartitionHashMap::search(std::string key) const {
    // Application MUST ensure this is called only by the owner thread.
    size_t submap_idx = get_submap_index(key);
    const Submap& submap = submaps[submap_idx]; // Direct access ok

    size_t current_capacity = submap.capacity();
    if (current_capacity == 0) return false;

    const std::vector<Entry>& current_entries = submap.entries;
    const std::string& key_ref = key; // Use reference for hashing/comparison
    size_t initial_idx = hasher(key_ref) % current_capacity;
    size_t probe = 0;

    // Limit probing to capacity
    while (probe < current_capacity) {
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        const Entry& current_entry_ref = current_entries[idx]; // Use const ref
        EntryState current_state = current_entry_ref.state.load(std::memory_order_relaxed);

        if (current_state == EntryState::EMPTY) {
             return false; // Found empty slot, key cannot be further in probe sequence
        }
        if (current_state == EntryState::OCCUPIED && current_entry_ref.key == key_ref) {
             return true; // Found the key
        }
        // If state is DELETED or OCCUPIED but key doesn't match, continue probing

        probe++;
    }
    // Probed entire table capacity, key not found
    return false;
}


bool ThreadPartitionHashMap::remove(std::string key) {
    // Application MUST ensure this is called only by the owner thread.
    size_t submap_idx = get_submap_index(key);
    Submap& submap = submaps[submap_idx]; // Direct access ok

    size_t current_capacity = submap.capacity();
    if (current_capacity == 0) return false;

    std::vector<Entry>& current_entries = submap.entries;
    const std::string& key_ref = key; // Use reference
    size_t initial_idx = hasher(key_ref) % current_capacity;
    size_t probe = 0;

    // Limit probing to capacity
    while (probe < current_capacity) {
        size_t idx = (initial_idx + (probe * probe + probe) / 2) % current_capacity;
        Entry& current_entry_ref = current_entries[idx];
        EntryState current_state = current_entry_ref.state.load(std::memory_order_relaxed);

        if (current_state == EntryState::EMPTY) {
            return false; // Found empty slot, key cannot be further
        }

        if (current_state == EntryState::OCCUPIED && current_entry_ref.key == key_ref) {
            // Found the key, mark as deleted (tombstone)
            current_entry_ref.state.store(EntryState::DELETED, std::memory_order_relaxed);
            // Optional: Clear the key data for security/memory reasons
            // current_entry_ref.key.clear();
            // current_entry_ref.key.shrink_to_fit();

            submap.size--; // Decrement non-atomic submap size
            total_size.fetch_sub(1, std::memory_order_relaxed); // Decrement atomic total size
            return true; // Removal successful
        }
        // If state is DELETED or OCCUPIED but key doesn't match, continue probing

        probe++;
    }
     // Probed entire table capacity, key not found
    return false;
}

int ThreadPartitionHashMap::size() const {
    // Read the atomic total size. Relaxed order is usually sufficient for size().
    size_t current_total_size = total_size.load(std::memory_order_relaxed);
    // Handle potential overflow if size_t exceeds int max, though unlikely here.
    if (current_total_size > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(current_total_size);
}

void ThreadPartitionHashMap::rehash() {
    // Global rehash is not supported in this model.
    // Local resizing happens automatically within insert().
    // This function does nothing, as per the AbstractHashMap interface requirement.
}

