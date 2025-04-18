#include "AbstractHashMap.h"

AbstractHashMap::AbstractHashMap(float loadFactor, int BUCKETS, int MAX_CAPACITY) {
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
}

float AbstractHashMap::getLoadFactor() const {
  return loadFactor;
}

int AbstractHashMap::getBuckets() const {
  return BUCKETS;
}

int AbstractHashMap::getMaxCapacity() const {
  return MAX_CAPACITY;
}

void AbstractHashMap::doubleBuckets() {
  BUCKETS *= 2;
}

void AbstractHashMap::doubleCapacity() {
  MAX_CAPACITY *= 2;
}


AbstractHashMap::~AbstractHashMap() {}