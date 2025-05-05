 Compiler and Flags
CXX = g++
# Common flags: -Wall (warnings), -O3 (optimization), -std=c++17, -pthread (for std::thread/atomics)
CXXFLAGS = -Wall -O3 -std=c++17 -pthread -g # Added -g for debugging symbols

# Directories
SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

# Source Files for the Library
LIB_SRC = $(SRC_DIR)/AbstractHashMap.cpp $(SRC_DIR)/ThreadPartitionHashMap.cpp

# Headers (Optional but good practice for explicit dependencies)
LIB_HEADERS = $(SRC_DIR)/AbstractHashMap.h $(SRC_DIR)/ThreadPartitionHashMap.h

# Include Path
INCLUDE = -I$(SRC_DIR)

# Target Executables
SIMPLE_TEST_EXEC = partition_simple_test
TIMING_TEST_EXEC = partition_timing_test
TARGETS = $(SIMPLE_TEST_EXEC) $(TIMING_TEST_EXEC)

# --- Default Target ---
# Builds all specified target executables
all: $(BUILD_DIR) $(addprefix $(BUILD_DIR)/, $(TARGETS))

# --- Build Directory Rule ---
# Creates the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# --- Rule for Simple Correctness Test ---
# Depends on its source, the library sources, and headers. Requires build dir.
$(BUILD_DIR)/$(SIMPLE_TEST_EXEC): $(TEST_DIR)/PartitionHashMapSimpleTest.cpp $(LIB_SRC) $(LIB_HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(TEST_DIR)/PartitionHashMapSimpleTest.cpp $(LIB_SRC) -o $@

# --- Rule for Timing Test ---
# Depends on its source, the library sources, and headers. Requires build dir.
$(BUILD_DIR)/$(TIMING_TEST_EXEC): $(TEST_DIR)/PartitionHashMapTimingTest.cpp $(LIB_SRC) $(LIB_HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(TEST_DIR)/PartitionHashMapTimingTest.cpp $(LIB_SRC) -o $@

# --- Clean Target ---
# Removes the build directory and its contents
clean:
	rm -rf $(BUILD_DIR)

# --- Phony Targets ---
# Declares targets that are not files
.PHONY: all clean $(TARGETS)

