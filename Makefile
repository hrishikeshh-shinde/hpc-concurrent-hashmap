CXX = g++
CXXFLAGS = -Wall -O3 -std=c++17 -pthread

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

SRC_FILES = $(SRC_DIR)/AbstractHashMap.cpp $(SRC_DIR)/ThreadPartitionHashMap.cpp
INCLUDE = -I$(SRC_DIR)

# Build all tests
all: compilation read_heavy

# Individual test targets
compilation: $(TEST_DIR)/CompilationTest.cpp $(SRC_FILES)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $^ -o $(BUILD_DIR)/$@

read_heavy: $(TEST_DIR)/ReadHeavyTest.cpp $(SRC_FILES)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $^ -o $(BUILD_DIR)/$@


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean compilation read_heavy