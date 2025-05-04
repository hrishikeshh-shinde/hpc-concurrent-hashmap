# CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(concurrent_hashmap VERSION 0.1.0 LANGUAGES CXX) # Specify language

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Recommended practice

# --- Configuration ---
set(SRC_DIR src)
set(TEST_DIR tests) # Assuming tests are in a 'tests' subdirectory

# --- Build HashMap Library ---
# Create a library from your hash map source files
add_library(hashmap_lib
    ${SRC_DIR}/AbstractHashMap.cpp
    ${SRC_DIR}/ThreadPartitionHashMap.cpp # Assumes final version is named this
)
# Make the include directory available to targets linking this library
target_include_directories(hashmap_lib PUBLIC ${SRC_DIR})
# Ensure C++17 for the library itself
target_compile_features(hashmap_lib PUBLIC cxx_std_17)
# Link pthreads to the library (needed for atomics, threads)
find_package(Threads REQUIRED)
target_link_libraries(hashmap_lib PUBLIC Threads::Threads)


# --- Build Test Executables ---

# 1. Partition HashMap Simple Correctness Test (Single-Threaded)
# Ensure the source file exists at this location
add_executable(partition_simple_test ${TEST_DIR}/PartitionHashMapSimpleTest.cpp)
target_link_libraries(partition_simple_test PRIVATE hashmap_lib) # Link against the map library
target_compile_features(partition_simple_test PRIVATE cxx_std_17)

# 2. Partition HashMap Timing Test (Multi-Threaded, File-Based)
# Ensure the source file exists at this location
add_executable(partition_timing_test ${TEST_DIR}/PartitionHashMapTimingTest.cpp)
target_link_libraries(partition_timing_test PRIVATE hashmap_lib) # Link against the map library
target_compile_features(partition_timing_test PRIVATE cxx_std_17)


# --- Commented out other old test targets ---
# add_executable(partition_correctness ${TEST_DIR}/PartitionHashMapCorrectnessTest.cpp) # Replaced by above
# add_executable(compilation ${TEST_DIR}/CompilationTest.cpp)
# target_link_libraries(compilation PRIVATE hashmap_lib)
# target_compile_features(compilation PRIVATE cxx_std_17)
# ... etc ...


# --- Optional: Installation ---
# Installs the two new test executables
install(TARGETS
    partition_simple_test
    partition_timing_test
    # ... other targets if uncommented ...
    DESTINATION bin)

# Installs header files from src/ to 'include' relative to CMAKE_INSTALL_PREFIX
install(FILES
    ${SRC_DIR}/AbstractHashMap.h
    ${SRC_DIR}/ThreadPartitionHashMap.h
    DESTINATION include)
