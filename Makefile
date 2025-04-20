CXX = g++
CXXFLAGS = -Wall -O3 -std=c++17 -pthread 

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

# --- Source Files ---
HASHMAP_SRC = $(SRC_DIR)/ThreadPartitionHashMap.cpp 
SRC_FILES = $(SRC_DIR)/AbstractHashMap.cpp $(HASHMAP_SRC)

# --- Header Files / Include Path ---
HASHMAP_HEADER = $(SRC_DIR)/ThreadPartitionHashMap.h 
ABSTRACT_HEADER = $(SRC_DIR)/AbstractHashMap.h
INCLUDE = -I$(SRC_DIR) 

# --- Test Executables ---
TEST_EXECUTABLES = compilation read_heavy resize_stress benchmarks

# --- Default Target ---
all: $(BUILD_DIR) $(addprefix $(BUILD_DIR)/, $(TEST_EXECUTABLES))

# --- Build Directory Rule ---
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Basic compilation and correctness test
$(BUILD_DIR)/compilation: $(TEST_DIR)/CompilationTest.cpp $(SRC_FILES) $(HASHMAP_HEADER) $(ABSTRACT_HEADER) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(filter %.cpp,$^) -o $@

# Read-heavy concurrency test
$(BUILD_DIR)/read_heavy: $(TEST_DIR)/ReadHeavyTest.cpp $(SRC_FILES) $(HASHMAP_HEADER) $(ABSTRACT_HEADER) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(filter %.cpp,$^) -o $@

# Resize stress test with comparison 
$(BUILD_DIR)/resize_stress: $(TEST_DIR)/ResizeStressTest.cpp $(SRC_FILES) $(HASHMAP_HEADER) $(ABSTRACT_HEADER) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(filter %.cpp,$^) -o $@

# Benchmark tests (comparison and scaling)
$(BUILD_DIR)/benchmarks: $(TEST_DIR)/BenchMarkTest.cpp $(SRC_FILES) $(HASHMAP_HEADER) $(ABSTRACT_HEADER) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(filter %.cpp,$^) -o $@

# --- Clean Target ---
clean:
	rm -rf $(BUILD_DIR)

# --- Phony Targets ---
.PHONY: all clean $(TEST_EXECUTABLES)
