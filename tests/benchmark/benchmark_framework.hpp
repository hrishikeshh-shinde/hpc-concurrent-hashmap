// tests/benchmark/benchmark_framework.hpp

#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <random>
#include <thread>
#include <functional>
#include <memory>
#include <atomic>

namespace benchmark {

class timer {
private:
    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration = std::chrono::nanoseconds;
    
    time_point start_time;
    
public:
    void start() {
        start_time = clock::now();
    }
    
    double elapsed_ms() const {
        auto end_time = clock::now();
        auto elapsed = std::chrono::duration_cast<duration>(end_time - start_time);
        return elapsed.count() / 1000000.0;
    }
};

struct benchmark_result {
    std::string name;
    double elapsed_ms;
    size_t operations;
    size_t num_threads;
    
    double ops_per_second() const {
        return operations / (elapsed_ms / 1000.0);
    }
    
    void print() const {
        std::cout << std::left << std::setw(30) << name 
                  << " | Threads: " << std::setw(2) << num_threads
                  << " | Time: " << std::fixed << std::setprecision(2) << std::setw(10) << elapsed_ms << " ms"
                  << " | Ops: " << std::setw(10) << operations
                  << " | " << std::setw(12) << std::setprecision(2) << ops_per_second() << " ops/sec"
                  << std::endl;
    }
};

template <typename HashMapType>
class hashmap_benchmark {
private:
    std::string name;
    std::unique_ptr<HashMapType> hashmap;
    
public:
    hashmap_benchmark(const std::string& benchmark_name, std::unique_ptr<HashMapType> map)
        : name(benchmark_name), hashmap(std::move(map)) {}
    
    benchmark_result run_insert_benchmark(size_t num_operations, size_t num_threads) {
        timer t;
        std::vector<std::thread> threads;
        std::atomic<size_t> counter{0};
        
        // Generate keys and values in advance
        std::vector<int> keys(num_operations);
        std::mt19937 gen(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<int> dist(0, std::numeric_limits<int>::max());
        
        for (size_t i = 0; i < num_operations; ++i) {
            keys[i] = dist(gen);
        }
        
        t.start();
        
        auto thread_func = [&](size_t thread_id) {
            while (true) {
                size_t idx = counter.fetch_add(1);
                if (idx >= num_operations) break;
                
                int key = keys[idx];
                int value = idx;
                
                hashmap->insert(key, value);
            }
        };
        
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back(thread_func, i);
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        double elapsed = t.elapsed_ms();
        
        return {name + " [insert]", elapsed, num_operations, num_threads};
    }
    
    benchmark_result run_find_benchmark(double read_ratio, size_t num_operations, size_t num_threads) {
        timer t;
        std::vector<std::thread> threads;
        std::atomic<size_t> counter{0};
        std::atomic<size_t> finds_succeeded{0};
        
        // Pre-populate with half the operations
        std::vector<int> keys(num_operations);
        std::mt19937 gen(42);
        std::uniform_int_distribution<int> dist(0, std::numeric_limits<int>::max());
        
        size_t prepopulate_count = num_operations / 2;
        for (size_t i = 0; i < prepopulate_count; ++i) {
            keys[i] = dist(gen);
            hashmap->insert(keys[i], i);
        }
        
        // Generate the rest of the keys
        for (size_t i = prepopulate_count; i < num_operations; ++i) {
            keys[i] = dist(gen);
        }
        
        t.start();
        
        auto thread_func = [&](size_t thread_id) {
            std::mt19937 local_gen(42 + thread_id);
            std::uniform_real_distribution<double> op_dist(0.0, 1.0);
            
            while (true) {
                size_t idx = counter.fetch_add(1);
                if (idx >= num_operations) break;
                
                int key = keys[idx];
                double op_choice = op_dist(local_gen);
                
                if (op_choice < read_ratio) {
                    // Do a find operation
                    auto result = hashmap->find(key);
                    if (result.has_value()) {
                        finds_succeeded.fetch_add(1);
                    }
                } else {
                    // Do an insert operation
                    hashmap->insert(key, idx);
                }
            }
        };
        
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back(thread_func, i);
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        double elapsed = t.elapsed_ms();
        
        std::string op_desc = "mixed [" + std::to_string(int(read_ratio * 100)) + "% reads]";
        return {name + " " + op_desc, elapsed, num_operations, num_threads};
    }
};

} // namespace benchmark