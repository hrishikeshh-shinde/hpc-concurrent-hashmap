#ifndef CHAIN_HASH_MAP_REHASH_H
#define CHAIN_HASH_MAP_REHASH_H
#include "AbstractHashMap.h"
#include <list>
#include <vector>
#include <mutex>

class ChainHashMapRehashThreads : public AbstractHashMap {

public:
  ChainHashMapRehashThreads(float, int, int); //loadFactor, BUCKETS, MAX_CAPACITY
  bool insert(std::string);
  bool search(std::string) const;
  bool remove(std::string);
  // Re-hashing
  void rehash();
  int size() const;
  float getLoadFactor() const; // To get loadFactor to determine if re-hashing needed
  int getBuckets() const;
  int getMaxCapacity() const;
  void doubleBuckets();
  void doubleCapacity();
  ~ChainHashMapRehashThreads();

private:
  // A value between 0 and 1(inclusive) to determine the load at which a
  // hash map should resize.
  float loadFactor;
  int BUCKETS;
  int MAX_CAPACITY;

  // The hash map data structure behind the scenes.
  std::vector<std::vector<std::string>> hashMap;

  // mutex for each bucket
  std::vector<std::mutex> bucketLocks;

  // Locks to protect access to each of the buckets.
  mutable std::vector<std::mutex> mutexArr;

  // global rehash lock
  std::mutex rehashMutex;

  // rehash flag
  bool isRehashing = false;

  // A utility method to compute the hash of a given string.
  int hash(const std::string &) const;

  // A utility method to compute the index of a hash in the hash map.
  int getIndex(const int hash) const;
};
#endif // CHAIN_HASH_MAP_REHASH_H