#ifndef CHAIN_HASH_MAP_H
#define CHAIN_HASH_MAP_H
#include "AbstractHashMap.h"
#include <list>
#include <vector>

/**
 * A single threaded hashmap example.
 */
class ChainHashMap : public AbstractHashMap {

public:
  ChainHashMap(float);
  bool insert(std::string);
  bool search(std::string) const;
  bool remove(std::string);
  void rehash();
  int size() const;
  ~ChainHashMap();

private:
  // Totat number of initial buckets for hash map.
  int BUCKETS = 100;

  // The hash map data structure behind the scenes.
  std::vector<std::list<std::string>> hashMap;

  // A utility method to compute the hash of a given string.
  int hash(const std::string &) const;

  // A utility method to compute the index of a hash in the hash map.
  int getIndex(const int hash) const;
};
#endif // CHAIN_HASH_MAP_H