// tests/PartitionHashMapTimingTest.cpp

#include "../src/ThreadPartitionHashMap.h" // Adjust path as needed
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <functional> // For std::ref
#include <numeric>    // For std::accumulate
#include <atomic>

// Global vector to hold test data for the current phase
// NOTE: Accessing globals from multiple threads needs care, but here
// it's populated sequentially before threads read disjoint parts.
std::vector<std::pair<std::string, bool>> current_phase_tests;
std::atomic<int> successful_ops_counter(0); // Counter for successful operations in a phase

// Worker function for insertion phase
void insert_worker(int thread_idx, int num_threads, ThreadPartitionHashMap &h, int start, int n) {
    int local_success = 0;
    for (int i = start; i < start + n; ++i) {
        if (i >= current_phase_tests.size()) break; // Boundary check

        const std::string& key = current_phase_tests[i].first;
        bool should_insert = current_phase_tests[i].second; // Expected outcome from file

        // --- Application Ownership Check ---
        size_t submap_idx = h.get_submap_index(key);
        size_t owner_id = h.get_owner_thread_index(submap_idx);

        if (static_cast<size_t>(thread_idx) == owner_id) {
            bool insert_result = h.insert(key);
            // Optional: Assert based on file expectation (can be noisy in timing tests)
            // assert(insert_result == should_insert);
            if (insert_result) {
                local_success++;
            }
        }
    }
    successful_ops_counter.fetch_add(local_success);
}

// Worker function for search phase
void search_worker(int thread_idx, int num_threads, const ThreadPartitionHashMap &h, int start, int n) {
     int local_finds = 0; // Count how many searches return true
     int local_checks = 0; // Count how many searches were performed by this thread
    for (int i = start; i < start + n; ++i) {
         if (i >= current_phase_tests.size()) break;

        const std::string& key = current_phase_tests[i].first;
        bool should_be_present = current_phase_tests[i].second; // Expected presence from file

        // --- Application Ownership Check ---
        size_t submap_idx = h.get_submap_index(key);
        size_t owner_id = h.get_owner_thread_index(submap_idx);

        if (static_cast<size_t>(thread_idx) == owner_id) {
            local_checks++;
            bool search_result = h.search(key);
            // Assert that the search result matches the expectation
            assert(search_result == should_be_present);
            if(search_result) {
                local_finds++;
            }
        }
    }
    // successful_ops_counter could track local_checks or local_finds if needed
    successful_ops_counter.fetch_add(local_checks); // Count checks performed
}

// Worker function for removal phase
void remove_worker(int thread_idx, int num_threads, ThreadPartitionHashMap &h, int start, int n) {
    int local_success = 0;
    for (int i = start; i < start + n; ++i) {
         if (i >= current_phase_tests.size()) break;

        const std::string& key = current_phase_tests[i].first;
        bool should_remove = current_phase_tests[i].second; // Expected outcome from file

        // --- Application Ownership Check ---
        size_t submap_idx = h.get_submap_index(key);
        size_t owner_id = h.get_owner_thread_index(submap_idx);

        if (static_cast<size_t>(thread_idx) == owner_id) {
            bool remove_result = h.remove(key);
            // Assert based on file expectation
            assert(remove_result == should_remove);
             if (remove_result) {
                local_success++;
            }
        }
    }
    successful_ops_counter.fetch_add(local_success);
}


int main(int argc, char *argv[]) {

    // Determine number of threads to use
    unsigned int cores = std::thread::hardware_concurrency();
    if (cores == 0) {
        cores = 4; // Default to 4 if detection fails
        std::cerr << "Warning: Could not detect hardware concurrency, defaulting to " << cores << " threads." << std::endl;
    }
     // Allow overriding from command line
    if (argc > 1) {
        try {
            unsigned int requested_cores = std::stoul(argv[1]);
            if (requested_cores > 0) {
                cores = requested_cores;
            } else {
                 std::cerr << "Warning: Invalid number of threads requested (" << argv[1] << "), using " << cores << "." << std::endl;
            }
        } catch (const std::exception& e) {
             std::cerr << "Warning: Could not parse thread count argument '" << argv[1] << "', using " << cores << ". Error: " << e.what() << std::endl;
        }
    }

    std::cout << "--- Testing ThreadPartitionHashMap Timing (" << cores << " threads) ---" << std::endl;
    std::cout << "Model: Application checks ownership." << std::endl;

    // Instantiate map with the number of threads we will use
    ThreadPartitionHashMap h(cores, 0.75f);

    std::string s;
    bool expected_result;
    std::chrono::high_resolution_clock::time_point start, end;
    std::chrono::duration<double, std::milli> time_ms;
    std::vector<std::thread> threads;

    // --- Test insertion ---
    std::cout << "\n--- Timing Insertion Phase ---" << std::endl;
    // Adjust path relative to executable location
    std::ifstream insertFile("../testdata/insert.txt");
    if (!insertFile) { std::cerr << "Error: Cannot open testdata/insert.txt" << std::endl; return 1; }
    current_phase_tests.clear();
    while (insertFile >> s >> expected_result) { current_phase_tests.push_back({s, expected_result}); }
    insertFile.close();

    if (current_phase_tests.empty()) { std::cerr << "Error: No data loaded from testdata/insert.txt" << std::endl; return 1; }
    const int N_insert = current_phase_tests.size();
    std::cout << "Loaded " << N_insert << " insertion instructions." << std::endl;

    successful_ops_counter = 0; // Reset counter
    start = std::chrono::high_resolution_clock::now();
    threads.clear();
    int chunk_size = (N_insert + cores - 1) / cores; // Ceiling division
    for (unsigned int i = 0; i < cores; ++i) {
        int start_idx = i * chunk_size;
        int count = std::min(chunk_size, N_insert - start_idx); // Handle last chunk potentially being smaller
        if (count > 0) {
             threads.emplace_back(insert_worker, i, cores, std::ref(h), start_idx, count);
        }
    }
    for (auto &t : threads) { t.join(); }
    end = std::chrono::high_resolution_clock::now();

    // Calculate expected size based on the input file
    int expected_inserts_from_file = 0;
    for(const auto& p : current_phase_tests) {
        if (p.second) expected_inserts_from_file++;
    }

    time_ms = end - start;
    std::cout << "Insertion time: " << time_ms.count() << " ms." << std::endl;
    std::cout << "  Successful inserts reported by workers: " << successful_ops_counter.load() << std::endl;
    std::cout << "  Expected successful inserts from file: " << expected_inserts_from_file << std::endl;
    std::cout << "  Final map size reported by map.size(): " << h.size() << std::endl;
    // Assert consistency
    assert(h.size() == successful_ops_counter.load());
    assert(h.size() == expected_inserts_from_file);


    // --- Test search ---
    std::cout << "\n--- Timing Search Phase ---" << std::endl;
    // Adjust path relative to executable location
    std::ifstream searchFile("../testdata/search.txt");
    if (!searchFile) { std::cerr << "Error: Cannot open testdata/search.txt" << std::endl; return 1; }
    current_phase_tests.clear();
    while (searchFile >> s >> expected_result) { current_phase_tests.push_back({s, expected_result}); }
    searchFile.close();

    if (current_phase_tests.empty()) { std::cerr << "Error: No data loaded from testdata/search.txt" << std::endl; return 1; }
    const int N_search = current_phase_tests.size();
    std::cout << "Loaded " << N_search << " search instructions." << std::endl;

    successful_ops_counter = 0; // Reset counter (now counts search attempts)
    start = std::chrono::high_resolution_clock::now();
    threads.clear();
    chunk_size = (N_search + cores - 1) / cores;
    for (unsigned int i = 0; i < cores; ++i) {
        int start_idx = i * chunk_size;
         int count = std::min(chunk_size, N_search - start_idx);
         if (count > 0) {
            // Pass h by const reference for search
            threads.emplace_back(search_worker, i, cores, std::cref(h), start_idx, count);
         }
    }
    for (auto &t : threads) { t.join(); }
    end = std::chrono::high_resolution_clock::now();

    time_ms = end - start;
    std::cout << "Search time: " << time_ms.count() << " ms." << std::endl;
    std::cout << "  Total search checks performed by workers: " << successful_ops_counter.load() << std::endl;


    // --- Test deletion ---
    std::cout << "\n--- Timing Deletion Phase ---" << std::endl;
     // Adjust path relative to executable location
    std::ifstream deletionFile("../testdata/delete.txt");
    if (!deletionFile) { std::cerr << "Error: Cannot open testdata/delete.txt" << std::endl; return 1; }
    current_phase_tests.clear();
    while (deletionFile >> s >> expected_result) { current_phase_tests.push_back({s, expected_result}); }
    deletionFile.close();

    if (current_phase_tests.empty()) { std::cerr << "Error: No data loaded from testdata/delete.txt" << std::endl; return 1; }
    const int N_delete = current_phase_tests.size();
    std::cout << "Loaded " << N_delete << " deletion instructions." << std::endl;

    // Calculate expected removes based on file
    int expected_removes_from_file = 0;
    for(const auto& p : current_phase_tests) {
        if (p.second) expected_removes_from_file++;
    }
    int expected_final_size = h.size() - expected_removes_from_file; // Size before delete - expected removes

    successful_ops_counter = 0; // Reset counter
    start = std::chrono::high_resolution_clock::now();
    threads.clear();
    chunk_size = (N_delete + cores - 1) / cores;
    for (unsigned int i = 0; i < cores; ++i) {
        int start_idx = i * chunk_size;
        int count = std::min(chunk_size, N_delete - start_idx);
         if (count > 0) {
            threads.emplace_back(remove_worker, i, cores, std::ref(h), start_idx, count);
         }
    }
    for (auto &t : threads) { t.join(); }
    end = std::chrono::high_resolution_clock::now();


    time_ms = end - start;
    std::cout << "Deletion time: " << time_ms.count() << " ms." << std::endl;
    std::cout << "  Successful removes reported by workers: " << successful_ops_counter.load() << std::endl;
    std::cout << "  Expected successful removes from file: " << expected_removes_from_file << std::endl;
    std::cout << "  Expected final size: " << expected_final_size << std::endl;
    std::cout << "  Final map size reported by map.size(): " << h.size() << std::endl;
    // Assert final size
    assert(h.size() == successful_ops_counter.load()); // Size should equal actual removes if starting from expected_inserts
    assert(h.size() == expected_final_size);
    // Often, delete.txt is designed to empty the map, so assert size is 0 if that's the case.
    // assert(h.size() == 0); // Uncomment if delete.txt should empty the map


    std::cout << "\nAll timing tests completed!" << std::endl;
    return 0;
}
