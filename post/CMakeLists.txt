cmake_minimum_required(VERSION 3.16)
project(wire-formats LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# =============================================================================
# Per-post test executables are added below as posts are written.
# =============================================================================

# =============================================================================
# Kraft's Inequality (post 1, 2020-03-22)
# =============================================================================
add_executable(test_kraft 2020-03-kraft-wire-formats/test_kraft.cpp)
target_link_libraries(test_kraft GTest::gtest_main)
target_include_directories(test_kraft PRIVATE 2020-03-kraft-wire-formats)
add_test(NAME test_kraft COMMAND test_kraft)
