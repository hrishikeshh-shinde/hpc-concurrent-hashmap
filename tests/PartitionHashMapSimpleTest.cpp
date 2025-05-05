// tests/PartitionHashMapSimpleTest.cpp

#include "../src/ThreadPartitionHashMap.h" // Adjust path as needed
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <numeric> // For std::accumulate
#include <unistd.h> // For getcwd
#include <limits.h> // For PATH_MAX (optional, POSIX)

// Helper function to get current working directory
std::string get_cwd() {
    char buff[PATH_MAX]; // PATH_MAX might not be defined everywhere, consider a large fixed size like 1024
    if (getcwd(buff, sizeof(buff)) != NULL) {
        return std::string(buff);
    } else {
        perror("getcwd() error");
        return "[unknown CWD]";
    }
}


int main(int argc, char *argv[]) {
    // Instantiate with num_worker_threads = 1 for single-threaded test
    ThreadPartitionHashMap h(1, 0.75f); // Use 1 worker thread, typical load factor

    std::vector<std::pair<std::string, bool>> tests;
    std::string s;
    bool expected_result;
    std::chrono::high_resolution_clock::time_point start, end;
    std::chrono::duration<double, std::milli> time_ms;

    // --- Get Test Data Path ---
    std::string test_data_base_path = "../testdata/"; // Default path (relative to build dir)
    if (argc > 1) {
        test_data_base_path = argv[1];
        // Ensure trailing slash if needed
        if (!test_data_base_path.empty() && test_data_base_path.back() != '/') {
            test_data_base_path += '/';
        }
        std::cout << "Using test data path from command line: " << test_data_base_path << std::endl;
    } else {
         std::cout << "Warning: No test data path provided via command line. Using default: " << test_data_base_path << std::endl;
         std::cout << "         (This default assumes executable is run from build/ directory)" << std::endl;
    }
    // --- End Get Test Data Path ---


    std::cout << "--- Testing ThreadPartitionHashMap (Single-Threaded Correctness) ---" << std::endl;

    // --- Test insertion ---
    std::cout << "\n--- Testing Insertion ---" << std::endl;
    std::string insert_file_path = test_data_base_path + "insert.txt";
    std::ifstream insertFile(insert_file_path);
    if (!insertFile) {
        std::cerr << "Error: Cannot open insert file: " << insert_file_path << std::endl;
        std::cerr << "Current Working Directory: " << get_cwd() << std::endl; // Print CWD on error
        perror("ifstream error"); // Print system error message
        return 1;
    }
    tests.clear();
    while (insertFile >> s >> expected_result) {
        tests.push_back({s, expected_result});
    }
    insertFile.close();

    if (tests.empty()) {
         std::cerr << "Error: No data loaded from " << insert_file_path << std::endl;
         return 1;
    }
    const int N_insert_ops = tests.size();
    std::cout << "Loaded " << N_insert_ops << " insertion instructions from " << insert_file_path << std::endl;

    start = std::chrono::high_resolution_clock::now();
    int actual_inserts = 0;
    int expected_inserts = 0;
    for (size_t i = 0; i < tests.size(); ++i) { // Use size_t for loop
        bool should_insert = tests[i].second;
        bool insert_result = h.insert(tests[i].first);
        assert(insert_result == should_insert);
        if (insert_result) {
            actual_inserts++;
        }
        if (should_insert) {
             expected_inserts++;
        }
    }
    std::cout << "Expected successful inserts based on file: " << expected_inserts << std::endl;
    std::cout << "Actual successful inserts reported by map: " << actual_inserts << std::endl;
    std::cout << "Final map size reported by map.size(): " << h.size() << std::endl;
    assert(h.size() == expected_inserts);
    assert(h.size() == actual_inserts);

    end = std::chrono::high_resolution_clock::now();
    time_ms = end - start;
    std::cout << "Insertion time: " << time_ms.count() << " ms." << std::endl;
    std::cout << "Insertion test PASSED." << std::endl;


    // --- Test search ---
    std::cout << "\n--- Testing Search ---" << std::endl;
    tests.clear();
    std::string search_file_path = test_data_base_path + "search.txt";
    std::ifstream searchFile(search_file_path);
     if (!searchFile) {
        std::cerr << "Error: Cannot open search file: " << search_file_path << std::endl;
        std::cerr << "Current Working Directory: " << get_cwd() << std::endl;
        perror("ifstream error");
        return 1;
    }
    while (searchFile >> s >> expected_result) {
        tests.push_back({s, expected_result});
    }
    searchFile.close();

    if (tests.empty()) {
         std::cerr << "Error: No data loaded from " << search_file_path << std::endl;
         return 1;
    }
    const int N_search_ops = tests.size();
    std::cout << "Loaded " << N_search_ops << " search instructions from " << search_file_path << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < tests.size(); ++i) { // Use size_t for loop
        assert(h.search(tests[i].first) == tests[i].second);
    }
    end = std::chrono::high_resolution_clock::now();
    time_ms = end - start;
    std::cout << "Search time: " << time_ms.count() << " ms." << std::endl;
    std::cout << "Search test PASSED." << std::endl;


    // --- Test deletion ---
    std::cout << "\n--- Testing Deletion ---" << std::endl;
    tests.clear();
    std::string delete_file_path = test_data_base_path + "delete.txt";
    std::ifstream deletionFile(delete_file_path);
     if (!deletionFile) {
        std::cerr << "Error: Cannot open delete file: " << delete_file_path << std::endl;
        std::cerr << "Current Working Directory: " << get_cwd() << std::endl;
        perror("ifstream error");
        return 1;
    }
    while (deletionFile >> s >> expected_result) {
        tests.push_back({s, expected_result});
    }
    deletionFile.close();

     if (tests.empty()) {
         std::cerr << "Error: No data loaded from " << delete_file_path << std::endl;
         return 1;
    }
    const int N_delete_ops = tests.size();
    std::cout << "Loaded " << N_delete_ops << " deletion instructions from " << delete_file_path << std::endl;

    start = std::chrono::high_resolution_clock::now();
    int expected_removes = 0;
    int actual_removes = 0;
    for (size_t i = 0; i < tests.size(); ++i) { // Use size_t for loop
        bool should_remove = tests[i].second;
        bool remove_result = h.remove(tests[i].first);
        assert(remove_result == should_remove);
         if (remove_result) {
            actual_removes++;
        }
        if (should_remove) {
             expected_removes++;
        }
    }
    int expected_final_size = expected_inserts - expected_removes;
    std::cout << "Expected successful removes based on file: " << expected_removes << std::endl;
    std::cout << "Actual successful removes reported by map: " << actual_removes << std::endl;
    std::cout << "Expected final size: " << expected_final_size << std::endl;
    std::cout << "Final map size reported by map.size(): " << h.size() << std::endl;
    assert(h.size() == expected_final_size);
    // If delete.txt is designed to empty the map, assert size is 0
    // assert(h.size() == 0);

    end = std::chrono::high_resolution_clock::now();
    time_ms = end - start;
    std::cout << "Deletion time: " << time_ms.count() << " ms." << std::endl;
    std::cout << "Deletion test PASSED." << std::endl;

    std::cout << "\nAll single-threaded correctness tests PASSED!" << std::endl;
    return 0;
}
