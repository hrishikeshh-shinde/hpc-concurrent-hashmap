CHAIN_HASH_MAP_SRC_FILES := src/AbstractHashMap.cpp src/ChainHashMap.cpp
CHAIN_HASH_MAP_TEST_FILE := tests/ChainHashMapTest.cpp

THREAD_SAFE_CHAIN_HASH_MAP_SRC_FILES := src/AbstractHashMap.cpp src/ThreadSafeChainHashMap.cpp
THREAD_SAFE_CHAIN_HASH_MAP_TEST_FILE := tests/ThreadSafeChainHashMapTest.cpp

all: chainhashmaptest threadsafechainhashmaptest unorderedsettest threadsafeunorderedsettest

chainhashmaptest: $(CHAIN_HASH_MAP_SRC_FILES) $(CHAIN_HASH_MAP_TEST_FILE)
	g++ $(CHAIN_HASH_MAP_SRC_FILES) $(CHAIN_HASH_MAP_TEST_FILE) -Ofast -o chainhashmaptest.out

threadsafechainhashmaptest: $(THREAD_SAFE_CHAIN_HASH_MAP_SRC_FILES) $(THREAD_SAFE_CHAIN_HASH_MAP_TEST_FILE)
	g++ $(THREAD_SAFE_CHAIN_HASH_MAP_SRC_FILES) $(THREAD_SAFE_CHAIN_HASH_MAP_TEST_FILE)  -Ofast -o threadsafechainhashmaptest.out

unorderedsettest: tests/UnorderedSetTest.cpp
	g++ tests/UnorderedSetTest.cpp  -Ofast -o unorderedsettest.out

threadsafeunorderedsettest: tests/ThreadSafeUnorderedSetTest.cpp
	g++ tests/ThreadSafeUnorderedSetTest.cpp  -Ofast -o threadsafeunorderedsettest.out

clean:
	rm *.out