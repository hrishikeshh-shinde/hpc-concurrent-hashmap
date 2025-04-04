#include "AbstractHashMap.h"

AbstractHashMap::AbstractHashMap(int loadFactor) {
  if (loadFactor < 0 or loadFactor > 100) {
    std::__throw_out_of_range("load factor value is out of range.");
  }
  this->loadFactor = loadFactor;
  this->count = 0;
}

AbstractHashMap::~AbstractHashMap() {}