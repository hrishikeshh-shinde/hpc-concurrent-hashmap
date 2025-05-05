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
#include <algorithm>  // For std::min
#include <unistd.h>   // For getcwd
#include <limits.h>   // For PATH_MAX

// Global vector to hold test data for the current phase
std::vector<std::pair<std::string, bool>> current_phase_tests;
std::atomic<int> successful_ops_counter(0); // Counter for successful operations in a phase

// Helper function to get current working directory
std::string get_cwd() {
    char buff[PATH_MAX];
    if (getcwd(buff, sizeof(buff)) != NULL) {
        return std::string(buff);
    } else {
        perror("getcwd() error");
        return "[unknown CWD]";
    }
}

// Worker function for insertion phase
void insert_worker(int thread_idx, int num_threads, ThreadPartitionHashMap &h, size_t start, size_t n) { // Use size_t for indices/counts
    int local_success = 0;
    size_t end_idx = start + n;
    for (size_t i = start; i < end_idx; ++i) {
        const std::string& key = current_phase_tests[i].first;
        [[maybe_unused]] bool should_insert = current_phase_tests[i].second;

        size_t submap_idx = h.get_submap_index(key);
        size_t owner_id = h.get_owner_thread_index(submap_idx);

        if (static_cast<size_t>(thread_idx) == owner_id) {
            bool insert_result = h.insert(key);
            if (insert_result) {
                local_success++;
            }
        }
    }
    successful_ops_counter.fetch_add(local_success);
}

// Worker function for search phase
void search_worker(int thread_idx, int num_threads, const ThreadPartitionHashMap &h, size_t start, size_t n) { // Use size_t
     int local_finds = 0;
     int local_checks = 0;
     size_t end_idx = start + n;
    for (size_t i = start; i < end_idx; ++i) {
        const std::string& key = current_phase_tests[i].first;
        bool should_be_present = current_phase_tests[i].second;

        size_t submap_idx = h.get_submap_index(key);
        size_t owner_id = h.get_owner_thread_index(submap_idx);

        if (static_cast<size_t>(thread_idx) == owner_id) {
            local_checks++;
            bool search_result = h.search(key);
            assert(search_result == should_be_present);
            if(search_result) {
                local_finds++;
            }
        }
    }
    successful_ops_counter.fetch_add(local_checks);
}

// Worker function for removal phase
void remove_worker(int thread_idx, int num_threads, ThreadPartitionHashMap &h, size_t start, size_t n) { // Use size_t
    int local_success = 0;
    size_t end_idx = start + n;
    for (size_t i = start; i < end_idx; ++i) {
        const std::string& key = current_phase_tests[i].first;
        bool should_remove = current_phase_tests[i].second;

        size_t submap_idx = h.get_submap_index(key);
        size_t owner_id = h.get_owner_thread_index(submap_idx);

        if (static_cast<size_t>(thread_idx) == owner_id) {
            bool remove_result = h.remove(key);
            assert(remove_result == should_remove);
             if (remove_result) {
                local_success++;
            }
        }
    }
    successful_ops_counter.fetch_add(local_success);
}


int main(int argc, char *argv[]) {

    // --- Argument Parsing ---
    unsigned int cores = std::thread::hardware_concurrency();
    std::string test_data_base_path = "../testdata/"; // Default path

    if (cores == 0) {
        cores = 4; // Default cores
        std::cerr << "Warning: Could not detect hardware concurrency, defaulting to " << cores << " threads." << std::endl;
    }

    // Argument 1: Number of threads (optional)
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

    // Argument 2: Path to testdata directory (optional)
     if (argc > 2) {
        test_data_base_path = argv[2];
        // Ensure trailing slash
        if (!test_data_base_path.empty() && test_data_base_path.back() != '/') {
            test_data_base_path += '/';
        }
         std::cout << "Using test data path from command line: " << test_data_base_path << std::endl;
    } else {
         std::cout << "Warning: No test data path provided via command line. Using default: " << test_data_base_path << std::endl;
         std::cout << "         (This default assumes executable is run from build/ directory)" << std::endl;
    }
    // --- End Argument Parsing ---


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
    std::string insert_file_path = test_data_base_path + "insert.txt";
    std::ifstream insertFile(insert_file_path);
    if (!insertFile) {
        std::cerr << "Error: Cannot open insert file: " << insert_file_path << std::endl;
        std::cerr << "Current Working Directory: " << get_cwd() << std::endl; // Print CWD on error
        perror("ifstream error");
        return 1;
    }
    current_phase_tests.clear();
    while (insertFile >> s >> expected_result) { current_phase_tests.push_back({s, expected_result}); }
    insertFile.close();

    if (current_phase_tests.empty()) { std::cerr << "Error: No data loaded from " << insert_file_path << std::endl; return 1; }
    const size_t N_insert = current_phase_tests.size(); // Use size_t
    std::cout << "Loaded " << N_insert << " insertion instructions from " << insert_file_path << std::endl;

    successful_ops_counter = 0;
    start = std::chrono::high_resolution_clock::now();
    threads.clear();
    size_t chunk_size = (N_insert + cores - 1) / cores; // Use size_t
    for (unsigned int i = 0; i < cores; ++i) {
        size_t start_idx = i * chunk_size;
        size_t count = (start_idx + chunk_size > N_insert) ? (N_insert - start_idx) : chunk_size;
        if (count > 0 && start_idx < N_insert) {
             threads.emplace_back(insert_worker, i, cores, std::ref(h), start_idx, count);
        }
    }
    for (auto &t : threads) { t.join(); }
    end = std::chrono::high_resolution_clock::now();

    int expected_inserts_from_file = 0;
    for(const auto& p : current_phase_tests) {
        if (p.second) expected_inserts_from_file++;
    }

    time_ms = end - start;
    std::cout << "Insertion time: " << time_ms.count() << " ms." << std::endl;
    std::cout << "  Successful inserts reported by workers: " << successful_ops_counter.load() << std::endl;
    std::cout << "  Expected successful inserts from file: " << expected_inserts_from_file << std::endl;
    std::cout << "  Final map size reported by map.size(): " << h.size() << std::endl;
    assert(h.size() == successful_ops_counter.load());
    assert(h.size() == expected_inserts_from_file);


    // --- Test search ---
    std::cout << "\n--- Timing Search Phase ---" << std::endl;
    std::string search_file_path = test_data_base_path + "search.txt";
    std::ifstream searchFile(search_file_path);
    if (!searchFile) {
        std::cerr << "Error: Cannot open search file: " << search_file_path << std::endl;
        std::cerr << "Current Working Directory: " << get_cwd() << std::endl;
        perror("ifstream error");
        return 1;
    }
    current_phase_tests.clear();
    while (searchFile >> s >> expected_result) { current_phase_tests.push_back({s, expected_result}); }
    searchFile.close();

    if (current_phase_tests.empty()) { std::cerr << "Error: No data loaded from " << search_file_path << std::endl; return 1; }
    const size_t N_search = current_phase_tests.size(); // Use size_t
    std::cout << "Loaded " << N_search << " search instructions from " << search_file_path << std::endl;

    successful_ops_counter = 0;
    start = std::chrono::high_resolution_clock::now();
    threads.clear();
    chunk_size = (N_search + cores - 1) / cores; // Use size_t
    for (unsigned int i = 0; i < cores; ++i) {
        size_t start_idx = i * chunk_size;
        size_t count = (start_idx + chunk_size > N_search) ? (N_search - start_idx) : chunk_size;
         if (count > 0 && start_idx < N_search) {
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
    std::string delete_file_path = test_data_base_path + "delete.txt";
    std::ifstream deletionFile(delete_file_path);
    if (!deletionFile) {
        std::cerr << "Error: Cannot open delete file: " << delete_file_path << std::endl;
        std::cerr << "Current Working Directory: " << get_cwd() << std::endl;
        perror("ifstream error");
        return 1;
    }
    current_phase_tests.clear();
    while (deletionFile >> s >> expected_result) { current_phase_tests.push_back({s, expected_result}); }
    deletionFile.close();

    if (current_phase_tests.empty()) { std::cerr << "Error: No data loaded from " << delete_file_path << std::endl; return 1; }
    const size_t N_delete = current_phase_tests.size(); // Use size_t
    std::cout << "Loaded " << N_delete << " deletion instructions from " << delete_file_path << std::endl;

    int expected_removes_from_file = 0;
    for(const auto& p : current_phase_tests) {
        if (p.second) expected_removes_from_file++;
    }
    int current_size_before_delete = h.size(); // Get size before starting deletion
    int expected_final_size = current_size_before_delete - expected_removes_from_file;

    successful_ops_counter = 0;
    start = std::chrono::high_resolution_clock::now();
    threads.clear();
    chunk_size = (N_delete + cores - 1) / cores; // Use size_t
    for (unsigned int i = 0; i < cores; ++i) {
        size_t start_idx = i * chunk_size;
        size_t count = (start_idx + chunk_size > N_delete) ? (N_delete - start_idx) : chunk_size;
         if (count > 0 && start_idx < N_delete) {
            threads.emplace_back(remove_worker, i, cores, std::ref(h), start_idx, count);
         }
    }
    for (auto &t : threads) { t.join(); }
    end = std::chrono::high_resolution_clock::now();


    time_ms = end - start;
    std::cout << "Deletion time: " << time_ms.count() << " ms." << std::endl;
    std::cout << "  Successful removes reported by workers: " << successful_ops_counter.load() << std::endl;
    std::cout << "  Expected successful removes from file: " << expected_removes_from_file << std::endl;
    std::cout << "  Expected final size (calculated): " << expected_final_size << std::endl;
    std::cout << "  Final map size reported by map.size(): " << h.size() << std::endl;
    // Assert final size based on calculation
    assert(h.size() == expected_final_size);
    // Assert consistency between reported removes and expected removes
    assert(successful_ops_counter.load() == expected_removes_from_file);


    std::cout << "\nAll timing tests completed!" << std::endl;
    return 0;
}
