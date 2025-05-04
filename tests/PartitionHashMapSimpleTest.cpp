// tests/PartitionHashMapSimpleTest.cpp

#include "../src/ThreadPartitionHashMap.h" // Adjust path as needed
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// Basic sequential operations test for ThreadPartitionHashMap
void test_basic_operations() {
    std::cout << "--- Running Basic Operations Test ---" << std::endl;
    // Instantiate with num_worker_threads = 1 for sequential testing
    ThreadPartitionHashMap map(1, 0.7f);

    // Test insert
    std::cout << "Testing insert..." << std::endl;
    assert(map.insert("key1") == true);
    assert(map.insert("key2") == true);
    assert(map.insert("key3") == true);
    assert(map.size() == 3);
    std::cout << "  Initial inserts OK. Size: " << map.size() << std::endl;

    // Test duplicate insert
    assert(map.insert("key1") == false);
    assert(map.size() == 3); // Size should not change
    std::cout << "  Duplicate insert blocked OK. Size: " << map.size() << std::endl;

    // Test search
    std::cout << "Testing search..." << std::endl;
    assert(map.search("key1") == true);
    assert(map.search("key2") == true);
    assert(map.search("key3") == true);
    assert(map.search("key_nonexistent") == false);
    std::cout << "  Search OK." << std::endl;

    // Test remove
    std::cout << "Testing remove..." << std::endl;
    assert(map.remove("key2") == true);
    assert(map.size() == 2);
    assert(map.search("key2") == false); // Verify removal
    std::cout << "  Remove existing OK. Size: " << map.size() << std::endl;

    // Test remove non-existent key
    assert(map.remove("key4") == false);
    assert(map.size() == 2); // Size should not change
    std::cout << "  Remove non-existent blocked OK. Size: " << map.size() << std::endl;

    // Test remove remaining keys
    assert(map.remove("key1") == true);
    assert(map.remove("key3") == true);
    assert(map.size() == 0);
    assert(map.search("key1") == false);
    assert(map.search("key3") == false);
    std::cout << "  Remove remaining OK. Final Size: " << map.size() << std::endl;

    std::cout << "Basic operations test passed!" << std::endl;
}

// Test basic rehashing functionality sequentially
void test_sequential_rehashing() {
    std::cout << "\n--- Running Sequential Rehashing Test ---" << std::endl;
    // Use a low load factor to trigger resizing sooner
    ThreadPartitionHashMap map(1, 0.5f);
    int num_keys_to_insert = 100; // Insert enough keys to likely trigger resizes

    std::cout << "Inserting " << num_keys_to_insert << " keys to trigger potential resizing..." << std::endl;
    for (int i = 0; i < num_keys_to_insert; ++i) {
        std::string key = "rehash_key_" + std::to_string(i);
        assert(map.insert(key) == true);
    }
    assert(map.size() == num_keys_to_insert);
    std::cout << "  Inserts completed. Size: " << map.size() << std::endl;

    std::cout << "Verifying all keys are present after resizing..." << std::endl;
    for (int i = 0; i < num_keys_to_insert; ++i) {
        std::string key = "rehash_key_" + std::to_string(i);
        assert(map.search(key) == true);
    }
    std::cout << "  Verification OK." << std::endl;

    std::cout << "Removing all keys..." << std::endl;
     for (int i = 0; i < num_keys_to_insert; ++i) {
        std::string key = "rehash_key_" + std::to_string(i);
        assert(map.remove(key) == true);
    }
    assert(map.size() == 0);
    std::cout << "  Removals completed. Final Size: " << map.size() << std::endl;


    std::cout << "Sequential rehashing test passed!" << std::endl;
}

int main() {
    std::cout << "Starting ThreadPartitionHashMap Simple Correctness Tests..." << std::endl;

    test_basic_operations();
    test_sequential_rehashing();

    std::cout << "\nAll simple correctness tests passed!" << std::endl;
    return 0;
}
