#include "ThreadPartitionHashMap.h"

// Submap implementation
ThreadPartitionHashMap::Submap::Submap(size_t initial_capacity) {
    entries.resize(initial_capacity);
}

size_t ThreadPartitionHashMap::Submap::capacity() const {
    return entries.size();
}

float ThreadPartitionHashMap::Submap::load_factor() const {
    return size.load() > 0 ? static_cast<float>(size) / capacity() : 0.0f;
}

bool ThreadPartitionHashMap::Submap::needs_resize() const {
    return load_factor() > getLoadFactor();
}

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

void ThreadPartitionHashMap::resize_submap(size_t submap_idx) {
    Submap& submap = submaps[submap_idx];
    
    if (submap.is_resizing.exchange(true) || !submap.needs_resize()) {
        submap.is_resizing.store(false);
        return;
    }
    
    size_t new_capacity = submap.capacity() * 2;
    std::vector<Entry> new_entries(new_capacity);
    
    for (const auto& entry : submap.entries) {
        if (!entry.occupied.load()) continue;
        
        size_t idx = get_entry_index(entry.key, new_capacity);
        size_t probe = 0;
        
        while (new_entries[idx].occupied.load()) {
            probe++;
            idx = (idx + probe) % new_capacity;
        }
        
        new_entries[idx].key = entry.key;
        new_entries[idx].occupied.store(true);
    }
    
    submap.entries = std::move(new_entries);
    submap.is_resizing.store(false);
}

// Constructor
ThreadPartitionHashMap::ThreadPartitionHashMap(float loadFactor) : AbstractHashMap(loadFactor) {
    for (size_t i = 0; i < NUM_SUBMAPS; ++i) {
        submaps[i] = Submap(8 + i % 8); 
}

// Public methods implementation
bool ThreadPartitionHashMap::insert(std::string key) {
    size_t submap_idx = get_submap_index(key);
    auto thread_submaps = get_thread_submaps();
    
    if (std::find(thread_submaps.begin(), thread_submaps.end(), submap_idx) 
            == thread_submaps.end()) {
        return false;
    }
    
    Submap& submap = submaps[submap_idx];
    
    if (submap.needs_resize()) {
        resize_submap(submap_idx);
    }
    
    // Find slot using linear probing
    size_t idx = get_entry_index(key, submap.capacity());
    size_t initial_idx = idx;
    size_t probe = 0;
    
    while (true) {
        if (!submap.entries[idx].occupied.load()) {
            bool expected = false;
            if (submap.entries[idx].occupied.compare_exchange_strong(expected, true)) {
                submap.entries[idx].key = std::move(key);
                
                // Increment sizes
                submap.size.fetch_add(1);
                total_size.fetch_add(1);
                count++; 
                return true;
            }
        }
        
        if (submap.entries[idx].occupied.load() && 
            submap.entries[idx].key == key) {
            return false;
        }
        
        probe++;
        idx = (initial_idx + probe) % submap.capacity();
        
        if (probe >= submap.capacity()) {
            resize_submap(submap_idx);
            return insert(std::move(key));
        }
    }
}

bool ThreadPartitionHashMap::search(std::string key) const {
    size_t submap_idx = get_submap_index(key);
    const Submap& submap = submaps[submap_idx];
    
    size_t idx = get_entry_index(key, submap.capacity());
    size_t initial_idx = idx;
    size_t probe = 0;
    
    while (probe < submap.capacity()) {
        if (!submap.entries[idx].occupied.load()) {
            return false;
        }
        
        if (submap.entries[idx].key == key) {
            return true;
        }
        
        probe++;
        idx = (initial_idx + probe) % submap.capacity();
    }
    
    return false;  
}

bool ThreadPartitionHashMap::remove(std::string key) {
    size_t submap_idx = get_submap_index(key);
    auto thread_submaps = get_thread_submaps();
    
    if (std::find(thread_submaps.begin(), thread_submaps.end(), submap_idx) 
            == thread_submaps.end()) {
        return false; 
    }
    
    Submap& submap = submaps[submap_idx];
    
    size_t idx = get_entry_index(key, submap.capacity());
    size_t initial_idx = idx;
    size_t probe = 0;
    
    while (probe < submap.capacity()) {
        if (!submap.entries[idx].occupied.load()) {
            return false;
        }
        
        if (submap.entries[idx].key == key) {
            bool expected = true;
            if (submap.entries[idx].occupied.compare_exchange_strong(expected, false)) {
                submap.size.fetch_sub(1);
                total_size.fetch_sub(1);
                count--;
                return true;
            }
            return false; 
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