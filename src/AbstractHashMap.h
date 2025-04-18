#ifndef ABSTRACT_HASH_MAP_H
#define ABSTRACT_HASH_MAP_H

#include <string>

/**
 * Abstract Hash Map class for std::string data.
 */
class AbstractHashMap {

public:
  // Constructor.
  AbstractHashMap(float, int, int); //loadFactor, BUCKETS, MAX_CAPACITY

  // Pure method to insert a key.
  virtual bool insert(std::string) = 0;

  // Pure method to search a key.
  virtual bool search(std::string) const = 0;

  // Pure method to remove a key.
  virtual bool remove(std::string) = 0;

  // Size.
  virtual int size() const = 0;

  // Re-hashing
  virtual void rehash() = 0;

  // To get loadFactor to determine if re-hashing needed
  float getLoadFactor() const;

  int getBuckets() const;

  int getMaxCapacity() const;

  void doubleBuckets();

  void doubleCapacity();

  // Pure destructor.
  virtual ~AbstractHashMap();

private:
  // A value between 0 and 1(inclusive) to determine the load at which a
  // hash map should resize.
  float loadFactor;
  int BUCKETS;
  int MAX_CAPACITY;

protected:
  // Total number of data elements in hash map.
  int count;
};
#endif // ABSTRACT_HASH_MAP_H