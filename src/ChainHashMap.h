#ifndef CHAIN_HASH_MAP_H
#define CHAIN_HASH_MAP_H
#include "AbstractHashMap.h"
#include <list>
#include <vector>

/**
 * A single threaded hashmap implementation.
 */
class ChainHashMap : public AbstractHashMap {

public:
  // Constructor.
  ChainHashMap();

  // Insertion.
  bool insert(std::string);

  // Search.
  bool search(std::string) const;

  // Deletion.
  bool remove(std::string);

  // Size.
  int size() const;

  // Destructor.
  ~ChainHashMap();

private:
  // Totat number of initial buckets for hash map.
  // For 1e7 elements, avg bucket size would be 10.
  const int BUCKETS = 1024 * 1024;

  // The hash map data structure behind the scenes.
  std::vector<std::list<std::string>> hashMap;

  // A utility method to compute the hash of a given string.
  int hash(const std::string &) const;

  // A utility method to compute the index of a hash in the hash map.
  int getIndex(const int hash) const;
};
#endif // CHAIN_HASH_MAP_H