#include "ChainHashMap.h"
#include <algorithm>
#include <iterator>
#include <iostream>
#include <omp.h>

ChainHashMap::ChainHashMap(float loadFactor) : AbstractHashMap(loadFactor) {
  hashMap = std::vector<std::list<std::string>>(BUCKETS);
}

bool ChainHashMap::insert(std::string key) {
  std::cout << key << " B=" << getBuckets() << " Sz=" << size() << " maxcap=" << MAX_CAPACITY << std::endl;
  // If current loadFactor greater than desired, call rehash
  if (size() + 1 > static_cast<int>(getLoadFactor() * MAX_CAPACITY)) {
    rehash();
  }

  const int index = getIndex(hash(key));
  hashMap[index].push_back(key);
  ++count;
  return true;
}

bool ChainHashMap::search(std::string key) const {
  const int index = getIndex(hash(key));
  return std::find(hashMap[index].begin(), hashMap[index].end(), key) !=
         hashMap[index].end();
}

bool ChainHashMap::remove(std::string key) {
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

// void ChainHashMap::rehash() {
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

void ChainHashMap::rehash() {
  std::cout << "DOUBLE" << std::endl;
  BUCKETS *= 2;
  MAX_CAPACITY *= 2;
  std::vector<std::list<std::string>> newHashMap(BUCKETS);

  int num_threads = omp_get_max_threads();
  std::vector<std::vector<std::list<std::string>>> localHashMaps(num_threads, std::vector<std::list<std::string>>(BUCKETS));

  #pragma omp parallel
  {
      int tid = omp_get_thread_num();
      #pragma omp for schedule(dynamic)
      for (size_t i = 0; i < hashMap.size(); ++i) {
          for (const auto &key : hashMap[i]) {
              int newIndex = getIndex(hash(key));
              localHashMaps[tid][newIndex].push_back(key);
          }
      }
  }

  for (int tid = 0; tid < num_threads; ++tid) {
      for (size_t bucketIdx = 0; bucketIdx < BUCKETS; ++bucketIdx) {
          if (!localHashMaps[tid][bucketIdx].empty()) {
              newHashMap[bucketIdx].splice(
                  newHashMap[bucketIdx].end(),
                  localHashMaps[tid][bucketIdx]);
          }
      }
  }

  hashMap = std::move(newHashMap);
}

int ChainHashMap::size() const { return count; }

int ChainHashMap::getIndex(const int hash) const { return hash % BUCKETS; }

int ChainHashMap::hash(const std::string &s) const {
  /**
   * Polynomial hashing.
   * h = ( s[0] + s[1] * p + s[2] * p^2 + s[3] * p^3 + ... ) % mod.
   */
  const int p = 97;
  const int mod = 1e9 + 7;
  int h = 0;
  int pow = 1;
  for (char c : s) {
    h += ((c - '!' + 1) * pow) % mod;
    pow *= p;
    pow %= mod;
  }
  return h;
}

int ChainHashMap::getBuckets() const {
  return BUCKETS;
}

ChainHashMap::~ChainHashMap() {}