#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>
#include "synthesis.hpp"

using namespace synthesis;

// ---- empirical_distribution tests ------------------------------------------

// Single-value sample: distribution is a single point mass.
TEST(SynthesisTest, EmpiricalDistributionSingleValue) {
    std::vector<std::uint64_t> sample = {5, 5, 5};
    auto dist = empirical_distribution(sample);
    ASSERT_EQ(dist.size(), 1u);
    EXPECT_NEAR(dist.at(5), 1.0, 1e-12);
}

// Two-value sample, equal counts: both have probability 0.5.
TEST(SynthesisTest, EmpiricalDistributionTwoEqualValues) {
    std::vector<std::uint64_t> sample = {1, 2, 1, 2};
    auto dist = empirical_distribution(sample);
    ASSERT_EQ(dist.size(), 2u);
    EXPECT_NEAR(dist.at(1), 0.5, 1e-12);
    EXPECT_NEAR(dist.at(2), 0.5, 1e-12);
}

// Probabilities must sum to 1.
TEST(SynthesisTest, EmpiricalDistributionSumsToOne) {
    std::vector<std::uint64_t> sample = {1, 2, 3, 1, 2, 1};
    auto dist = empirical_distribution(sample);
    double total = 0.0;
    for (const auto& [v, p] : dist) total += p;
    EXPECT_NEAR(total, 1.0, 1e-12);
}

// Known frequencies: {1: 3 times, 2: 1 time} -> {1: 0.75, 2: 0.25}.
TEST(SynthesisTest, EmpiricalDistributionKnownFrequencies) {
    std::vector<std::uint64_t> sample = {1, 1, 1, 2};
    auto dist = empirical_distribution(sample);
    ASSERT_EQ(dist.size(), 2u);
    EXPECT_NEAR(dist.at(1), 0.75, 1e-12);
    EXPECT_NEAR(dist.at(2), 0.25, 1e-12);
}

// ---- entropy_of tests -------------------------------------------------------

// Entropy of a uniform distribution over K symbols = log2(K).
TEST(SynthesisTest, EntropyOfUniform) {
    std::map<std::uint64_t, double> dist = {{1, 0.25}, {2, 0.25}, {3, 0.25}, {4, 0.25}};
    EXPECT_NEAR(entropy_of(dist), 2.0, 1e-12);
}

// Entropy of a degenerate distribution (one symbol certain) = 0.
TEST(SynthesisTest, EntropyOfDegenerate) {
    std::map<std::uint64_t, double> dist = {{7, 1.0}};
    EXPECT_NEAR(entropy_of(dist), 0.0, 1e-12);
}

// Entropy of {0.5, 0.5} = 1.
TEST(SynthesisTest, EntropyOfBinaryHalf) {
    std::map<std::uint64_t, double> dist = {{1, 0.5}, {2, 0.5}};
    EXPECT_NEAR(entropy_of(dist), 1.0, 1e-12);
}

// ---- length_for tests -------------------------------------------------------

// Unary: length of n is n bits (for positive integers).
TEST(SynthesisTest, LengthForUnary) {
    for (std::uint64_t n = 1; n <= 10; ++n) {
        EXPECT_EQ(length_for("Unary", n), n) << "n=" << n;
    }
}

// Gamma: length = 2*floor(log2(n)) + 1.
TEST(SynthesisTest, LengthForGamma) {
    // n=1 -> 1, n=2 -> 3, n=3 -> 3, n=4 -> 5, n=8 -> 7, n=16 -> 9
    EXPECT_EQ(length_for("Gamma", 1), 1u);
    EXPECT_EQ(length_for("Gamma", 2), 3u);
    EXPECT_EQ(length_for("Gamma", 3), 3u);
    EXPECT_EQ(length_for("Gamma", 4), 5u);
    EXPECT_EQ(length_for("Gamma", 8), 7u);
    EXPECT_EQ(length_for("Gamma", 16), 9u);
}

// Delta: total = gamma_len(floor(log2(n))+1) + floor(log2(n)).
// Standard Elias delta formula: L=floor(log2(n))+1, encode L in gamma, then k trailing bits.
// Known values: n=1->1, n=2->4, n=3->4, n=4->5, n=16->9, n=256->15.
TEST(SynthesisTest, LengthForDelta) {
    EXPECT_EQ(length_for("Delta", 1), 1u);
    EXPECT_EQ(length_for("Delta", 2), 4u);
    EXPECT_EQ(length_for("Delta", 3), 4u);
    EXPECT_EQ(length_for("Delta", 4), 5u);
    EXPECT_EQ(length_for("Delta", 16), 9u);
    EXPECT_EQ(length_for("Delta", 256), 15u);
}

// VByte: length = 8 * ceil(ceil(log2(n+1)) / 7), or 8 for n in [0,127].
TEST(SynthesisTest, LengthForVByte) {
    EXPECT_EQ(length_for("VByte", 0),    8u);   // 1 byte
    EXPECT_EQ(length_for("VByte", 1),    8u);   // 1 byte
    EXPECT_EQ(length_for("VByte", 127),  8u);   // 1 byte
    EXPECT_EQ(length_for("VByte", 128),  16u);  // 2 bytes
    EXPECT_EQ(length_for("VByte", 16383), 16u); // 2 bytes
    EXPECT_EQ(length_for("VByte", 16384), 24u); // 3 bytes
}
