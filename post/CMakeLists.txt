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

# =============================================================================
# McMillan's Converse (post 2, 2020-09-13)
# =============================================================================
add_executable(test_mcmillan 2020-09-mcmillan-wire-formats/test_mcmillan.cpp)
target_link_libraries(test_mcmillan GTest::gtest_main)
target_include_directories(test_mcmillan PRIVATE 2020-09-mcmillan-wire-formats)
add_test(NAME test_mcmillan COMMAND test_mcmillan)

# =============================================================================
# Universal Codes as Priors (post 3, 2022-01-15)
# =============================================================================
add_executable(test_priors 2022-01-priors-wire-formats/test_priors.cpp)
target_link_libraries(test_priors GTest::gtest_main)
target_include_directories(test_priors PRIVATE 2022-01-priors-wire-formats)
add_test(NAME test_priors COMMAND test_priors)

# =============================================================================
# Unary and Elias Gamma (post 4, 2022-06-19)
# =============================================================================
add_executable(test_unary_gamma 2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp)
target_link_libraries(test_unary_gamma GTest::gtest_main)
target_include_directories(test_unary_gamma PRIVATE
    2022-06-elias-gamma-wire-formats
    2022-01-priors-wire-formats)
add_test(NAME test_unary_gamma COMMAND test_unary_gamma)
