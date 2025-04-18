#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "ThreadPartitionHashMap.h"

// Basic sequential operations test
void test_basic_operations() {
    ThreadPartitionHashMap map(0.7);
    
    // Test insert
    assert(map.insert("key1") == true);
    assert(map.insert("key2") == true);
    assert(map.insert("key3") == true);
    
    // Test duplicate insert
    assert(map.insert("key1") == false);
    
    // Test size
    assert(map.size() == 3);
    
    // Test search
    assert(map.search("key1") == true);
    assert(map.search("key2") == true);
    assert(map.search("key3") == true);
    assert(map.search("key4") == false);
    
    // Test remove
    assert(map.remove("key2") == true);
    assert(map.size() == 2);
    assert(map.search("key2") == false);
    
    // Test remove non-existent key
    assert(map.remove("key4") == false);
    
    std::cout << "Basic operations test passed!" << std::endl;
}

// Test rehashing functionality
void test_rehashing() {
    ThreadPartitionHashMap map(0.5); 
    
    for (int i = 0; i < 1000; i++) {
        map.insert("key-" + std::to_string(i));
    }
    
    for (int i = 0; i < 1000; i++) {
        assert(map.search("key-" + std::to_string(i)) == true);
    }
    
    std::cout << "Rehashing test passed!" << std::endl;
}

int main() {
    std::cout << "Starting correctness tests..." << std::endl;
    
    test_basic_operations();
    test_rehashing();
    
    std::cout << "All correctness tests passed!" << std::endl;
    return 0;
}