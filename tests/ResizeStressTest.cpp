#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <cassert>
#include <random> // For random keys in stress test
#include <chrono>
#include <numeric>
#include <functional> // For std::ref
#include <mutex>        // Added
#include <shared_mutex> // Added
#include <unordered_map>// Added

// Assuming your corrected header is named this
#include "ThreadPartitionHashMap.h"

// --- Configuration ---
const int RESIZE_NUM_THREADS = 8;
const int RESIZE_KEYS_PER_THREAD = 50000;
const float RESIZE_LOAD_FACTOR = 0.6f; // For ThreadPartitionHashMap

// --- Thread-Safe std::unordered_map Wrapper (Copied from BenchmarkTests) ---
class LockedUnorderedMap {
private:

    std::unordered_map<std::string, bool> map_;
    mutable std::shared_mutex mutex_;

public:
    bool insert(std::string key) {
        std::unique_lock lock(mutex_);
        return map_.try_emplace(std::move(key), true).second; // Use move
    }

    bool search(std::string key) const { 
        std::shared_lock lock(mutex_);
        return map_.count(key) > 0;
    }

    size_t size() const {
         std::shared_lock lock(mutex_);
         return map_.size();
    }
};


std::mt19937& get_resize_rng_engine() {
    static std::random_device rd_resize;
    static std::mt19937 engine_resize(rd_resize());
    return engine_resize;
}

std::string generate_resize_key(int thread_id, int key_idx) {
    return "rst-" + std::to_string(thread_id) + "-" + std::to_string(key_idx);
}


// --- Templated Thread Worker Function ---
template <typename MapType>
void stress_insert_thread_func(MapType& map, int thread_id, std::atomic<int>& success_counter) {
    int local_success = 0;
    for (int i = 0; i < RESIZE_KEYS_PER_THREAD; ++i) {
        std::string key = generate_resize_key(thread_id, i);
        try {
            // Pass key by value as required by insert signature now
            if (map.insert(key)) {
                 local_success++;
            }
        } catch (const std::exception& e) {
            std::cerr << "Thread " << thread_id << " caught exception during insert: " << e.what() << std::endl;
        }
    }
    success_counter.fetch_add(local_success, std::memory_order_relaxed);
}

void test_resize_stress_compare() {
    std::cout << "==== Running Resize Stress Test with Comparison (" << RESIZE_NUM_THREADS << " threads) ====" << std::endl;

    ThreadPartitionHashMap my_map(RESIZE_LOAD_FACTOR);
    LockedUnorderedMap std_map_locked;

    std::vector<std::thread> threads;
    std::atomic<int> my_map_success_count(0);
    std::atomic<int> std_map_success_count(0);

    std::cout << "--- Stressing ThreadPartitionHashMap ---" << std::endl;
    auto start1 = std::chrono::high_resolution_clock::now();
    threads.clear();
    for (int i = 0; i < RESIZE_NUM_THREADS; ++i) {
        threads.emplace_back(stress_insert_thread_func<ThreadPartitionHashMap>, std::ref(my_map), i, std::ref(my_map_success_count));
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
    std::cout << "ThreadPartitionHashMap insertion finished in " << duration1_ms << " ms." << std::endl;
    std::cout << "Successfully inserted keys (approx): " << my_map_success_count.load() << std::endl;
    std::cout << "Final reported map size: " << my_map.size() << std::endl; // Call size() after joins


    // --- Run on LockedUnorderedMap ---
    std::cout << "--- Stressing LockedUnorderedMap ---" << std::endl;
     auto start2 = std::chrono::high_resolution_clock::now();
    threads.clear();
    for (int i = 0; i < RESIZE_NUM_THREADS; ++i) {
        threads.emplace_back(stress_insert_thread_func<LockedUnorderedMap>, std::ref(std_map_locked), i, std::ref(std_map_success_count));
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
    std::cout << "LockedUnorderedMap insertion finished in " << duration2_ms << " ms." << std::endl;
    std::cout << "Successfully inserted keys (approx): " << std_map_success_count.load() << std::endl;
    std::cout << "Final reported map size: " << std_map_locked.size() << std::endl; // Call size() after joins


    // --- Verification  ---
    std::cout << "--- Verification Phase ---" << std::endl;
    bool my_map_ok = true;
    size_t my_map_found = 0;
    size_t std_map_found = 0;

    for (int t = 0; t < RESIZE_NUM_THREADS; ++t) {
        for (int i = 0; i < RESIZE_KEYS_PER_THREAD; ++i) {
             std::string key = generate_resize_key(t, i);
             if (my_map.search(key)) { my_map_found++; }
             if (std_map_locked.search(key)) { std_map_found++; }
        }
    }
    std::cout << "Verification Found (ThreadPartitionHashMap): " << my_map_found << " keys." << std::endl;
    std::cout << "Verification Found (LockedUnorderedMap): " << std_map_found << " keys." << std::endl;
    // Basic assertion: Check if found count matches success count (can have races if not careful)
    assert(static_cast<size_t>(my_map_success_count.load()) == my_map_found && "Mismatch between successful inserts and found keys in custom map!");
    assert(static_cast<size_t>(std_map_success_count.load()) == std_map_found && "Mismatch between successful inserts and found keys in standard map!");


    std::cout << "Resize stress test with comparison passed!" << std::endl;
    std::cout << "---------------------------------------------" << std::endl;
}


int main() {
    test_resize_stress_compare();
    return 0;
}
