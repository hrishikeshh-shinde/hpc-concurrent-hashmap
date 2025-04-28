#include "ChainHashMapRehashOpenMp.h"
#include <algorithm>
#include <iterator>
#include <iostream>
#include <omp.h>
#include <mutex>
#include <thread>

ChainHashMapRehashOpenMp::ChainHashMapRehashOpenMp(float loadFactor, int BUCKETS, int MAX_CAPACITY) : AbstractHashMap() {
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
  isRehashing = false;
}


bool ChainHashMapRehashOpenMp::insert(std::string key) {
  // If current loadFactor greater than desired, call rehash
  // std::lock_guard<std::mutex> g(globalMutex); // Lock global lock
  // Spin-wait if rehahsing is happening
  while (isRehashing) {
    std::this_thread::yield();
  }

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

bool ChainHashMapRehashOpenMp::search(std::string key) const {
  // std::lock_guard<std::mutex> g(globalMutex); // Lock global lock
  const int index = getIndex(hash(key));
  return std::find(hashMap[index].begin(), hashMap[index].end(), key) != hashMap[index].end();
}


bool ChainHashMapRehashOpenMp::remove(std::string key) {
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

void ChainHashMapRehashOpenMp::rehash() {
  // Try to acquire rehash lock without blocking others unnecessarily
  {
      std::unique_lock<std::mutex> lock(rehashMutex, std::try_to_lock);
      if (!lock.owns_lock() || isRehashing) {
          // Another thread is already rehashing
          return;
      }
      isRehashing = true;
  }

  // Double capacity under lock
  int oldBuckets, newBuckets;
  {
      std::lock_guard<std::mutex> lock(rehashMutex);
      oldBuckets = getBuckets();
      doubleBuckets();
      doubleCapacity();
      newBuckets = getBuckets();
  }

  // Create new hash map structure
  std::vector<std::vector<std::string>> newHashMap(newBuckets);
  
  // Parallel rehashing with OpenMP
  #pragma omp parallel
  {
      // Each thread processes a portion of the old buckets
      #pragma omp for schedule(dynamic)
      for (int i = 0; i < oldBuckets; ++i) {
          // Lock the old bucket while reading
          std::lock_guard<std::mutex> oldLock(mutexArr[i]);
          
          // Create local copy of this bucket's contents
          auto bucketCopy = hashMap[i];
          
          // Release old bucket lock before working with new buckets
          oldLock.~lock_guard(); // Manual destruction to release early
          
          for (const auto &key : bucketCopy) {
              int newIndex = getIndex(hash(key));
              
              // Lock the new bucket while writing
              std::lock_guard<std::mutex> newLock(mutexArr[newIndex]);
              newHashMap[newIndex].push_back(key);
          }
      }
  }

  // Final update under lock
  {
      std::lock_guard<std::mutex> lock(rehashMutex);
      hashMap = std::move(newHashMap);
      isRehashing = false;
  }
}


int ChainHashMapRehashOpenMp::size() const { return count; }

int ChainHashMapRehashOpenMp::getIndex(const int hash) const { return hash % getBuckets(); }

int ChainHashMapRehashOpenMp::hash(const std::string &s) const {
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

float ChainHashMapRehashOpenMp::getLoadFactor() const {
  return loadFactor;
}

int ChainHashMapRehashOpenMp::getBuckets() const {
  return BUCKETS;
}

int ChainHashMapRehashOpenMp::getMaxCapacity() const {
  return MAX_CAPACITY;
}

void ChainHashMapRehashOpenMp::doubleBuckets() {
  BUCKETS *= 2;
}

void ChainHashMapRehashOpenMp::doubleCapacity() {
  MAX_CAPACITY *= 2;
}

ChainHashMapRehashOpenMp::~ChainHashMapRehashOpenMp() {}