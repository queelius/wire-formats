#include <gtest/gtest.h>
#include <bit>
#include <cstdint>
#include <vector>
#include "elias_delta_omega.hpp"

using namespace elias_delta_omega;

// Minimal in-memory BitSink/BitSource for tests.
struct BitBuffer {
    std::vector<bool> bits;
    void write(bool b) { bits.push_back(b); }
    bool read() {
        bool b = bits[pos_];
        ++pos_;
        return b;
    }
    std::size_t pos_ = 0;
};

// ---- Gamma local re-implementation tests ------------------------------------

static uint64_t gamma_round_trip(uint64_t n) {
    BitBuffer buf;
    Gamma::encode(n, buf);
    buf.pos_ = 0;
    return Gamma::decode(buf);
}

static std::size_t gamma_bit_count(uint64_t n) {
    BitBuffer buf;
    Gamma::encode(n, buf);
    return buf.bits.size();
}

TEST(EliasDeltaOmegaTest, GammaLocalRoundTrip) {
    for (uint64_t n = 1; n <= 256; ++n) {
        EXPECT_EQ(gamma_round_trip(n), n) << "n=" << n;
    }
}

// Gamma length = 2*floor(log2(n)) + 1.
TEST(EliasDeltaOmegaTest, GammaLocalBitCount) {
    for (uint64_t n = 1; n <= 64; ++n) {
        std::size_t k = std::bit_width(n) - 1;
        EXPECT_EQ(gamma_bit_count(n), 2 * k + 1) << "n=" << n;
    }
}

// ---- Delta tests ------------------------------------------------------------

static uint64_t delta_round_trip(uint64_t n) {
    BitBuffer buf;
    Delta::encode(n, buf);
    buf.pos_ = 0;
    return Delta::decode(buf);
}

static std::size_t delta_bit_count(uint64_t n) {
    BitBuffer buf;
    Delta::encode(n, buf);
    return buf.bits.size();
}

TEST(EliasDeltaOmegaTest, DeltaRoundTrip) {
    for (uint64_t n = 1; n <= 256; ++n) {
        EXPECT_EQ(delta_round_trip(n), n) << "n=" << n;
    }
}

TEST(EliasDeltaOmegaTest, DeltaRoundTripLarge) {
    for (uint64_t n : {uint64_t{1000}, uint64_t{65536}, uint64_t{1000000}}) {
        EXPECT_EQ(delta_round_trip(n), n) << "n=" << n;
    }
}

// Spot-check specific encodings from the spec:
// 1 -> gamma(1) = "1" (L=1, no trailing bits after the leading 1)
TEST(EliasDeltaOmegaTest, DeltaEncoding1IsSingleOne) {
    BitBuffer buf;
    Delta::encode(uint64_t{1}, buf);
    ASSERT_EQ(buf.bits.size(), 1u);
    EXPECT_EQ(buf.bits[0], true);
}

// 2 -> gamma(2)."0" = "010"."0" = "0100" (4 bits)
TEST(EliasDeltaOmegaTest, DeltaEncoding2Is0100) {
    BitBuffer buf;
    Delta::encode(uint64_t{2}, buf);
    ASSERT_EQ(buf.bits.size(), 4u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], false);
    EXPECT_EQ(buf.bits[3], false);
}

// 3 -> gamma(2)."1" = "010"."1" = "0101" (4 bits)
TEST(EliasDeltaOmegaTest, DeltaEncoding3Is0101) {
    BitBuffer buf;
    Delta::encode(uint64_t{3}, buf);
    ASSERT_EQ(buf.bits.size(), 4u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], false);
    EXPECT_EQ(buf.bits[3], true);
}

// 4 -> gamma(3)."00" = "011"."00" = "01100" (5 bits)
TEST(EliasDeltaOmegaTest, DeltaEncoding4Is01100) {
    BitBuffer buf;
    Delta::encode(uint64_t{4}, buf);
    ASSERT_EQ(buf.bits.size(), 5u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], true);
    EXPECT_EQ(buf.bits[3], false);
    EXPECT_EQ(buf.bits[4], false);
}

// Delta length is always <= Gamma length for n >= 16 (crossover claim from spec).
TEST(EliasDeltaOmegaTest, DeltaShorterThanGammaForN16AndAbove) {
    for (uint64_t n = 16; n <= 1024; ++n) {
        std::size_t dl = delta_bit_count(n);
        std::size_t gl = gamma_bit_count(n);
        EXPECT_LE(dl, gl) << "n=" << n << " delta=" << dl << " gamma=" << gl;
    }
}
