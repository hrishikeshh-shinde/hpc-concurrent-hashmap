# CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(concurrent_hashmap VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Enable optimizations for benchmarks
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Add benchmark target
add_executable(comparison_test tests/benchmark/comparison_test.cpp)
target_compile_features(comparison_test PRIVATE cxx_std_17)

# Enable multithreading
find_package(Threads REQUIRED)
target_link_libraries(comparison_test PRIVATE Threads::Threads)

# Install targets
install(TARGETS comparison_test DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)