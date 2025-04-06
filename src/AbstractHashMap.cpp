#include "AbstractHashMap.h"

AbstractHashMap::AbstractHashMap(float loadFactor) {
  if (loadFactor < 0 or loadFactor > 1) {
    std::__throw_out_of_range("load factor value is out of range.");
  }
  this->loadFactor = loadFactor;
  this->count = 0;
}

float AbstractHashMap::getLoadFactor() const {
  return loadFactor;
}

AbstractHashMap::~AbstractHashMap() {}