
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <atomic>
#include <random>
#include <numeric>
#include <functional> // For std::ref
#include <unordered_map>
#include <stdexcept>

// Assuming your corrected header is named this
#include "ThreadPartitionHashMap.h"

// --- Configuration ---
const int PREPOPULATE_KEYS = 100000; // Number of keys to insert initially
const int TOTAL_OPERATIONS = 1000000; // Total operations per test run
const int READ_PERCENT = 80;   // Percentage of search operations
const int INSERT_PERCENT = 10; // Percentage of insert operations
// Remove percentage is implicitly (100 - READ - INSERT)

// --- Thread-Safe std::unordered_map Wrapper ---
// Uses shared_mutex for fair comparison with ThreadPartitionHashMap's locking
class LockedUnorderedMap {
private:
    std::unordered_map<std::string, bool> map_; // Using bool as dummy value
    mutable std::shared_mutex mutex_;

public:
    bool insert(const std::string& key) {
        std::unique_lock lock(mutex_);
        // try_emplace returns pair<iterator, bool>
        return map_.try_emplace(key, true).second;
    }

    bool search(const std::string& key) const {
        std::shared_lock lock(mutex_);
        return map_.count(key) > 0;
    }

    bool remove(const std::string& key) {
        std::unique_lock lock(mutex_);
        return map_.erase(key) > 0; // erase returns number of elements removed
    }

    size_t size() const {
         std::shared_lock lock(mutex_);
         return map_.size();
    }

    void clear() {
        std::unique_lock lock(mutex_);
        map_.clear();
    }
};

// --- Workload Generation ---
std::mt19937& get_rng_engine() {
    // Seed with a random device if possible, otherwise fallback
    static std::random_device rd;
    static std::mt19937 engine(rd());
    return engine;
}

std::string generate_key(int max_key) {
    // Generate keys like "key-N" where N is random
    std::uniform_int_distribution<int> dist(0, max_key - 1);
    return "key-" + std::to_string(dist(get_rng_engine()));
}

enum class OperationType { SEARCH, INSERT, REMOVE };

OperationType get_operation_type() {
    std::uniform_int_distribution<int> dist(1, 100);
    int rand_val = dist(get_rng_engine());
    if (rand_val <= READ_PERCENT) {
        return OperationType::SEARCH;
    } else if (rand_val <= READ_PERCENT + INSERT_PERCENT) {
        return OperationType::INSERT;
    } else {
        return OperationType::REMOVE;
    }
}

// --- Workload Execution Function ---
template <typename MapType>
void run_workload(MapType& map, int num_ops_per_thread, int key_range, std::atomic<long long>& total_time_ns) {
    long long thread_local_time_ns = 0;
    auto thread_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_ops_per_thread; ++i) {
        std::string key = generate_key(key_range);
        OperationType op = get_operation_type();

        // It's okay if operations sometimes fail (e.g., inserting existing key)
        // We are measuring the time taken to attempt the operations.
        switch(op) {
            case OperationType::SEARCH:
                map.search(key);
                break;
            case OperationType::INSERT:
                map.insert(key); // Pass key by value
                break;
            case OperationType::REMOVE:
                map.remove(key); // Pass key by value
                break;
        }
    }
    auto thread_end = std::chrono::high_resolution_clock::now();
    thread_local_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(thread_end - thread_start).count();
    total_time_ns.fetch_add(thread_local_time_ns);
}


// --- Test Functions ---

void run_comparison_test(int num_threads) {
    std::cout << "==== Running Comparison Test (" << num_threads << " threads) ====" << std::endl;

    ThreadPartitionHashMap my_map(0.7f);
    LockedUnorderedMap std_map_locked;

    // Pre-populate
    std::cout << "Pre-populating maps with " << PREPOPULATE_KEYS << " keys..." << std::endl;
    for (int i = 0; i < PREPOPULATE_KEYS; ++i) {
        std::string key = "key-" + std::to_string(i);
        my_map.insert(key); // Pass by value
        std_map_locked.insert(key);
    }
    std::cout << "Pre-population complete." << std::endl;

    std::atomic<long long> my_map_total_time(0);
    std::atomic<long long> std_map_total_time(0);
    std::vector<std::thread> threads;

    int ops_per_thread = TOTAL_OPERATIONS / num_threads;
    int key_range = PREPOPULATE_KEYS * 2; // Operate on keys in and out of initial set

    // --- Benchmark ThreadPartitionHashMap ---
    std::cout << "Benchmarking ThreadPartitionHashMap..." << std::endl;
    auto start1 = std::chrono::high_resolution_clock::now();
    threads.clear();
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(run_workload<ThreadPartitionHashMap>, std::ref(my_map), ops_per_thread, key_range, std::ref(my_map_total_time));
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
    double throughput1 = static_cast<double>(TOTAL_OPERATIONS) / (duration1_ms / 1000.0);


    // --- Benchmark LockedUnorderedMap ---
     std::cout << "Benchmarking LockedUnorderedMap..." << std::endl;
    auto start2 = std::chrono::high_resolution_clock::now();
    threads.clear();
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(run_workload<LockedUnorderedMap>, std::ref(std_map_locked), ops_per_thread, key_range, std::ref(std_map_total_time));
    }
    for (auto& t : threads) {
        t.join();
    }
     auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
    double throughput2 = static_cast<double>(TOTAL_OPERATIONS) / (duration2_ms / 1000.0);

    // --- Print Results ---
    std::cout << "--- Comparison Results (" << num_threads << " threads, " << TOTAL_OPERATIONS << " ops) ---" << std::endl;
    std::cout << "ThreadPartitionHashMap:" << std::endl;
    std::cout << "  Wall Time: " << duration1_ms << " ms" << std::endl;
    std::cout << "  Avg Thread Time: " << (my_map_total_time.load() / num_threads) / 1e6 << " ms" << std::endl; // Convert ns to ms
    std::cout << "  Throughput: " << throughput1 << " ops/sec" << std::endl;

    std::cout << "LockedUnorderedMap:" << std::endl;
    std::cout << "  Wall Time: " << duration2_ms << " ms" << std::endl;
     std::cout << "  Avg Thread Time: " << (std_map_total_time.load() / num_threads) / 1e6 << " ms" << std::endl; // Convert ns to ms
     std::cout << "  Throughput: " << throughput2 << " ops/sec" << std::endl;
    std::cout << "---------------------------------------------" << std::endl;

}


void run_scaling_test() {
     std::cout << "==== Running Scaling Test ====" << std::endl;

     std::vector<int> thread_counts = {1, 2, 4, 8};
     int max_threads = std::thread::hardware_concurrency();
     if (max_threads > 8 && std::find(thread_counts.begin(), thread_counts.end(), max_threads) == thread_counts.end()) {
         thread_counts.push_back(max_threads);
     }
     // Ensure thread counts are sorted if needed
     // std::sort(thread_counts.begin(), thread_counts.end());

     std::cout << "Thread Counts to Test: ";
     for(int tc : thread_counts) { std::cout << tc << " "; }
     std::cout << std::endl;


     for (int num_threads : thread_counts) {
         ThreadPartitionHashMap my_map(0.7f);

         // Pre-populate (same for each thread count)
         for (int i = 0; i < PREPOPULATE_KEYS; ++i) {
             my_map.insert("key-" + std::to_string(i));
         }

         std::atomic<long long> my_map_total_time(0);
         std::vector<std::thread> threads;
         int ops_per_thread = TOTAL_OPERATIONS / num_threads;
         int key_range = PREPOPULATE_KEYS * 2;

         auto start_time = std::chrono::high_resolution_clock::now();
         threads.clear();
          for (int i = 0; i < num_threads; ++i) {
              threads.emplace_back(run_workload<ThreadPartitionHashMap>, std::ref(my_map), ops_per_thread, key_range, std::ref(my_map_total_time));
          }
          for (auto& t : threads) {
              t.join();
          }
         auto end_time = std::chrono::high_resolution_clock::now();
         auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
         double throughput = (duration_ms > 0) ? (static_cast<double>(TOTAL_OPERATIONS) / (duration_ms / 1000.0)) : 0.0;

         std::cout << "--- Scaling Result (" << num_threads << " threads, " << TOTAL_OPERATIONS << " ops) ---" << std::endl;
         std::cout << "  Wall Time: " << duration_ms << " ms" << std::endl;
         if (num_threads > 0) {
            std::cout << "  Avg Thread Time: " << (my_map_total_time.load() / num_threads) / 1e6 << " ms" << std::endl;
         }
         std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;

     }
      std::cout << "---------------------------------------------" << std::endl;
}


int main() {
    std::cout << "Starting Hash Map Benchmarks..." << std::endl;

    int num_threads_for_comparison = std::thread::hardware_concurrency();
    // Ensure at least 1 thread for comparison
    if (num_threads_for_comparison == 0) num_threads_for_comparison = 1;

    run_comparison_test(num_threads_for_comparison);
    run_scaling_test();

    std::cout << "All benchmarks completed!" << std::endl;
    return 0;
}

