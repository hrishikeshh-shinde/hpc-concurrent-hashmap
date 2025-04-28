#include "ChainHashMapRehashThreads.h"
#include <algorithm>
#include <iterator>
#include <iostream>
#include <mutex>
#include <thread>

ChainHashMapRehashThreads::ChainHashMapRehashThreads(float loadFactor, int BUCKETS, int MAX_CAPACITY) : AbstractHashMap() {
  if (loadFactor < 0 or loadFactor > 1) {
    std::__throw_out_of_range("load factor value is out of range.");
  }
  if (BUCKETS < 1) {
    std::__throw_out_of_range("BUCKETS value is out of range.");
  }
  if (MAX_CAPACITY < 1) {
    std::__throw_out_of_range("MAX_CAPACITY value is out of range.");
  }
  this->loadFactor = loadFactor;
  this->count = 0;
  this->BUCKETS = BUCKETS;
  this->MAX_CAPACITY = MAX_CAPACITY;
  hashMap = std::vector<std::vector<std::string>>(getBuckets());
  mutexArr = std::vector<std::mutex>(BUCKETS);
}


bool ChainHashMapRehashThreads::insert(std::string key) {
  // If current loadFactor greater than desired, call rehash
  // std::lock_guard<std::mutex> g(globalMutex); // Lock global lock

  // Now lock to check size and trigger rehash safely
  if (size() + 1 > getLoadFactor() * getMaxCapacity()) {
    std::lock_guard<std::mutex> lock(rehashMutex);
    if (size() + 1 > getLoadFactor() * getMaxCapacity()) { 
      rehash();
    }
  }

  const int index = getIndex(hash(key));
  std::lock_guard<std::mutex> lk(mutexArr[index]);
  hashMap[index].push_back(key);
  ++count;
  return true;
}

bool ChainHashMapRehashThreads::search(std::string key) const {
  // std::lock_guard<std::mutex> g(globalMutex); // Lock global lock
  const int index = getIndex(hash(key));
  return std::find(hashMap[index].begin(), hashMap[index].end(), key) != hashMap[index].end();
}


bool ChainHashMapRehashThreads::remove(std::string key) {
  // std::lock_guard<std::mutex> g(globalMutex); // Lock global lock
  const int index = getIndex(hash(key));
  std::lock_guard<std::mutex> lk(mutexArr[index]);
  std::vector<std::string>::iterator it =
      std::find(hashMap[index].begin(), hashMap[index].end(), key);
  // Do nothing if the key doesn't exist.
  if (it == hashMap[index].end()) {
    return false;
  }
  // Else lock the bucket and erase the element at it.
  
  hashMap[index].erase(it);
  --count;
  return true;
}

void ChainHashMapRehashThreads::rehash() {
    int oldBuckets = getBuckets();
    auto oldHashMap = hashMap; // Only copy the old hashmap (not mutexes)

    doubleBuckets();
    doubleCapacity();

    int newBuckets = getBuckets();
    std::vector<std::vector<std::string>> newHashMap(newBuckets);
    std::vector<std::mutex> newBucketLocks(newBuckets);

    // Rebuild mutex array fresh
    mutexArr = std::vector<std::mutex>(newBuckets);

    // Parallel rehashing
    const int num_threads = 8;
    std::vector<std::thread> threads(num_threads);

    auto rehashTask = [&](int thread_id) {
        for (int i = thread_id; i < oldBuckets; i += num_threads) {
            for (const auto &key : oldHashMap[i]) {
                int newIndex = getIndex(hash(key));
                std::lock_guard<std::mutex> lock(newBucketLocks[newIndex]);
                newHashMap[newIndex].push_back(key);
            }
        }
    };

    for (int tid = 0; tid < num_threads; ++tid) {
        threads[tid] = std::thread(rehashTask, tid);
    }

    for (auto &t : threads) {
        t.join();
    }

    // Update the hashMap and bucketLocks
    hashMap = std::move(newHashMap);
    bucketLocks = std::move(newBucketLocks);
}


int ChainHashMapRehashThreads::size() const { return count; }

int ChainHashMapRehashThreads::getIndex(const int hash) const { return hash % getBuckets(); }

int ChainHashMapRehashThreads::hash(const std::string &s) const {
  /**
   * Polynomial hashing.
   * h = ( s[0] + s[1] * p + s[2] * p^2 + s[3] * p^3 + ... ) % mod.
   */
  const int p = 97;
  const long long int mod = 1e9 + 7;
  long long h = 0;
  long long pow = 1;
  for (char c : s) {
    h += ((c - '!' + 1) * pow) % mod;
    h %= mod;
    pow *= p;
    pow %= mod;
  }
  return h;
}

float ChainHashMapRehashThreads::getLoadFactor() const {
  return loadFactor;
}

int ChainHashMapRehashThreads::getBuckets() const {
  return BUCKETS;
}

int ChainHashMapRehashThreads::getMaxCapacity() const {
  return MAX_CAPACITY;
}

void ChainHashMapRehashThreads::doubleBuckets() {
  BUCKETS *= 2;
}

void ChainHashMapRehashThreads::doubleCapacity() {
  MAX_CAPACITY *= 2;
}

ChainHashMapRehashThreads::~ChainHashMapRehashThreads() {}