// arithmetic_coding.hpp
// Pedagogical integer range coder for the post "Arithmetic Coding" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (include/pfc/arithmetic_coding.hpp)
//
// Reference: Witten, Neal, Cleary, "Arithmetic Coding for Data Compression,"
// CACM 30(6), 1987.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace arithmetic_coding {

// 32-bit range coder constants.
constexpr std::uint32_t TOP_VALUE      = 0xFFFFFFFFu;
constexpr std::uint32_t HALF           = 0x80000000u;
constexpr std::uint32_t QUARTER        = 0x40000000u;
constexpr std::uint32_t THREE_QUARTER  = 0xC0000000u;

// Forward declarations -- implementations arrive in Tasks 3 through 8.
class BitWriter;
class BitReader;
class ArithmeticEncoder;
class ArithmeticDecoder;

}  // namespace arithmetic_coding
