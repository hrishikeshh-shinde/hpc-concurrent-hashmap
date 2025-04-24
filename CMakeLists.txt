# CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(concurrent_hashmap VERSION 0.1.0 LANGUAGES CXX) # Specify language

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Recommended practice

# --- Configuration ---
# Assuming headers are in 'src' and test sources are in 'tests'
set(SRC_DIR src)
set(TEST_DIR tests)
# Assuming new test files are also moved to tests/
# If ResizeStressTest_compare.cpp etc. are in root, remove $(TEST_DIR)/ below

# Include directory for headers (src contains .h files)
# Use target_include_directories for better practice
# include_directories(${SRC_DIR}) # Old way, less preferred

# Enable optimizations for release builds (CMake handles this better with build types)
# Example: cmake .. -DCMAKE_BUILD_TYPE=Release
# But keeping user's flag setting for now:
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

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
# Link pthreads to the library (needed for atomics, mutexes, threads)
find_package(Threads REQUIRED)
target_link_libraries(hashmap_lib PUBLIC Threads::Threads)


# --- Build Test Executables ---

# 1. Compilation Test (Correctness)
add_executable(compilation ${TEST_DIR}/CompilationTest.cpp)
target_link_libraries(compilation PRIVATE hashmap_lib) # Link against the map library
target_compile_features(compilation PRIVATE cxx_std_17)

# 2. Read-Heavy Test (with Comparison)
add_executable(read_heavy_compare ${TEST_DIR}/ReadHeavyTest_compare.cpp) # Use correct filename
target_link_libraries(read_heavy_compare PRIVATE hashmap_lib)
target_compile_features(read_heavy_compare PRIVATE cxx_std_17)

# 3. Resize Stress Test (with Comparison)
add_executable(resize_stress_compare ${TEST_DIR}/ResizeStressTest_compare.cpp) # Use correct filename
target_link_libraries(resize_stress_compare PRIVATE hashmap_lib)
target_compile_features(resize_stress_compare PRIVATE cxx_std_17)

# 4. Write-Heavy Test (with Comparison)
add_executable(write_heavy ${TEST_DIR}/WriteHeavyTest.cpp) # Use correct filename
target_link_libraries(write_heavy PRIVATE hashmap_lib)
target_compile_features(write_heavy PRIVATE cxx_std_17)

# 5. Benchmark Test (Comparison & Scaling)
add_executable(benchmarks ${TEST_DIR}/BenchmarkTests.cpp) # Use correct filename
target_link_libraries(benchmarks PRIVATE hashmap_lib)
target_compile_features(benchmarks PRIVATE cxx_std_17)


# --- Optional: Installation ---
# Installs executables to 'bin' relative to CMAKE_INSTALL_PREFIX
install(TARGETS 
    compilation
    read_heavy_compare
    resize_stress_compare
    write_heavy
    benchmarks
    DESTINATION bin)

# Installs header files from src/ to 'include' relative to CMAKE_INSTALL_PREFIX
# This assumes your public headers are ThreadPartitionHashMap.h and AbstractHashMap.h
install(FILES 
    ${SRC_DIR}/AbstractHashMap.h
    ${SRC_DIR}/ThreadPartitionHashMap.h 
    DESTINATION include)

# Example: To install *all* .h files from src/
# install(DIRECTORY ${SRC_DIR}/ DESTINATION include FILES_MATCHING PATTERN "*.h")

