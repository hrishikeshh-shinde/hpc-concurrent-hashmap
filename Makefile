CHAIN_HASH_MAP_SRC_FILES := src/AbstractHashMap.cpp src/ChainHashMap.cpp
CHAIN_HASH_MAP_TEST_FILE := tests/ChainHashMapTest.cpp

THREAD_SAFE_CHAIN_HASH_MAP_SRC_FILES := src/AbstractHashMap.cpp src/ThreadSafeChainHashMap.cpp
THREAD_SAFE_CHAIN_HASH_MAP_TEST_FILE := tests/ThreadSafeChainHashMapTest.cpp

all: chainhashmaptest threadsafechainhashmaptest unorderedsettest threadsafeunorderedsettest

chainhashmaptest: $(CHAIN_HASH_MAP_SRC_FILES) $(CHAIN_HASH_MAP_TEST_FILE)
	g++ -std=c++17 $(CHAIN_HASH_MAP_SRC_FILES) $(CHAIN_HASH_MAP_TEST_FILE) -g -o chainhashmaptest.out

threadsafechainhashmaptest: $(THREAD_SAFE_CHAIN_HASH_MAP_SRC_FILES) $(THREAD_SAFE_CHAIN_HASH_MAP_TEST_FILE)
	g++ -std=c++17 $(THREAD_SAFE_CHAIN_HASH_MAP_SRC_FILES) $(THREAD_SAFE_CHAIN_HASH_MAP_TEST_FILE) -g -o threadsafechainhashmaptest.out

unorderedsettest: tests/UnorderedSetTest.cpp
	g++ -std=c++17 tests/UnorderedSetTest.cpp -g -o unorderedsettest.out

threadsafeunorderedsettest: tests/ThreadSafeUnorderedSetTest.cpp
	g++ -std=c++17 tests/ThreadSafeUnorderedSetTest.cpp -g -o threadsafeunorderedsettest.out

clean:
	rm *.out