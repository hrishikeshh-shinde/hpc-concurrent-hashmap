#include "ThreadSafeChainHashMap.h"
#include <algorithm>
#include <iostream>
#include <iterator>

ThreadSafeChainHashMap::ThreadSafeChainHashMap() : AbstractHashMap() {
  hashMap = std::vector<std::list<std::string>>(BUCKETS);
  mutexArr = std::vector<std::mutex>(BUCKETS);
}

bool ThreadSafeChainHashMap::insert(std::string key) {
  const int index = getIndex(hash(key));
  std::lock_guard<std::mutex> lk(mutexArr[index]);
  hashMap[index].push_back(key);
  ++count;
  return true;
}

bool ThreadSafeChainHashMap::search(std::string key) const {
  const int index = getIndex(hash(key));
  return std::find(hashMap[index].begin(), hashMap[index].end(), key) !=
         hashMap[index].end();
}

bool ThreadSafeChainHashMap::remove(std::string key) {
  const int index = getIndex(hash(key));
  std::list<std::string>::iterator it =
      std::find(hashMap[index].begin(), hashMap[index].end(), key);
  // Do nothing if the key doesn't exist.
  if (it == hashMap[index].end()) {
    return false;
  }
  // Else lock the bucket and erase the element at it.
  std::lock_guard<std::mutex> lk(mutexArr[index]);
  hashMap[index].erase(it);
  --count;
  return true;
}

int ThreadSafeChainHashMap::size() const { return count; }

int ThreadSafeChainHashMap::getIndex(const int hash) const {
  return hash % BUCKETS;
}

int ThreadSafeChainHashMap::hash(const std::string &s) const {
  /**
   * Polynomial hashing.
   * h = ( s[0] + s[1] * p + s[2] * p^2 + s[3] * p^3 + ... ) % mod.
   */
  const int p = 97;
  const long long int mod = 1e9 + 7;
  long long h = 0;
  long long pow = 1;
  for (char c : s) {
    h += ((c - '!' + 1) * pow) % mod;
    h %= mod;
    pow *= p;
    pow %= mod;
  }
  return h;
}

ThreadSafeChainHashMap::~ThreadSafeChainHashMap() {}