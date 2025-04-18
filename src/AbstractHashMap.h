#ifndef ABSTRACT_HASH_MAP_H
#define ABSTRACT_HASH_MAP_H

#include <atomic>
#include <string>

/**
 * Abstract Hash Map class for std::string data.
 */
class AbstractHashMap {

public:
  // Constructor.
  AbstractHashMap();

  // Pure method to insert a key.
  virtual bool insert(std::string) = 0;

  // Pure method to search a key.
  virtual bool search(std::string) const = 0;

  // Pure method to remove a key.
  virtual bool remove(std::string) = 0;

  // Size.
  virtual int size() const = 0;

  // Pure destructor.
  virtual ~AbstractHashMap();

protected:
  // Total number of data elements in hash map.
  std::atomic<int> count;
};
#endif // ABSTRACT_HASH_MAP_H