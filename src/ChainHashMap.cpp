#include "ChainHashMap.h"
#include <algorithm>
#include <iterator>

ChainHashMap::ChainHashMap(int loadFactor) : AbstractHashMap(loadFactor) {
  hashMap = std::vector<std::list<std::string>>(BUCKETS);
}

bool ChainHashMap::insert(std::string key) {
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

ChainHashMap::~ChainHashMap() {}