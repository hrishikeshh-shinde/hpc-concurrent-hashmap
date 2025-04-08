#include "../src/ChainHashMap.h"
#include <cassert>
#include <iostream>

void testBaseHashMap() {
  ChainHashMap h1(0.5);

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
  ChainHashMap h1(0.05);
  
  assert(h1.insert("test"));
  assert(h1.insert("abc"));
  assert(h1.insert("pqrs"));
  assert(h1.insert("wxyz"));
  assert(h1.insert("wxyz"));

  assert(h1.size() == 5);

  assert(h1.insert("test1"));
  assert(h1.insert("abc2"));
  assert(h1.insert("pqrs3"));
  assert(h1.insert("wxyz4"));
  
  assert(h1.getBuckets() == 200);
  assert(h1.size() == 9);

  assert(h1.search("test"));
  assert(h1.search("abc"));
  assert(h1.search("pqrs"));
  assert(h1.search("wxyz"));
  assert(h1.search("wxyz"));
  assert(h1.search("test1"));
  assert(h1.search("abc2"));
  assert(h1.search("pqrs3"));
  assert(h1.search("wxyz4"));

  std::cout << "testRehash() PASSED" << "\n";
}

/**
 * A single threaded test application to test single threaded ChainHashMap.
 */
int main(int argc, char *argv[]) {
  testBaseHashMap();
  testRehash();
}