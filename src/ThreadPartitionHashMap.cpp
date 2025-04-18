#include "ThreadPartitionHashMap.h"
#include <algorithm>

// Helper methods implementation
size_t ThreadPartitionHashMap::get_submap_index(const std::string& key) const {
    return hasher(key) % NUM_SUBMAPS;
}

size_t ThreadPartitionHashMap::get_entry_index(const std::string& key, size_t submap_size) const {
    return hasher(key) % submap_size;
}

std::vector<size_t> ThreadPartitionHashMap::get_thread_submaps() const {
    std::vector<size_t> result;
    size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    size_t num_threads = std::thread::hardware_concurrency();
    
    // Fix: Use the same type for both arguments to std::max
    num_threads = std::max(num_threads, static_cast<size_t>(1));
    
    // Each thread is responsible for NUM_SUBMAPS / num_threads submaps
    size_t submaps_per_thread = NUM_SUBMAPS / num_threads;
    if (submaps_per_thread == 0) submaps_per_thread = 1;
    
    size_t start_idx = (thread_id % num_threads) * submaps_per_thread;
    
    for (size_t i = 0; i < submaps_per_thread && start_idx + i < NUM_SUBMAPS; ++i) {
        result.push_back((start_idx + i) % NUM_SUBMAPS);
    }
    return result;
}

void ThreadPartitionHashMap::resize_submap(size_t submap_idx) {
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
        new_entries[idx].occupied.store(true);
    }
    
    // Swap in new entries
    submap.entries = std::move(new_entries);
    submap.is_resizing.store(false);
}

// Constructor
ThreadPartitionHashMap::ThreadPartitionHashMap(float loadFactor) 
    : AbstractHashMap(loadFactor), total_size(0) {
    // Initialize submaps individually
    for (size_t i = 0; i < NUM_SUBMAPS; ++i) {
        // Create and initialize each submap directly
        submaps[i] = Submap(this, 8 + i % 8);
    }
}

bool ThreadPartitionHashMap::insert(std::string key) {
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
                submap.entries[idx].key = std::move(key);
                
                // Increment sizes
                submap.size.fetch_add(1);
                total_size.fetch_add(1);
                count++; // Update AbstractHashMap's count
                return true;
            }
            // Someone else claimed this slot, continue probing
        }
        
        // Check if this is an update to existing key
        if (submap.entries[idx].occupied.load() && 
            submap.entries[idx].key == key) {
            // Key already exists
            return false;
        }
        
        // Continue probing
        probe++;
        idx = (initial_idx + probe) % submap.capacity();
        
        // Avoid infinite loop
        if (probe >= submap.capacity()) {
            // Emergency resize if we've probed the entire table
            resize_submap(submap_idx);
            // Restart insertion
            return insert(std::move(key));
        }
    }
}

bool ThreadPartitionHashMap::search(std::string key) const {
    size_t submap_idx = get_submap_index(key);
    const Submap& submap = submaps[submap_idx];
    
    // Find slot using linear probing
    size_t idx = get_entry_index(key, submap.capacity());
    size_t initial_idx = idx;
    size_t probe = 0;
    
    while (probe < submap.capacity()) {
        if (!submap.entries[idx].occupied.load()) {
            // Found empty slot, key doesn't exist
            return false;
        }
        
        if (submap.entries[idx].key == key) {
            // Found the key
            return true;
        }
        
        // Continue probing
        probe++;
        idx = (initial_idx + probe) % submap.capacity();
    }
    
    return false;  // Not found after checking entire submap
}

bool ThreadPartitionHashMap::remove(std::string key) {
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
        
        if (submap.entries[idx].key == key) {
            // Found the key, mark as unoccupied
            bool expected = true;
            if (submap.entries[idx].occupied.compare_exchange_strong(expected, false)) {
                // Successfully marked as unoccupied
                submap.size.fetch_sub(1);
                total_size.fetch_sub(1);
                count--; // Update AbstractHashMap's count
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

int ThreadPartitionHashMap::size() const {
    return count;
}

void ThreadPartitionHashMap::rehash() {
    for (size_t i = 0; i < NUM_SUBMAPS; ++i) {
        resize_submap(i);
    }
}

ThreadPartitionHashMap::~ThreadPartitionHashMap() {
    // No need for explicit resource cleanup
}