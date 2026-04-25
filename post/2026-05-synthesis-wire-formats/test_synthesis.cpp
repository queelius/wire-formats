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
