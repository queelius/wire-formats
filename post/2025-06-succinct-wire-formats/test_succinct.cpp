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

// rank1(i) counts set bits in [0, i) -- i.e., strictly before position i.
// rank1(0) is always 0 (no bits before position 0).
TEST(SuccinctBVTest, Rank1AtZero) {
    SuccinctBitVector bv({true, false, true});
    EXPECT_EQ(bv.rank1(0), 0u);
}

// All-zeros: rank1 is always 0.
TEST(SuccinctBVTest, Rank1AllZeros) {
    std::vector<bool> bits(200, false);
    SuccinctBitVector bv(bits);
    for (std::size_t i = 0; i <= 200; ++i) {
        EXPECT_EQ(bv.rank1(i), 0u) << "i=" << i;
    }
}

// All-ones: rank1(i) == i.
TEST(SuccinctBVTest, Rank1AllOnes) {
    std::vector<bool> bits(200, true);
    SuccinctBitVector bv(bits);
    for (std::size_t i = 0; i <= 200; ++i) {
        EXPECT_EQ(bv.rank1(i), i) << "i=" << i;
    }
}

// Known pattern: 1,0,1,0,1,0,1 -> rank1(k) = ceil(k/2).
TEST(SuccinctBVTest, Rank1Alternating) {
    SuccinctBitVector bv({true, false, true, false, true, false, true});
    EXPECT_EQ(bv.rank1(0), 0u);
    EXPECT_EQ(bv.rank1(1), 1u);
    EXPECT_EQ(bv.rank1(2), 1u);
    EXPECT_EQ(bv.rank1(3), 2u);
    EXPECT_EQ(bv.rank1(4), 2u);
    EXPECT_EQ(bv.rank1(5), 3u);
    EXPECT_EQ(bv.rank1(6), 3u);
    EXPECT_EQ(bv.rank1(7), 4u);
}

// rank1 across a word boundary (tests that word 0 and word 1 both contribute).
TEST(SuccinctBVTest, Rank1CrossWordBoundary) {
    std::vector<bool> bits(128, false);
    bits[60] = true;
    bits[65] = true;
    bits[127] = true;
    SuccinctBitVector bv(bits);
    EXPECT_EQ(bv.rank1(60), 0u);
    EXPECT_EQ(bv.rank1(61), 1u);
    EXPECT_EQ(bv.rank1(65), 1u);
    EXPECT_EQ(bv.rank1(66), 2u);
    EXPECT_EQ(bv.rank1(128), 3u);
}

// Verify superblock_rank_at(sb) matches the naive rank at the superblock start.
// Accessor exposed for testing only.
TEST(SuccinctBVTest, SuperblockRankMatchesNaive) {
    // Build a bit vector that spans several superblocks.
    const std::size_t N = 4096 * 3 + 100;  // 3 full superblocks + 100 extra bits.
    std::vector<bool> bits(N, false);
    // Set every 7th bit so the pattern is non-trivial.
    for (std::size_t i = 0; i < N; i += 7) bits[i] = true;
    SuccinctBitVector bv(bits);
    // Compare superblock entry to naive scan result at each superblock boundary.
    for (std::size_t sb = 0; sb < 4; ++sb) {
        std::size_t pos = sb * 4096;
        if (pos > N) break;
        EXPECT_EQ(bv.superblock_rank_at(sb), bv.rank1_naive(pos))
            << "superblock=" << sb;
    }
}

// Verify block_rank_at(block) matches naive scan from the superblock start.
TEST(SuccinctBVTest, BlockRankMatchesNaive) {
    const std::size_t N = 4096 + 512;  // One full superblock + a bit more.
    std::vector<bool> bits(N, false);
    for (std::size_t i = 0; i < N; i += 3) bits[i] = true;
    SuccinctBitVector bv(bits);
    // Check blocks 0..64 (within superblock 0).
    for (std::size_t blk = 0; blk < 64; ++blk) {
        std::size_t pos = blk * 64;  // Block start position.
        // Block rank is relative to superblock 0, which starts at 0.
        EXPECT_EQ(bv.block_rank_at(blk), bv.rank1_naive(pos))
            << "block=" << blk;
    }
}
