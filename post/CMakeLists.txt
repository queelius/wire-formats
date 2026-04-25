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

# =============================================================================
# Elias Delta and Omega (post 5, 2022-11-13)
# =============================================================================
add_executable(test_elias_delta_omega
    2022-11-elias-delta-omega-wire-formats/test_elias_delta_omega.cpp)
target_link_libraries(test_elias_delta_omega GTest::gtest_main)
target_include_directories(test_elias_delta_omega PRIVATE
    2022-11-elias-delta-omega-wire-formats
    2022-01-priors-wire-formats)
add_test(NAME test_elias_delta_omega COMMAND test_elias_delta_omega)

# =============================================================================
# Fibonacci Coding (post 6, 2023-04-23)
# =============================================================================
add_executable(test_fibonacci
    2023-04-fibonacci-wire-formats/test_fibonacci.cpp)
target_link_libraries(test_fibonacci GTest::gtest_main)
target_include_directories(test_fibonacci PRIVATE
    2023-04-fibonacci-wire-formats
    2022-01-priors-wire-formats)
add_test(NAME test_fibonacci COMMAND test_fibonacci)

# =============================================================================
# Rice / Golomb (post 7, 2023-09-17)
# =============================================================================
add_executable(test_rice_golomb 2023-09-rice-golomb-wire-formats/test_rice_golomb.cpp)
target_link_libraries(test_rice_golomb GTest::gtest_main)
target_include_directories(test_rice_golomb PRIVATE
    2023-09-rice-golomb-wire-formats
    2022-01-priors-wire-formats)
add_test(NAME test_rice_golomb COMMAND test_rice_golomb)

# =============================================================================
# VByte / Varint (post 8, 2024-02-25)
# =============================================================================
add_executable(test_vbyte 2024-02-vbyte-wire-formats/test_vbyte.cpp)
target_link_libraries(test_vbyte GTest::gtest_main)
target_include_directories(test_vbyte PRIVATE
    2024-02-vbyte-wire-formats
    2022-01-priors-wire-formats
    2022-06-elias-gamma-wire-formats
    2022-11-elias-delta-omega-wire-formats)
add_test(NAME test_vbyte COMMAND test_vbyte)

# =============================================================================
# Huffman Coding (post 9, 2024-08-04)
# =============================================================================
add_executable(test_huffman 2024-08-huffman-wire-formats/test_huffman.cpp)
target_link_libraries(test_huffman GTest::gtest_main)
target_include_directories(test_huffman PRIVATE
    2024-08-huffman-wire-formats
    2022-01-priors-wire-formats)
add_test(NAME test_huffman COMMAND test_huffman)

# =============================================================================
# Arithmetic Coding (post 10, 2025-01-12)
# =============================================================================
add_executable(test_arithmetic_coding
    2025-01-arithmetic-coding-wire-formats/test_arithmetic_coding.cpp)
target_link_libraries(test_arithmetic_coding GTest::gtest_main)
target_include_directories(test_arithmetic_coding PRIVATE
    2025-01-arithmetic-coding-wire-formats
    2022-01-priors-wire-formats)
add_test(NAME test_arithmetic_coding COMMAND test_arithmetic_coding)

# =============================================================================
# Succinct Bit Vectors and Rank/Select (post 11, 2025-06-22)
# =============================================================================
add_executable(test_succinct_bv 2025-06-succinct-wire-formats/test_succinct.cpp)
target_link_libraries(test_succinct_bv GTest::gtest_main)
target_include_directories(test_succinct_bv PRIVATE 2025-06-succinct-wire-formats)
add_test(NAME test_succinct_bv COMMAND test_succinct_bv)

# =============================================================================
# RoaringBitmap (post 12, 2025-12-07)
# =============================================================================
add_executable(test_roaring 2025-12-roaring-bitmap-wire-formats/test_roaring.cpp)
target_link_libraries(test_roaring GTest::gtest_main)
target_include_directories(test_roaring PRIVATE 2025-12-roaring-bitmap-wire-formats)
add_test(NAME test_roaring COMMAND test_roaring)
