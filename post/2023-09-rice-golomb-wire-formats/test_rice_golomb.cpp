#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "rice_golomb.hpp"

using namespace rice_golomb;

// Minimal in-memory BitSink/BitSource for tests.
struct BitBuffer {
    std::vector<bool> bits;
    std::size_t pos_ = 0;
    void write(bool b) { bits.push_back(b); }
    bool read() { bool b = bits[pos_]; ++pos_; return b; }
};

// Helper: encode then decode n via Rice<K>, check round-trip.
template<std::size_t K>
static std::uint64_t rice_round_trip(std::uint64_t n) {
    BitBuffer buf;
    Rice<K>::encode(n, buf);
    buf.pos_ = 0;
    return Rice<K>::decode(buf);
}

// Helper: return the bit count emitted for Rice<K>(n).
template<std::size_t K>
static std::size_t rice_bit_count(std::uint64_t n) {
    BitBuffer buf;
    Rice<K>::encode(n, buf);
    return buf.bits.size();
}

// Round-trip: Rice<1>
TEST(RiceGolombTest, Rice1RoundTrip) {
    for (std::uint64_t n = 0; n <= 20; ++n) {
        EXPECT_EQ(rice_round_trip<1>(n), n) << "n=" << n;
    }
}

// Round-trip: Rice<2>
TEST(RiceGolombTest, Rice2RoundTrip) {
    for (std::uint64_t n = 0; n <= 40; ++n) {
        EXPECT_EQ(rice_round_trip<2>(n), n) << "n=" << n;
    }
}

// Round-trip: Rice<4>
TEST(RiceGolombTest, Rice4RoundTrip) {
    for (std::uint64_t n = 0; n <= 100; ++n) {
        EXPECT_EQ(rice_round_trip<4>(n), n) << "n=" << n;
    }
}

// Codeword length for Rice<K>(n): should be floor(n/2^K) + 1 + K bits.
// (floor(n >> K) zero bits + one '1' bit + K remainder bits)
TEST(RiceGolombTest, Rice2BitCount) {
    // k=2: length = (n >> 2) + 1 + 2 = (n >> 2) + 3
    for (std::uint64_t n = 0; n <= 30; ++n) {
        std::size_t expected = static_cast<std::size_t>(n >> 2) + 1 + 2;
        EXPECT_EQ(rice_bit_count<2>(n), expected) << "n=" << n;
    }
}

// Spot-check encoding of n=0 with K=2: q=0, r=0 -> "1 00"
TEST(RiceGolombTest, Rice2Encoding0) {
    BitBuffer buf;
    Rice<2>::encode(std::uint64_t{0}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);  // 1 unary bit + 2 remainder bits
    EXPECT_EQ(buf.bits[0], true);   // unary(0+1) = "1"
    EXPECT_EQ(buf.bits[1], false);  // r=0 bit 1
    EXPECT_EQ(buf.bits[2], false);  // r=0 bit 0
}

// Spot-check encoding of n=4 with K=2: q=1, r=0 -> "0 1 00"
// q=1 zero bit, stop bit '1', 2 remainder bits (r=0). Total: 4 bits.
TEST(RiceGolombTest, Rice2Encoding4) {
    BitBuffer buf;
    Rice<2>::encode(std::uint64_t{4}, buf);
    ASSERT_EQ(buf.bits.size(), 4u);  // 1 zero (q=1) + 1 stop + 2 remainder bits
    EXPECT_EQ(buf.bits[0], false);  // q=1 zero bit
    EXPECT_EQ(buf.bits[1], true);   // stop bit
    EXPECT_EQ(buf.bits[2], false);  // r=0 MSB
    EXPECT_EQ(buf.bits[3], false);  // r=0 LSB
}

// Spot-check encoding of n=5 with K=2: q=1, r=1 -> "0 1 01"
// q=1 zero bit, stop bit '1', 2 remainder bits (r=1). Total: 4 bits.
TEST(RiceGolombTest, Rice2Encoding5) {
    BitBuffer buf;
    Rice<2>::encode(std::uint64_t{5}, buf);
    ASSERT_EQ(buf.bits.size(), 4u);
    EXPECT_EQ(buf.bits[0], false);  // q=1 zero bit
    EXPECT_EQ(buf.bits[1], true);   // stop bit
    EXPECT_EQ(buf.bits[2], false);  // r=1 MSB
    EXPECT_EQ(buf.bits[3], true);   // r=1 LSB
}

// n=0 is valid for Rice (non-negative integers).
TEST(RiceGolombTest, Rice1RoundTripZero) {
    EXPECT_EQ(rice_round_trip<1>(std::uint64_t{0}), std::uint64_t{0});
}

// Large value round-trip.
TEST(RiceGolombTest, Rice4LargeRoundTrip) {
    for (std::uint64_t n : {std::uint64_t{1000}, std::uint64_t{65535}, std::uint64_t{1000000}}) {
        EXPECT_EQ(rice_round_trip<4>(n), n) << "n=" << n;
    }
}

// Helper: encode then decode n via Golomb<M>, check round-trip.
template<std::size_t M>
static std::uint64_t golomb_round_trip(std::uint64_t n) {
    BitBuffer buf;
    Golomb<M>::encode(n, buf);
    buf.pos_ = 0;
    return Golomb<M>::decode(buf);
}

// Helper: return the bit count emitted for Golomb<M>(n).
template<std::size_t M>
static std::size_t golomb_bit_count(std::uint64_t n) {
    BitBuffer buf;
    Golomb<M>::encode(n, buf);
    return buf.bits.size();
}

// Round-trip: Golomb<1> (degenerate: all values have same length - unary)
TEST(RiceGolombTest, Golomb1RoundTrip) {
    for (std::uint64_t n = 0; n <= 10; ++n) {
        EXPECT_EQ(golomb_round_trip<1>(n), n) << "n=" << n;
    }
}

// Round-trip: Golomb<4> (power-of-2: equivalent to Rice<2>)
TEST(RiceGolombTest, Golomb4RoundTrip) {
    for (std::uint64_t n = 0; n <= 40; ++n) {
        EXPECT_EQ(golomb_round_trip<4>(n), n) << "n=" << n;
    }
}

// Round-trip: Golomb<5> (non-power-of-2 m)
TEST(RiceGolombTest, Golomb5RoundTrip) {
    for (std::uint64_t n = 0; n <= 50; ++n) {
        EXPECT_EQ(golomb_round_trip<5>(n), n) << "n=" << n;
    }
}

// Round-trip: Golomb<7> (non-power-of-2 m)
TEST(RiceGolombTest, Golomb7RoundTrip) {
    for (std::uint64_t n = 0; n <= 70; ++n) {
        EXPECT_EQ(golomb_round_trip<7>(n), n) << "n=" << n;
    }
}

// Golomb<4> and Rice<2> agree (4 = 2^2, so K=2 Rice = M=4 Golomb).
TEST(RiceGolombTest, Golomb4EqualsRice2) {
    for (std::uint64_t n = 0; n <= 30; ++n) {
        EXPECT_EQ(golomb_round_trip<4>(n), rice_round_trip<2>(n)) << "n=" << n;
        EXPECT_EQ(golomb_bit_count<4>(n), rice_bit_count<2>(n)) << "n=" << n;
    }
}

// Spot-check Golomb<5> codeword lengths for n=0..9.
// For m=5: bits = ceil(log2(5)) = 3. cutoff = 2^3 - 5 = 3.
// r < 3: use 2 bits. r >= 3: use 3 bits (with r+3 shifted).
// q = floor(n/5), remainder r = n mod 5.
// Length = q + 1 (unary) + (2 if r<3 else 3).
TEST(RiceGolombTest, Golomb5LengthsTable) {
    // n=0: q=0,r=0 -> len=1+2=3
    // n=1: q=0,r=1 -> len=1+2=3
    // n=2: q=0,r=2 -> len=1+2=3
    // n=3: q=0,r=3 -> len=1+3=4
    // n=4: q=0,r=4 -> len=1+3=4
    // n=5: q=1,r=0 -> len=2+2=4
    // n=9: q=1,r=4 -> len=2+3=5
    EXPECT_EQ(golomb_bit_count<5>(0), 3u);
    EXPECT_EQ(golomb_bit_count<5>(1), 3u);
    EXPECT_EQ(golomb_bit_count<5>(2), 3u);
    EXPECT_EQ(golomb_bit_count<5>(3), 4u);
    EXPECT_EQ(golomb_bit_count<5>(4), 4u);
    EXPECT_EQ(golomb_bit_count<5>(5), 4u);
    EXPECT_EQ(golomb_bit_count<5>(9), 5u);
}

// Large-value round-trip for non-power-of-2 M.
TEST(RiceGolombTest, Golomb5LargeRoundTrip) {
    for (std::uint64_t n : {std::uint64_t{100}, std::uint64_t{999}, std::uint64_t{9999}}) {
        EXPECT_EQ(golomb_round_trip<5>(n), n) << "n=" << n;
    }
}
