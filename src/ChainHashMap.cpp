#include "ChainHashMap.h"
#include <algorithm>
#include <iterator>

ChainHashMap::ChainHashMap(float loadFactor) : AbstractHashMap(loadFactor) {
  hashMap = std::vector<std::list<std::string>>(BUCKETS);
}

bool ChainHashMap::insert(std::string key) {
  // If current loadFactor greater than desired, call rehash
  if (static_cast<float>(size() + 1) / BUCKETS > getLoadFactor()) {
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

void ChainHashMap::rehash() {
  BUCKETS *= 2;
  std::vector<std::list<std::string>> newHashMap(BUCKETS);
  // Re-distributing the elements into the newHashMap
  for (const auto &bucket : hashMap) {
    for (const auto &key : bucket) {
      const int newIndex = getIndex(hash(key));
      newHashMap[newIndex].push_back(key);
    }
  }
  // Move the newHashMap back into our old hashMap
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