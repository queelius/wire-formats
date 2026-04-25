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
