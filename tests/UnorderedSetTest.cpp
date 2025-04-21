#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>

/* A single threaded test application to test unordered_set. */
int main(int argc, char *argv[]) {

  std::unordered_set<std::string> h;
  std::vector<std::pair<std::string, bool>> tests;
  std::string s;
  bool toInsert;
  std::chrono::high_resolution_clock::time_point start, end;
  std::chrono::duration<double, std::milli> time;

  // Test insertion.
  std::ifstream insertFile("testdata/insert.txt");
  while (insertFile >> s >> toInsert) {
    tests.push_back({s, toInsert});
  }
  insertFile.close();

  const int N = tests.size();

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < tests.size(); ++i) {
    if (tests[i].second) {
      h.insert(tests[i].first);
    }
  }
  assert(h.size() == N / 2);
  end = std::chrono::high_resolution_clock::now();

  time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
      end - start);
  std::cout << "Insertion time: " << time.count() << " ms.\n";

  // Test search.
  tests.clear();
  std::ifstream searchFile("testdata/search.txt");
  while (searchFile >> s >> toInsert) {
    tests.push_back({s, toInsert});
  }
  searchFile.close();

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < tests.size(); ++i) {
    assert((h.find(tests[i].first) != h.end() ? true : false) ==
           tests[i].second);
  }
  end = std::chrono::high_resolution_clock::now();

  time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
      end - start);
  std::cout << "Search time: " << time.count() << " ms.\n";

  // Test deletion.
  tests.clear();
  std::ifstream deletionFile("testdata/delete.txt");
  while (deletionFile >> s >> toInsert) {
    tests.push_back({s, toInsert});
  }
  deletionFile.close();

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < tests.size(); ++i) {
    assert(h.erase(tests[i].first) == tests[i].second);
  }
  assert(h.size() == 0);
  end = std::chrono::high_resolution_clock::now();

  time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
      end - start);
  std::cout << "Deletion time: " << time.count() << " ms.\n";
}