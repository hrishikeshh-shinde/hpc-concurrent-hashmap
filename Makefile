CHAIN_HASH_MAP_SRC_FILES := src/AbstractHashMap.cpp src/ChainHashMap.cpp
CHAIN_HASH_MAP_TEST_FILE := tests/ChainHashMapTest.cpp

THREAD_SAFE_CHAIN_HASH_MAP_SRC_FILES := src/AbstractHashMap.cpp src/ThreadSafeChainHashMap.cpp
THREAD_SAFE_CHAIN_HASH_MAP_TEST_FILE := tests/ThreadSafeChainHashMapTest.cpp

CHAIN_HASH_MAP_REHASH_OPEN_MP_SRC_FILES := src/AbstractHashMap.cpp src/ChainHashMapRehashOpenMp.cpp
CHAIN_HASH_MAP_REHASH_OPEN_MP_TEST_FILE := tests/ChainHashMapRehashOpenMpTest.cpp

CHAIN_HASH_MAP_REHASH_THREADS_SRC_FILES := src/AbstractHashMap.cpp src/ChainHashMapRehashThreads.cpp
CHAIN_HASH_MAP_REHASH_THREADS_TEST_FILE := tests/ChainHashMapRehashThreadsTest.cpp

all: chainhashmaptest threadsafechainhashmaptest unorderedsettest threadsafeunorderedsettest chainhashmaprehashopenmptest chainhashmaprehashthreadstest

chainhashmaptest: $(CHAIN_HASH_MAP_SRC_FILES) $(CHAIN_HASH_MAP_TEST_FILE)
	g++ -std=c++17 $(CHAIN_HASH_MAP_SRC_FILES) $(CHAIN_HASH_MAP_TEST_FILE) -g -o chainhashmaptest.out

threadsafechainhashmaptest: $(THREAD_SAFE_CHAIN_HASH_MAP_SRC_FILES) $(THREAD_SAFE_CHAIN_HASH_MAP_TEST_FILE)
	g++ -std=c++17 $(THREAD_SAFE_CHAIN_HASH_MAP_SRC_FILES) $(THREAD_SAFE_CHAIN_HASH_MAP_TEST_FILE) -g -o threadsafechainhashmaptest.out

unorderedsettest: tests/UnorderedSetTest.cpp
	g++ -std=c++17 tests/UnorderedSetTest.cpp -g -o unorderedsettest.out

threadsafeunorderedsettest: tests/ThreadSafeUnorderedSetTest.cpp
	g++ -std=c++17 tests/ThreadSafeUnorderedSetTest.cpp -g -o threadsafeunorderedsettest.out

chainhashmaprehashopenmptest: $(CHAIN_HASH_MAP_REHASH_OPEN_MP_SRC_FILES) $(CHAIN_HASH_MAP_REHASH_OPEN_MP_TEST_FILE)
	/opt/homebrew/Cellar/gcc/14.2.0_1/bin/g++-14 -std=c++17 -pthread $(CHAIN_HASH_MAP_REHASH_OPEN_MP_SRC_FILES) $(CHAIN_HASH_MAP_REHASH_OPEN_MP_TEST_FILE) -fopenmp -O3 -o chainhashmaprehashopenmptest.out

chainhashmaprehashthreadstest: $(CHAIN_HASH_MAP_REHASH_THREADS_SRC_FILES) $(CHAIN_HASH_MAP_REHASH_THREADS_TEST_FILE)
	g++ -std=c++17 -pthread $(CHAIN_HASH_MAP_REHASH_THREADS_SRC_FILES) $(CHAIN_HASH_MAP_REHASH_THREADS_TEST_FILE) -O3 -o chainhashmaprehashthreadstest.out


clean:
	rm *.out