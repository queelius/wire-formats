// priors.hpp
// Pedagogical implementation for the post "Universal Codes as Priors" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc

#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <vector>

namespace priors {

// ---- implied_prior -- the core correspondence: lengths -> probabilities ------
//
// For a prefix-free code with codeword lengths (l_1, ..., l_n), the implicit
// probability of symbol i is 2^{-l_i}. If the lengths do not saturate Kraft
// (sum < 1), normalize so the probabilities sum to 1.
//
// This is the inverse of Shannon's prescription: given p_i, the optimal length
// is -log2(p_i). Going backward: given a length l_i, the implied probability is
// 2^{-l_i}.

inline std::vector<double> implied_prior(const std::vector<std::size_t>& lengths) {
    std::vector<double> probs;
    probs.reserve(lengths.size());
    double total = 0.0;
    for (std::size_t l : lengths) {
        double p = std::ldexp(1.0, -static_cast<int>(l));
        probs.push_back(p);
        total += p;
    }
    // Normalize if Kraft sum is less than 1.
    if (total < 1.0) {
        for (double& p : probs) p /= total;
    }
    return probs;
}

}  // namespace priors
