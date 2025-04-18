#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include "ThreadPartitionHashMap.h"

void test_read_heavy(int num_threads, int num_keys, int num_reads_per_key) {
    ThreadPartitionHashMap map(0.7);
    std::atomic<int> success_count(0);
    
    for (int i = 0; i < num_keys; i++) {
        map.insert("key-" + std::to_string(i));
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; t++) {
        threads.push_back(std::thread([&, t]() {
            int local_success = 0;
            
            for (int r = 0; r < num_reads_per_key; r++) {
                for (int i = 0; i < num_keys; i++) {
                    if (map.search("key-" + std::to_string(i))) {
                        local_success++;
                    }
                }
            }
            
            success_count += local_success;
        }));
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    int total_operations = num_threads * num_keys * num_reads_per_key;
    double ops_per_second = total_operations / (duration.count() / 1000.0);
    
    std::cout << "Read-heavy test results:" << std::endl;
    std::cout << "- Threads: " << num_threads << std::endl;
    std::cout << "- Total keys: " << num_keys << std::endl;
    std::cout << "- Reads per key: " << num_reads_per_key << std::endl;
    std::cout << "- Total read operations: " << total_operations << std::endl;
    std::cout << "- Successful reads: " << success_count << std::endl;
    std::cout << "- Time: " << duration.count() << " ms" << std::endl;
    std::cout << "- Throughput: " << ops_per_second << " ops/sec" << std::endl;
}

int main(int argc, char* argv[]) {
    int num_threads = std::thread::hardware_concurrency();
    int num_keys = 1000;
    int num_reads_per_key = 100;
    
    if (argc > 1) num_threads = std::stoi(argv[1]);
    if (argc > 2) num_keys = std::stoi(argv[2]);
    if (argc > 3) num_reads_per_key = std::stoi(argv[3]);
    
    test_read_heavy(num_threads, num_keys, num_reads_per_key);
    return 0;
}