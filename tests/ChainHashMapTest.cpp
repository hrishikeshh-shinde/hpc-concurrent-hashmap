#include "../src/ChainHashMap.h"
#include <cassert>
#include <iostream>

/**
 * A single threaded test application to test single threaded ChainHashMap.
 */
int main(int argc, char *argv[]) {

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

  std::cout << "PASSED" << "\n";
}