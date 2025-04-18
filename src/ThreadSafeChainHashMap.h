#ifndef THREAD_SAFE_CHAIN_HASH_MAP_H
#define THREAD_SAFE_CHAIN_HASH_MAP_H
#include "AbstractHashMap.h"
#include <list>
#include <mutex>
#include <vector>

/**
 * A thread safe chain hashmap implementation.
 */
class ThreadSafeChainHashMap : public AbstractHashMap {

public:
  // Constructor.
  ThreadSafeChainHashMap();

  // Insertion.
  bool insert(std::string);

  // Search.
  bool search(std::string) const;

  // Deletion.
  bool remove(std::string);

  // Size.
  int size() const;

  // Destructor.
  ~ThreadSafeChainHashMap();

private:
  // Totat number of initial buckets for hash map.
  // For 1e7 elements, avg bucket size would be 10.
  const int BUCKETS = 1024 * 1024;

  // The hash map data structure behind the scenes.
  std::vector<std::list<std::string>> hashMap;

  // Locks to protect access to each of the buckets.
  std::vector<std::mutex> mutexArr;

  // A utility method to compute the hash of a given string.
  int hash(const std::string &) const;

  // A utility method to compute the index of a hash in the hash map.
  int getIndex(const int hash) const;
};
#endif // THREAD_SAFE_CHAIN_HASH_MAP_H