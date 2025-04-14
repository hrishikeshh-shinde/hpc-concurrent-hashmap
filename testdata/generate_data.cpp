#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

std::string generate_string() {
  std::string s;
  // Generate a random length between 1 and 100;
  int len = rand() % 100 + 1;
  for (uint16_t i = 1; i <= len; ++i) {
    // Generate a random character between ascii values of 33 and 122 inclusive.
    int ascii = rand() % 90 + 33;
    s += char(ascii);
  }
  return s;
}

const int N = 1e6;

int main(void) {
  srand(time(nullptr));

  std::vector<std::pair<std::string, bool>> tests(N);
  std::string s;
  std::map<std::string, bool> exists;
  // Generate unique strings.
  for (uint32_t i = 0; i <= N - 1; ++i) {
    s = generate_string();
    if (!exists[s]) {
      tests[i].first = s;
      exists[s] = true;
    } else {
      --i;
    }
  }

  // Let half of them insert into the hashmap.
  for (uint32_t i = 0; i <= N / 2 - 1; ++i) {
    tests[i].second = true;
  }

  // File for inserts.
  random_shuffle(tests.begin(), tests.end());
  std::ofstream insertFile("insert.txt");
  for (uint32_t i = 0; i <= N - 1; ++i) {
    insertFile << tests[i].first << " " << tests[i].second << "\n";
  }
  insertFile.close();

  // File for search.
  random_shuffle(tests.begin(), tests.end());
  std::ofstream searchFile("search.txt");
  for (uint32_t i = 0; i <= N - 1; ++i) {
    searchFile << tests[i].first << " " << tests[i].second << "\n";
  }
  searchFile.close();

  // File for deletion.
  random_shuffle(tests.begin(), tests.end());
  std::ofstream deleteFile("delete.txt");
  for (uint32_t i = 0; i <= N - 1; ++i) {
    deleteFile << tests[i].first << " " << tests[i].second << "\n";
  }
  deleteFile.close();
  return 0;
}