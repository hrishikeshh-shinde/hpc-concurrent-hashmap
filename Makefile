CHAIN_HASH_MAP_SRC_FILES := src/AbstractHashMap.cpp src/ChainHashMap.cpp
CHAIN_HASH_MAP_TEST_FILE := tests/ChainHashMapTest.cpp

chainhashmaptest: $(CHAIN_HASH_MAP_SRC_FILES) $(CHAIN_HASH_MAP_TEST_FILE)
	g++ -std=c++17 $(CHAIN_HASH_MAP_SRC_FILES) $(CHAIN_HASH_MAP_TEST_FILE) -o chainhashmaptest.out

clean:
	rm *.out