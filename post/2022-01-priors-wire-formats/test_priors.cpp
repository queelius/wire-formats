#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "priors.hpp"

using namespace priors;

// Helper: check that two double vectors are element-wise close.
static void expect_close(const std::vector<double>& a,
                         const std::vector<double>& b,
                         double tol = 1e-9) {
    ASSERT_EQ(a.size(), b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        EXPECT_NEAR(a[i], b[i], tol) << "index " << i;
    }
}

// Unary lengths for symbols 1..K: l_i = i.
static std::vector<std::size_t> unary_lengths(std::size_t K) {
    std::vector<std::size_t> v(K);
    for (std::size_t i = 0; i < K; ++i) v[i] = i + 1;
    return v;
}

// Gamma lengths for symbols 1..N: l_n = 2*floor(log2(n)) + 1.
static std::vector<std::size_t> gamma_lengths(std::size_t N) {
    std::vector<std::size_t> v(N);
    for (std::size_t i = 0; i < N; ++i) {
        std::size_t n = i + 1;
        std::size_t k = 0;
        std::size_t tmp = n;
        while (tmp > 1) { tmp >>= 1; ++k; }
        v[i] = 2 * k + 1;
    }
    return v;
}

// Unary implied prior should be geometric(1/2): p_i = 2^{-i} for i = 1..K,
// normalized because the truncated sum < 1.
TEST(PriorsTest, ImpliedPriorUnaryIsGeometricHalf) {
    // For unary lengths {1, 2, 3, 4}, Kraft sum = 1/2 + 1/4 + 1/8 + 1/16 = 15/16.
    // Normalization: p_i = (2^{-i}) / (15/16) = 16/(15 * 2^i).
    auto lengths = unary_lengths(4);
    auto probs = implied_prior(lengths);
    ASSERT_EQ(probs.size(), 4u);
    double total = 0.0;
    for (double p : probs) total += p;
    EXPECT_NEAR(total, 1.0, 1e-12);
    // Each successive probability should be half the previous (geometric ratio).
    for (std::size_t i = 1; i < probs.size(); ++i) {
        EXPECT_NEAR(probs[i], probs[i - 1] / 2.0, 1e-12) << "index " << i;
    }
}

// For a Kraft-saturating code (e.g., fixed-width 2-bit: lengths {2,2,2,2}),
// implied prior should be uniform: each prob = 0.25.
TEST(PriorsTest, ImpliedPriorSaturatingCodeIsUniform) {
    std::vector<std::size_t> lengths = {2, 2, 2, 2};
    auto probs = implied_prior(lengths);
    ASSERT_EQ(probs.size(), 4u);
    for (double p : probs) {
        EXPECT_NEAR(p, 0.25, 1e-12);
    }
}

// Gamma implied prior: for symbols 1..N, should be approximately p_n ~ 1/n^2.
// We verify the ratio p_1 / p_n is approximately n^2 (with a factor-of-2
// deviation for non-power-of-2 n values, which is expected from gamma's
// block structure).
TEST(PriorsTest, ImpliedPriorGammaApproxPowerLaw2) {
    auto lengths = gamma_lengths(16);
    auto probs = implied_prior(lengths);
    ASSERT_EQ(probs.size(), 16u);
    // p_1 is the reference. Each p_n should be within a factor of 4 of p_1/n^2.
    double p1 = probs[0];
    for (std::size_t i = 1; i < probs.size(); ++i) {
        double n = static_cast<double>(i + 1);
        double expected = p1 / (n * n);
        // Allow factor-of-4 deviation (gamma assigns same length to whole block).
        EXPECT_GT(probs[i], expected / 4.0) << "n=" << n;
        EXPECT_LT(probs[i], expected * 4.0) << "n=" << n;
    }
}

// Probabilities must sum to 1.
TEST(PriorsTest, ImpliedPriorSumsToOne) {
    auto lengths = gamma_lengths(32);
    auto probs = implied_prior(lengths);
    double total = 0.0;
    for (double p : probs) total += p;
    EXPECT_NEAR(total, 1.0, 1e-9);
}
