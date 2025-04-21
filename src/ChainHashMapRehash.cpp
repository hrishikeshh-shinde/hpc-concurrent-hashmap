#include "ChainHashMapRehash.h"
#include <algorithm>
#include <iterator>
#include <iostream>
#include <omp.h>
#include <mutex>

ChainHashMapRehash::ChainHashMapRehash(float loadFactor, int BUCKETS, int MAX_CAPACITY) : AbstractHashMap() {
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
  hashMap = std::vector<std::list<std::string>>(getBuckets());
}

bool ChainHashMapRehash::insert(std::string key) {
  std::cout << key << " B=" << getBuckets() << " Sz=" << size() << " maxcap=" << getMaxCapacity() << std::endl;
  // If current loadFactor greater than desired, call rehash
  if (size() + 1 > static_cast<int>(getLoadFactor() * getMaxCapacity())) {
    rehash();
  }

  const int index = getIndex(hash(key));
  hashMap[index].push_back(key);
  ++count;
  return true;
}

bool ChainHashMapRehash::search(std::string key) const {
  const int index = getIndex(hash(key));
  return std::find(hashMap[index].begin(), hashMap[index].end(), key) !=
         hashMap[index].end();
}

bool ChainHashMapRehash::remove(std::string key) {
  const int index = getIndex(hash(key));
  std::list<std::string>::iterator it =
      std::find(hashMap[index].begin(), hashMap[index].end(), key);
  // Do nothing if the key doesn't exist.
  if (it == hashMap[index].end()) {
    return false;
  }
  hashMap[index].erase(it);
  --count;
  return true;
}

// Base version
// void ChainHashMapRehash::rehash() {
//   BUCKETS *= 2;
//   MAX_CAPACITY *= 2;
//   std::vector<std::list<std::string>> newHashMap(BUCKETS);
//   // Re-distributing the elements into the newHashMap
//   for (const auto &bucket : hashMap) {
//     for (const auto &key : bucket) {
//       const int newIndex = getIndex(hash(key));
//       newHashMap[newIndex].push_back(key);
//     }
//   }
//   // Move the newHashMap back into our old hashMap
//   hashMap = std::move(newHashMap);
// }

// OpenMp Version
// void ChainHashMapRehash::rehash() {
//   std::cout << "DOUBLE" << std::endl;
//   doubleBuckets();
//   doubleCapacity();
//   int Buckets = getBuckets();
//   std::vector<std::list<std::string>> newHashMap(Buckets);

//   int num_threads = omp_get_max_threads();
//   std::vector<std::vector<std::list<std::string>>> localHashMaps(num_threads, std::vector<std::list<std::string>>(Buckets));

//   #pragma omp parallel
//   {
//       int tid = omp_get_thread_num();
//       #pragma omp for schedule(dynamic)
//       for (size_t i = 0; i < hashMap.size(); ++i) {
//           for (const auto &key : hashMap[i]) {
//               int newIndex = getIndex(hash(key));
//               localHashMaps[tid][newIndex].push_back(key);
//           }
//       }
//   }

//   for (int tid = 0; tid < num_threads; ++tid) {
//       for (size_t bucketIdx = 0; bucketIdx < Buckets; ++bucketIdx) {
//           if (!localHashMaps[tid][bucketIdx].empty()) {
//               newHashMap[bucketIdx].splice(
//                   newHashMap[bucketIdx].end(),
//                   localHashMaps[tid][bucketIdx]);
//           }
//       }
//   }

//   hashMap = std::move(newHashMap);
// }

// Thread Version
void ChainHashMapRehash::rehash() {
    int oldBuckets = getBuckets();
    doubleBuckets();
    doubleCapacity();

    std::vector<std::list<std::string>> newHashMap(getBuckets());
    std::vector<std::mutex> newBucketLocks(getBuckets());

    // Number of threads (e.g., 4)
    const int num_threads = 4;
    std::vector<std::thread> threads(num_threads);

    // Lambda for thread operation
    auto rehashTask = [&](int thread_id) {
        // Each thread processes a subset of old buckets
        for (int i = thread_id; i < oldBuckets; i += num_threads) {
            for (const auto &key : hashMap[i]) {
                int newIndex = getIndex(hash(key));

                // Lock only the bucket you're inserting into
                std::lock_guard<std::mutex> lock(newBucketLocks[newIndex]);
                newHashMap[newIndex].push_back(key);
            }
        }
    };

    // Launch threads
    for (int tid = 0; tid < num_threads; ++tid) {
        threads[tid] = std::thread(rehashTask, tid);
    }

    // Join threads
    for (auto &t : threads) {
        t.join();
    }

    // Move new map into old map
    hashMap = std::move(newHashMap);
    bucketLocks = std::move(newBucketLocks);
}

int ChainHashMapRehash::size() const { return count; }

int ChainHashMapRehash::getIndex(const int hash) const { return hash % getBuckets(); }

int ChainHashMapRehash::hash(const std::string &s) const {
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

float ChainHashMapRehash::getLoadFactor() const {
  return loadFactor;
}

int ChainHashMapRehash::getBuckets() const {
  return BUCKETS;
}

int ChainHashMapRehash::getMaxCapacity() const {
  return MAX_CAPACITY;
}

void ChainHashMapRehash::doubleBuckets() {
  BUCKETS *= 2;
}

void ChainHashMapRehash::doubleCapacity() {
  MAX_CAPACITY *= 2;
}

ChainHashMapRehash::~ChainHashMapRehash() {}