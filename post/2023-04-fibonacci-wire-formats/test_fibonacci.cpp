#include <gtest/gtest.h>
#include <cstdint>
#include <numeric>
#include <vector>
#include "fibonacci.hpp"

using namespace fibonacci;

// ---- Zeckendorf tests -------------------------------------------------------

// to_zeckendorf(n) returns a bit vector where bits[i] == true iff F_{i+2} is
// in the Zeckendorf sum. Index 0 corresponds to F_2 = 1, index 1 to F_3 = 2,
// index 2 to F_4 = 3, etc.

// Helper: reconstruct n from its Zeckendorf bits.
static uint64_t from_zeckendorf(const std::vector<bool>& bits) {
    std::vector<uint64_t> fibs{1, 2};
    while (fibs.size() < bits.size()) {
        fibs.push_back(fibs[fibs.size()-1] + fibs[fibs.size()-2]);
    }
    uint64_t n = 0;
    for (std::size_t i = 0; i < bits.size(); ++i) {
        if (bits[i]) n += fibs[i];
    }
    return n;
}

// Helper: verify no two consecutive bits are true (Zeckendorf uniqueness).
static bool no_consecutive_ones(const std::vector<bool>& bits) {
    for (std::size_t i = 0; i + 1 < bits.size(); ++i) {
        if (bits[i] && bits[i+1]) return false;
    }
    return true;
}

// Spot-check: known Zeckendorf representations.
// 1 = F_2                     -> bits = {1}
// 2 = F_3                     -> bits = {0, 1}
// 3 = F_4                     -> bits = {0, 0, 1}
// 4 = F_4 + F_2 = 3+1         -> bits = {1, 0, 1}
// 10 = F_6 + F_3 = 8+2        -> bits = {0, 1, 0, 0, 1}
// 11 = F_6 + F_4 = 8+3        -> bits = {0, 0, 1, 0, 1}
TEST(FibonacciTest, ZeckendorfSpotCheck1) {
    auto b = to_zeckendorf(1u);
    ASSERT_GE(b.size(), 1u);
    EXPECT_EQ(b[0], true);
    EXPECT_EQ(from_zeckendorf(b), 1u);
}

TEST(FibonacciTest, ZeckendorfSpotCheck4) {
    auto b = to_zeckendorf(4u);
    // 4 = 3 + 1 = F_4 + F_2: bits[0]=1 (F_2), bits[1]=0 (F_3), bits[2]=1 (F_4).
    ASSERT_GE(b.size(), 3u);
    EXPECT_EQ(b[0], true);
    EXPECT_EQ(b[1], false);
    EXPECT_EQ(b[2], true);
    EXPECT_EQ(from_zeckendorf(b), 4u);
}

TEST(FibonacciTest, ZeckendorfSpotCheck10) {
    auto b = to_zeckendorf(10u);
    EXPECT_EQ(from_zeckendorf(b), 10u);
    EXPECT_TRUE(no_consecutive_ones(b));
}

// Round-trip: for all n in 1..200, from_zeckendorf(to_zeckendorf(n)) == n.
TEST(FibonacciTest, ZeckendorfRoundTrip) {
    for (uint64_t n = 1; n <= 200; ++n) {
        auto b = to_zeckendorf(n);
        EXPECT_EQ(from_zeckendorf(b), n) << "n=" << n;
    }
}

// Non-consecutive: Zeckendorf bits never have two adjacent 1s.
TEST(FibonacciTest, ZeckendorfNoConsecutiveOnes) {
    for (uint64_t n = 1; n <= 200; ++n) {
        EXPECT_TRUE(no_consecutive_ones(to_zeckendorf(n))) << "n=" << n;
    }
}

// Uniqueness: to_zeckendorf should always return the same result for the same n.
TEST(FibonacciTest, ZeckendorfDeterministic) {
    for (uint64_t n = 1; n <= 50; ++n) {
        EXPECT_EQ(to_zeckendorf(n), to_zeckendorf(n)) << "n=" << n;
    }
}

// ---- Fibonacci codec tests --------------------------------------------------

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

static uint64_t fib_round_trip(uint64_t n) {
    BitBuffer buf;
    Fibonacci::encode(n, buf);
    buf.pos_ = 0;
    return Fibonacci::decode(buf);
}

static std::size_t fib_bit_count(uint64_t n) {
    BitBuffer buf;
    Fibonacci::encode(n, buf);
    return buf.bits.size();
}

TEST(FibonacciTest, FibonacciRoundTrip) {
    for (uint64_t n = 1; n <= 200; ++n) {
        EXPECT_EQ(fib_round_trip(n), n) << "n=" << n;
    }
}

TEST(FibonacciTest, FibonacciRoundTripLarge) {
    for (uint64_t n : {uint64_t{1000}, uint64_t{10000}, uint64_t{100000}}) {
        EXPECT_EQ(fib_round_trip(n), n) << "n=" << n;
    }
}

// Spot-check known codewords from the spec:
// 1 -> "11" (F_2 bit + terminator)
// 2 -> "011" (F_3 bit + terminator: bits={0,1}, append 1)
// 3 -> "0011" (F_4: bits={0,0,1}, append 1)
// 4 -> "1011" (F_2+F_4: bits={1,0,1}, append 1)
// 8 -> "000011" (F_6: bits={0,0,0,0,1}, append 1)

TEST(FibonacciTest, FibonacciEncoding1Is11) {
    BitBuffer buf;
    Fibonacci::encode(uint64_t{1}, buf);
    ASSERT_EQ(buf.bits.size(), 2u);
    EXPECT_EQ(buf.bits[0], true);   // F_2 bit
    EXPECT_EQ(buf.bits[1], true);   // terminator
}

TEST(FibonacciTest, FibonacciEncoding2Is011) {
    BitBuffer buf;
    Fibonacci::encode(uint64_t{2}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);
    EXPECT_EQ(buf.bits[0], false);  // F_2 bit = 0
    EXPECT_EQ(buf.bits[1], true);   // F_3 bit = 1
    EXPECT_EQ(buf.bits[2], true);   // terminator
}

TEST(FibonacciTest, FibonacciEncoding4Is1011) {
    BitBuffer buf;
    Fibonacci::encode(uint64_t{4}, buf);
    ASSERT_EQ(buf.bits.size(), 4u);
    EXPECT_EQ(buf.bits[0], true);   // F_2=1 bit
    EXPECT_EQ(buf.bits[1], false);  // F_3=2 bit
    EXPECT_EQ(buf.bits[2], true);   // F_4=3 bit
    EXPECT_EQ(buf.bits[3], true);   // terminator
}

// Every codeword ends in "11" (Zeckendorf bits followed by terminator '1').
// The last Zeckendorf bit is always 1 (it is the highest Fibonacci in the sum).
TEST(FibonacciTest, AllCodewordsEndIn11) {
    for (uint64_t n = 1; n <= 100; ++n) {
        BitBuffer buf;
        Fibonacci::encode(n, buf);
        std::size_t len = buf.bits.size();
        ASSERT_GE(len, 2u) << "n=" << n;
        // The last two bits must both be 1.
        EXPECT_EQ(buf.bits[len - 1], true) << "terminator missing for n=" << n;
        EXPECT_EQ(buf.bits[len - 2], true) << "last Zeckendorf bit not 1 for n=" << n;
    }
}

// No codeword contains "11" except at the very end.
TEST(FibonacciTest, NoInternalConsecutiveOnes) {
    for (uint64_t n = 1; n <= 100; ++n) {
        BitBuffer buf;
        Fibonacci::encode(n, buf);
        std::size_t len = buf.bits.size();
        // Check all pairs except the final pair (which is the "11" terminator).
        for (std::size_t i = 0; i + 2 < len; ++i) {
            EXPECT_FALSE(buf.bits[i] && buf.bits[i+1])
                << "Internal '11' at position " << i << " for n=" << n;
        }
    }
}
