#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include <vector>
#include "unary_gamma.hpp"

using namespace unary_gamma;

// Minimal in-memory BitSink for tests.
struct BitBuffer {
    std::vector<bool> bits;
    void write(bool b) { bits.push_back(b); }
    bool read() {
        bool b = bits[pos_]; ++pos_; return b;
    }
    std::size_t pos_ = 0;
};

// Helper: encode then decode n, check round-trip.
static uint64_t unary_round_trip(uint64_t n) {
    BitBuffer buf;
    Unary::encode(n, buf);
    buf.pos_ = 0;
    return Unary::decode(buf);
}

// Helper: return the bit count emitted for n.
static std::size_t unary_bit_count(uint64_t n) {
    BitBuffer buf;
    Unary::encode(n, buf);
    return buf.bits.size();
}

TEST(UnaryGammaTest, UnaryRoundTrip1) {
    EXPECT_EQ(unary_round_trip(1), 1u);
}

TEST(UnaryGammaTest, UnaryRoundTrip2) {
    EXPECT_EQ(unary_round_trip(2), 2u);
}

TEST(UnaryGammaTest, UnaryRoundTripLarge) {
    for (uint64_t n = 1; n <= 20; ++n) {
        EXPECT_EQ(unary_round_trip(n), n) << "n=" << n;
    }
}

// Codeword for n has exactly n bits.
TEST(UnaryGammaTest, UnaryBitCount) {
    for (uint64_t n = 1; n <= 20; ++n) {
        EXPECT_EQ(unary_bit_count(n), n) << "n=" << n;
    }
}

// Codeword for n=1 is a single '1' bit.
TEST(UnaryGammaTest, UnaryEncoding1IsSingleOne) {
    BitBuffer buf;
    Unary::encode(uint64_t{1}, buf);
    ASSERT_EQ(buf.bits.size(), 1u);
    EXPECT_EQ(buf.bits[0], true);
}

// Codeword for n=3 is "001": two zeros then a one.
TEST(UnaryGammaTest, UnaryEncoding3IsTwoZerosOneOne) {
    BitBuffer buf;
    Unary::encode(uint64_t{3}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], false);
    EXPECT_EQ(buf.bits[2], true);
}

// Helper: encode then decode n via Gamma, check round-trip.
static uint64_t gamma_round_trip(uint64_t n) {
    BitBuffer buf;
    Gamma::encode(n, buf);
    buf.pos_ = 0;
    return Gamma::decode(buf);
}

// Helper: return bit count for Gamma(n).
static std::size_t gamma_bit_count(uint64_t n) {
    BitBuffer buf;
    Gamma::encode(n, buf);
    return buf.bits.size();
}

TEST(UnaryGammaTest, GammaRoundTrip) {
    for (uint64_t n = 1; n <= 256; ++n) {
        EXPECT_EQ(gamma_round_trip(n), n) << "n=" << n;
    }
}

// Length of Gamma(n) = 2*floor(log2(n)) + 1.
TEST(UnaryGammaTest, GammaBitCount) {
    for (uint64_t n = 1; n <= 128; ++n) {
        std::size_t k = 0;
        uint64_t tmp = n;
        while (tmp > 1) { tmp >>= 1; ++k; }
        std::size_t expected = 2 * k + 1;
        EXPECT_EQ(gamma_bit_count(n), expected) << "n=" << n;
    }
}

// Spot-check specific encodings from the spec.
// 1 -> "1", 2 -> "010", 3 -> "011", 4 -> "00100", 8 -> "0001000"
TEST(UnaryGammaTest, GammaEncoding1IsSingleOne) {
    BitBuffer buf;
    Gamma::encode(uint64_t{1}, buf);
    ASSERT_EQ(buf.bits.size(), 1u);
    EXPECT_EQ(buf.bits[0], true);
}

TEST(UnaryGammaTest, GammaEncoding2Is010) {
    BitBuffer buf;
    Gamma::encode(uint64_t{2}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], false);
}

TEST(UnaryGammaTest, GammaEncoding3Is011) {
    BitBuffer buf;
    Gamma::encode(uint64_t{3}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], true);
}

TEST(UnaryGammaTest, GammaEncoding4Is00100) {
    BitBuffer buf;
    Gamma::encode(uint64_t{4}, buf);
    ASSERT_EQ(buf.bits.size(), 5u);
    // "00100": leading zeros count = 2, then the 3-bit binary for 4
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], false);
    EXPECT_EQ(buf.bits[2], true);
    EXPECT_EQ(buf.bits[3], false);
    EXPECT_EQ(buf.bits[4], false);
}
