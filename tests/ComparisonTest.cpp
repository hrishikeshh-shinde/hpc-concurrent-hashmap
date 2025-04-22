#include <cassert>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector> // <<< Added
#include <string> // <<< Added
#include <chrono> // <<< Added
#include <functional> // <<< Added for std::ref

// <<< Changed Include and Class Name >>>
#include "ThreadPartitionHashMap.h" // Include your map's header

/**
 * A multi-threaded test application adapted for ThreadPartitionHashMap.
 */

// Global vector to hold test data read from files
// Note: Global variables can sometimes be tricky with threading if not careful,
// but here it's populated sequentially before threads access disjoint parts.
std::vector<std::pair<std::string, bool>> tests;

// Test function for insertion
// Takes the map implementation type as a template parameter
template <typename HashMapType>
void test_insert_worker(int start, int n, HashMapType &h) {
    for (int i = start; i < start + n; ++i) { // Corrected loop condition
        // Ensure index is within bounds (important if N is not divisible by cores)
        if (i < tests.size()) {
            // tests[i].second indicates if this key should actually be inserted successfully
            // (e.g., it's not a duplicate within this thread's workload if 'true')
            // We assert based on the *expected* outcome from the file.
             bool expected_to_insert = tests[i].second;
             bool inserted = h.insert(tests[i].first); // Pass by value
             assert(inserted == expected_to_insert); // Check if insert result matches expectation
        }
    }
}

// Test function for searching
template <typename HashMapType>
void test_search_worker(int start, int n, const HashMapType &h) { // Pass map by const ref
    for (int i = start; i < start + n; ++i) { // Corrected loop condition
        if (i < tests.size()) {
            // tests[i].second indicates if the key *should* be present after the insert phase
            assert(h.search(tests[i].first) == tests[i].second); // Pass by value
        }
    }
}

// Test function for removal
template <typename HashMapType>
void test_remove_worker(int start, int n, HashMapType &h) {
    for (int i = start; i < start + n; ++i) { // Corrected loop condition
         if (i < tests.size()) {
            // tests[i].second indicates if the key *should* be present and thus removable
            assert(h.remove(tests[i].first) == tests[i].second); // Pass by value
        }
    }
}


int main(int argc, char *argv[]) {

    // <<< Use your class name >>>
    ThreadPartitionHashMap h(0.7f); // Use appropriate load factor

    std::string s;
    bool expected_result; // Renamed from toInsert for clarity
    std::chrono::high_resolution_clock::time_point start, end;
    std::chrono::duration<double, std::milli> time_ms;
    int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 4; // Fallback if detection fails
    std::vector<std::thread> threads;

    std::cout << "Using " << cores << " threads for testing." << std::endl;

    // --- Test insertion ---
    std::cout << "\n--- Testing Insertion ---" << std::endl;
    std::ifstream insertFile("testdata/insert.txt");
    if (!insertFile) {
        std::cerr << "Error: Cannot open testdata/insert.txt" << std::endl;
        return 1;
    }
    tests.clear(); // Clear previous test data
    while (insertFile >> s >> expected_result) {
        tests.push_back({s, expected_result});
    }
    insertFile.close();

    if (tests.empty()) {
         std::cerr << "Error: No data loaded from testdata/insert.txt" << std::endl;
         return 1;
    }
    const int N_insert = tests.size();
    std::cout << "Loaded " << N_insert << " insertion instructions." << std::endl;

    start = std::chrono::high_resolution_clock::now();
    int p = 0;
    int chunk_size = N_insert / cores;
    threads.clear(); // Clear threads vector
    for (int i = 0; i < cores - 1; ++i) {
        threads.push_back(std::thread(test_insert_worker<ThreadPartitionHashMap>, p, chunk_size, std::ref(h)));
        p += chunk_size;
    }
    // Last thread takes the remaining elements
    threads.push_back(std::thread(test_insert_worker<ThreadPartitionHashMap>, p, N_insert - p, std::ref(h)));

    for (auto &t : threads) {
        t.join();
    }
    // The assertion h.size() == N / 2 assumes exactly half the lines in insert.txt had 'true'
    // Let's count expected size instead for robustness
    size_t expected_insert_count = 0;
    for(const auto& pair : tests) {
        if (pair.second) expected_insert_count++;
    }
    std::cout << "Expected size after inserts: " << expected_insert_count << std::endl;
    std::cout << "Actual size after inserts: " << h.size() << std::endl;
    assert(static_cast<size_t>(h.size()) == expected_insert_count); // Use size_t for comparison
    end = std::chrono::high_resolution_clock::now();

    time_ms = end - start; // Simpler duration calculation
    std::cout << "Insertion time: " << time_ms.count() << " ms." << std::endl;


    // --- Test search ---
    std::cout << "\n--- Testing Search ---" << std::endl;
    std::ifstream searchFile("testdata/search.txt");
     if (!searchFile) {
        std::cerr << "Error: Cannot open testdata/search.txt" << std::endl;
        return 1;
    }
    tests.clear();
    threads.clear();
    while (searchFile >> s >> expected_result) {
        tests.push_back({s, expected_result});
    }
    searchFile.close();

    if (tests.empty()) {
         std::cerr << "Error: No data loaded from testdata/search.txt" << std::endl;
         return 1;
    }
    const int N_search = tests.size();
    std::cout << "Loaded " << N_search << " search instructions." << std::endl;

    start = std::chrono::high_resolution_clock::now();
    p = 0;
    chunk_size = N_search / cores;
    for (int i = 0; i < cores - 1; ++i) {
        // Pass h by const reference for search
        threads.push_back(std::thread(test_search_worker<const ThreadPartitionHashMap>, p, chunk_size, std::cref(h)));
        p += chunk_size;
    }
    threads.push_back(std::thread(test_search_worker<const ThreadPartitionHashMap>, p, N_search - p, std::cref(h)));

    for (auto &t : threads) {
        t.join();
    }
    end = std::chrono::high_resolution_clock::now();

    time_ms = end - start;
    std::cout << "Search time: " << time_ms.count() << " ms." << std::endl;


    // --- Test deletion ---
    std::cout << "\n--- Testing Deletion ---" << std::endl;
    std::ifstream deletionFile("testdata/delete.txt");
     if (!deletionFile) {
        std::cerr << "Error: Cannot open testdata/delete.txt" << std::endl;
        return 1;
    }
    tests.clear();
    threads.clear();
    while (deletionFile >> s >> expected_result) {
        tests.push_back({s, expected_result});
    }
    deletionFile.close();

     if (tests.empty()) {
         std::cerr << "Error: No data loaded from testdata/delete.txt" << std::endl;
         return 1;
    }
    const int N_delete = tests.size();
    std::cout << "Loaded " << N_delete << " deletion instructions." << std::endl;

    start = std::chrono::high_resolution_clock::now();
    p = 0;
    chunk_size = N_delete / cores;
    for (int i = 0; i < cores - 1; ++i) {
        threads.push_back(std::thread(test_remove_worker<ThreadPartitionHashMap>, p, chunk_size, std::ref(h)));
        p += chunk_size;
    }
    threads.push_back(std::thread(test_remove_worker<ThreadPartitionHashMap>, p, N_delete - p, std::ref(h)));

    for (auto &t : threads) {
        t.join();
    }
    // Assert final size is 0 (assuming delete.txt removes everything inserted)
    std::cout << "Final size after deletes: " << h.size() << std::endl;
    assert(h.size() == 0);
    end = std::chrono::high_resolution_clock::now();

    time_ms = end - start;
    std::cout << "Deletion time: " << time_ms.count() << " ms." << std::endl;

    std::cout << "\nTeam test finished successfully!" << std::endl;
    return 0;
}
