CXX = g++
CXXFLAGS = -Wall -O3 -std=c++17 -pthread

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

SRC_FILES = $(SRC_DIR)/AbstractHashMap.cpp $(SRC_DIR)/ThreadPartitionHashMap.cpp
INCLUDE = -I$(SRC_DIR)

# Build all tests
all: compilation read_heavy resize_stress benchmarks

# Basic compilation and correctness test
$(BUILD_DIR)/compilation: $(TEST_DIR)/CompilationTest.cpp $(SRC_FILES) $(SRC_DIR)/ThreadPartitionHashMap.h $(SRC_DIR)/AbstractHashMap.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(filter %.cpp,$^) -o $@

# Read-heavy concurrency test
$(BUILD_DIR)/read_heavy: $(TEST_DIR)/ReadHeavyTest.cpp $(SRC_FILES) $(SRC_DIR)/ThreadPartitionHashMap.h $(SRC_DIR)/AbstractHashMap.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(filter %.cpp,$^) -o $@

# Resize stress test with comparison
$(BUILD_DIR)/resize_stress_compare: ResizeStressTest.cpp $(SRC_FILES) $(SRC_DIR)/ThreadPartitionHashMap.h $(SRC_DIR)/AbstractHashMap.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(filter %.cpp,$^) -o $@

# Benchmark tests (comparison and scaling) 
$(BUILD_DIR)/benchmarks: BenchmarkTests.cpp $(SRC_FILES) $(SRC_DIR)/ThreadPartitionHashMap.h $(SRC_DIR)/AbstractHashMap.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(filter %.cpp,$^) -o $@


# Clean target
clean:
	rm -rf $(BUILD_DIR)

# Phony targets (prevent conflicts with files named 'all', 'clean', etc.)
.PHONY: all clean compilation read_heavy resize_stress benchmarks