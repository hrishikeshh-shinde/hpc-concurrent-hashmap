#include <cassert>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

/**
 * A multi-threaded test application to test single-threaded thread-safe
 * unordered_set.
 */
std::vector<std::pair<std::string, bool>> tests;
std::mutex mtx;

void test_insert(int start, int n, std::unordered_set<std::string> &h) {
  for (int i = start; i <= n + start - 1; ++i) {
    if (tests[i].second) {
      std::lock_guard<std::mutex> lk(mtx);
      h.insert(tests[i].first);
    }
  }
}

void test_search(int start, int n, std::unordered_set<std::string> &h) {
  for (int i = start; i <= n + start - 1; ++i) {
    assert((h.find(tests[i].first) != h.end() ? true : false) ==
           tests[i].second);
  }
}

void test_remove(int start, int n, std::unordered_set<std::string> &h) {
  for (int i = start; i <= n + start - 1; ++i) {
    std::lock_guard<std::mutex> lk(mtx);
    assert(h.erase(tests[i].first) == tests[i].second);
  }
}

int main(int argc, char *argv[]) {
  std::unordered_set<std::string> h;
  std::string s;
  bool toInsert;
  std::chrono::high_resolution_clock::time_point start, end;
  std::chrono::duration<double, std::milli> time;
  int cores = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;

  // Test insertion.
  std::ifstream insertFile("testdata/insert.txt");
  while (insertFile >> s >> toInsert) {
    tests.push_back({s, toInsert});
  }
  insertFile.close();

  const int N = tests.size();

  start = std::chrono::high_resolution_clock::now();
  int p = 0;
  for (int i = 1; i <= cores - 1; ++i) {
    threads.push_back(std::thread(test_insert, p, N / cores, std::ref(h)));
    p += N / cores;
  }
  threads.push_back(
      std::thread(test_insert, p, N / cores + N % cores, std::ref(h)));
  for (auto &t : threads) {
    t.join();
  }
  assert(h.size() == N / 2);
  end = std::chrono::high_resolution_clock::now();

  time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
      end - start);
  std::cout << "Insertion time: " << time.count() << " ms.\n";

  // Test search.
  tests.clear();
  threads.clear();
  std::ifstream searchFile("testdata/search.txt");
  while (searchFile >> s >> toInsert) {
    tests.push_back({s, toInsert});
  }
  searchFile.close();

  start = std::chrono::high_resolution_clock::now();
  p = 0;
  for (int i = 1; i <= cores - 1; ++i) {
    threads.push_back(std::thread(test_search, p, N / cores, std::ref(h)));
    p += N / cores;
  }
  threads.push_back(
      std::thread(test_search, p, N / cores + N % cores, std::ref(h)));
  for (auto &t : threads) {
    t.join();
  }
  end = std::chrono::high_resolution_clock::now();

  time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
      end - start);
  std::cout << "Search time: " << time.count() << " ms.\n";

  // Test deletion.
  tests.clear();
  threads.clear();
  std::ifstream deletionFile("testdata/delete.txt");
  while (deletionFile >> s >> toInsert) {
    tests.push_back({s, toInsert});
  }
  deletionFile.close();

  start = std::chrono::high_resolution_clock::now();
  p = 0;
  for (int i = 1; i <= cores - 1; ++i) {
    threads.push_back(std::thread(test_remove, p, N / cores, std::ref(h)));
    p += N / cores;
  }
  threads.push_back(
      std::thread(test_remove, p, N / cores + N % cores, std::ref(h)));
  for (auto &t : threads) {
    t.join();
  }
  assert(h.size() == 0);
  end = std::chrono::high_resolution_clock::now();
  std::cout << "Deletion time: " << time.count() << " ms.\n";
  return 0;
}