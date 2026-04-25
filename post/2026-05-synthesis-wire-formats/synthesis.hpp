// synthesis.hpp
// Pedagogical implementation for the post "Synthesis: Codecs as Structure" in the
// "Algebra over Wire Formats" series. For the production library, see PFC:
// https://github.com/queelius/pfc

#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace synthesis {

// ---- empirical_distribution -- estimate distribution from a sample ----------
//
// Counts occurrences of each value in the sample, then normalizes by the
// total count to produce a probability distribution.
//
// Returns a map from value to estimated probability. All probabilities are
// positive (zero-count values are not included) and sum to 1.

inline std::map<std::uint64_t, double>
empirical_distribution(const std::vector<std::uint64_t>& sample)
{
    assert(!sample.empty() && "Cannot estimate distribution from empty sample");
    std::map<std::uint64_t, std::size_t> counts;
    for (std::uint64_t v : sample) ++counts[v];
    std::map<std::uint64_t, double> dist;
    double total = static_cast<double>(sample.size());
    for (const auto& [v, c] : counts) {
        dist[v] = static_cast<double>(c) / total;
    }
    return dist;
}

}  // namespace synthesis
