#include "../src/ChainHashMapRehash.h"
#include <cassert>
#include <iostream>

void testBaseHashMap() {
  ChainHashMapRehash h1(0.5, 10, 100);

  assert(h1.insert("test"));
  assert(h1.insert("abc"));
  assert(h1.insert("pqrs"));
  assert(h1.insert("wxyz"));
  assert(h1.insert("wxyz"));

  assert(h1.size() == 5);
  assert(h1.search("abc"));
  assert(h1.search("cdef") == false);
  assert(h1.search("mno") == false);

  assert(h1.remove("wxyz"));
  assert(h1.remove("abc"));
  assert(h1.remove("efg") == false);
  assert(h1.size() == 3);

  std::cout << "testBaseHashMap() PASSED" << "\n";
}

void testRehash() {
  ChainHashMapRehash h1(0.05, 10, 100);
  
  assert(h1.insert("test"));
  assert(h1.insert("abc"));
  assert(h1.insert("pqrs"));
  assert(h1.insert("wxyz"));
  assert(h1.insert("erty"));

  assert(h1.size() == 5);

  assert(h1.getBuckets() == 10);
  assert(h1.insert("test1"));
  assert(h1.getBuckets() == 20);

  assert(h1.size() == 6);

  std::cout << "testRehash() PASSED" << "\n";
}

/**
 * A single threaded test application to test single threaded ChainHashMap.
 */
int main(int argc, char *argv[]) {
  testBaseHashMap();
  testRehash();
}