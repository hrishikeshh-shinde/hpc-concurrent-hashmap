# Concurrent HashMap in C++ with OpenMP Parallelism

This project implements a high-performance concurrent hashmap in C++, designed for multi-threaded workloads. It supports dynamic resizing, fine-grained locking, OpenMP parallelism, and includes an experimental lock-free partitioned version.

## Authors

- Avi Valse  
- Chirag Jain  
- Duke Nguyen  
- Hrishikesh Shinde  
- Jason Ranjit Joseph Rajasekar

## Overview

A concurrent hashmap is a key-value data structure that supports safe and efficient multi-threaded inserts, searches, and deletes. While Java has built-in support, C++ lacks a standard concurrent hashmap. This project addresses that gap with multiple versions:

- ThreadSafeChainHashMap (fixed-size, per-bucket locking)
- C++ Thread-based Rehash
- OpenMP-based Rehash
- Lock-Free Partitioned HashMap

## Features

- Thread-safe insert, search, and delete operations
- Sharded map structure using vectors of lists
- Parallelized rehashing using both C++ threads and OpenMP
- Lock-free partitioned hashmap (application-controlled thread ownership)
- Benchmarking framework and testing suite

## Branches

- `rehash` (default): Main implementation with parallelism and rehashing
- `ducndh`: Experimental lock-free partitioned hashmap

## API

All implementations support the following operations:

```cpp
bool insert(const std::string&);
bool search(const std::string&);
bool erase(const std::string&);
```

## Build Instructions

```bash
make all
```

## Run Tests

```bash
./test_insert
./test_search
./test_delete
```

## Benchmarking Methodology

- **Workload**: 1,000,000 unique strings of length 1–100  
- **Threading**: Input divided evenly across `std::thread::hardware_concurrency()` threads  
- **Measurement**: Wall-clock timing for insert, search, and delete phases separately

## Results

| Version                  | Insertion Time (ms) | Search Time (ms) | Deletion Time (ms) |
|--------------------------|---------------------|-------------------|---------------------|
| ThreadSafeChainHashMap   | 41.1538             | 45.0812           | 56.0066             |
| C++ Threaded Rehash      | 181.975             | 65.0195           | 66.7258             |
| OpenMP Rehash            | 49.2445             | 68.4586           | 73.7651             |
| Lock-Free Partitioned    | 126.355             | 59.3406           | 99.5053             |
| std::unordered_set       | 332.908             | 29.8971           | 652.265             |

## Observations

### Insert

- ThreadSafeChainHashMap performs best due to minimal locking and no resizing.
- OpenMP Rehash is close in performance with added flexibility.
- C++ Threaded Rehash suffers from thread management overhead.
- All implementations outperform `std::unordered_set`.

### Search

- `std::unordered_set` is fastest, but not thread-safe.
- ThreadSafeChainHashMap performs well due to non-blocking reads.
- Lock-Free and OpenMP versions trade performance for safety.

### Delete

- ThreadSafeChainHashMap has the best delete times.
- OpenMP is slightly slower but supports resizing.
- Lock-Free suffers from lack of delegation.
- `std::unordered_set` is significantly slower under concurrency.

## Technical Takeaways

- Atomicity is essential for correctness in concurrent structures.
- Fixed-size maps are fast but inflexible.
- OpenMP provides scalable performance with lower complexity.
- Lock-free is not always better — it depends on the workload.
- STL containers are not concurrency-aware.

## Future Work

- Add delegation in the lock-free model
- Support dynamic shard resizing
- Explore NUMA-aware optimizations
- Replace `std::thread` spawning with a thread pool for rehashing
