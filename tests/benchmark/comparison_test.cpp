// tests/benchmark/comparison_test.cpp

#include "benchmark_framework.hpp"
#include "concurrent_hashmap/thread_partitioned.hpp"
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <memory>

// Wrapper for std::unordered_map to make it thread-safe with a single mutex
template <typename Key, typename Value>
class mutex_unordered_map : public concurrent::base_hashmap<Key, Value> {
private:
    std::unordered_map<Key, Value> map;
    mutable std::mutex mutex;
    
public:
    bool insert(const Key& key, const Value& value) override {
        std::lock_guard<std::mutex> lock(mutex);
        auto result = map.insert_or_assign(key, value);
        return result.second;
    }
    
    bool insert(Key&& key, Value&& value) override {
        std::lock_guard<std::mutex> lock(mutex);
        auto result = map.insert_or_assign(std::move(key), std::move(value));
        return result.second;
    }
    
    std::optional<Value> find(const Key& key) const override {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    bool erase(const Key& key) override {
        std::lock_guard<std::mutex> lock(mutex);
        return map.erase(key) > 0;
    }
    
    bool contains(const Key& key) const override {
        std::lock_guard<std::mutex> lock(mutex);
        return map.find(key) != map.end();
    }
    
    size_t size() const override {
        std::lock_guard<std::mutex> lock(mutex);
        return map.size();
    }
    
    void clear() override {
        std::lock_guard<std::mutex> lock(mutex);
        map.clear();
    }
    
    size_t bucket_count() const override {
        std::lock_guard<std::mutex> lock(mutex);
        return map.bucket_count();
    }
    
    float load_factor() const override {
        std::lock_guard<std::mutex> lock(mutex);
        return map.load_factor();
    }
    
    void max_load_factor(float ml) override {
        std::lock_guard<std::mutex> lock(mutex);
        map.max_load_factor(ml);
    }
};

int main() {
    std::cout << "Concurrent Hashmap Benchmarks" << std::endl;
    std::cout << "============================" << std::endl;
    
    const size_t NUM_OPERATIONS = 1000000;
    
    // Create benchmarks for different implementations
    auto std_map = std::make_unique<mutex_unordered_map<int, int>>();
    auto concurrent_map = std::make_unique<concurrent::thread_partitioned_hashmap<int, int>>();
    
    benchmark::hashmap_benchmark std_benchmark("std::unordered_map+mutex", std::move(std_map));
    benchmark::hashmap_benchmark concurrent_benchmark("thread_partitioned_hashmap", std::move(concurrent_map));
    
    // Run insert benchmarks with different thread counts
    std::cout << "\nInsert Benchmarks:" << std::endl;
    for (size_t threads : {1, 2, 4, 8, 16}) {
        auto std_result = std_benchmark.run_insert_benchmark(NUM_OPERATIONS, threads);
        std_result.print();
        
        auto concurrent_result = concurrent_benchmark.run_insert_benchmark(NUM_OPERATIONS, threads);
        concurrent_result.print();
        
        std::cout << std::endl;
    }
    
    // Run mixed benchmarks with different read ratios
    std::cout << "\nMixed Operation Benchmarks (8 threads):" << std::endl;
    for (double read_ratio : {0.25, 0.5, 0.75, 0.9}) {
        auto std_result = std_benchmark.run_find_benchmark(read_ratio, NUM_OPERATIONS, 8);
        std_result.print();
        
        auto concurrent_result = concurrent_benchmark.run_find_benchmark(read_ratio, NUM_OPERATIONS, 8);
        concurrent_result.print();
        
        std::cout << std::endl;
    }
    
    return 0;
}