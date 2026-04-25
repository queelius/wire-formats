#include <gtest/gtest.h>
#include <vector>
#include "succinct_bitvector.hpp"

using namespace succinct_bv;

// Constructing from an empty vector yields size 0.
TEST(SuccinctBVTest, EmptyConstruct) {
    SuccinctBitVector bv({});
    EXPECT_EQ(bv.size(), 0u);
}

// Single-bit vectors.
TEST(SuccinctBVTest, SingleBitTrue) {
    SuccinctBitVector bv({true});
    EXPECT_EQ(bv.size(), 1u);
    EXPECT_EQ(bv.bit(0), true);
}

TEST(SuccinctBVTest, SingleBitFalse) {
    SuccinctBitVector bv({false});
    EXPECT_EQ(bv.size(), 1u);
    EXPECT_EQ(bv.bit(0), false);
}

// 7-bit pattern: 1010101.
TEST(SuccinctBVTest, SevenBitPattern) {
    SuccinctBitVector bv({true, false, true, false, true, false, true});
    EXPECT_EQ(bv.size(), 7u);
    for (std::size_t i = 0; i < 7; ++i) {
        EXPECT_EQ(bv.bit(i), (i % 2 == 0)) << "i=" << i;
    }
}

// Exactly 64 bits (one full word).
TEST(SuccinctBVTest, Exactly64Bits) {
    std::vector<bool> bits(64, false);
    bits[0] = true;
    bits[63] = true;
    SuccinctBitVector bv(bits);
    EXPECT_EQ(bv.size(), 64u);
    EXPECT_TRUE(bv.bit(0));
    EXPECT_TRUE(bv.bit(63));
    EXPECT_FALSE(bv.bit(1));
    EXPECT_FALSE(bv.bit(32));
}

// 100 bits: crosses the first word boundary.
TEST(SuccinctBVTest, CrossWordBoundary) {
    std::vector<bool> bits(100, false);
    bits[63] = true;  // Last bit of word 0.
    bits[64] = true;  // First bit of word 1.
    SuccinctBitVector bv(bits);
    EXPECT_EQ(bv.size(), 100u);
    EXPECT_TRUE(bv.bit(63));
    EXPECT_TRUE(bv.bit(64));
    EXPECT_FALSE(bv.bit(62));
    EXPECT_FALSE(bv.bit(65));
}

// popcount_word uses std::popcount (C++20).
TEST(SuccinctBVTest, PopcountWordZero) {
    EXPECT_EQ(succinct_bv::popcount_word(uint64_t{0}), 0u);
}

TEST(SuccinctBVTest, PopcountWordAllOnes) {
    EXPECT_EQ(succinct_bv::popcount_word(~uint64_t{0}), 64u);
}

TEST(SuccinctBVTest, PopcountWordSingleBit) {
    EXPECT_EQ(succinct_bv::popcount_word(uint64_t{1}), 1u);
    EXPECT_EQ(succinct_bv::popcount_word(uint64_t{1} << 63), 1u);
}

TEST(SuccinctBVTest, PopcountWordKnown) {
    // 0b1010'1010 = 0xAA: 4 set bits.
    EXPECT_EQ(succinct_bv::popcount_word(uint64_t{0xAAAA'AAAA'AAAA'AAAA}), 32u);
    // 0x0F0F...: alternating nibbles, 32 bits set.
    EXPECT_EQ(succinct_bv::popcount_word(uint64_t{0x0F0F'0F0F'0F0F'0F0F}), 32u);
}
