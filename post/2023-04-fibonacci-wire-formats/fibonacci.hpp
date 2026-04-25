// fibonacci.hpp
// Pedagogical implementation for the post "Fibonacci Coding" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: Fibonacci)

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fibonacci {

// BitSink and BitSource concepts (minimal local definitions for self-contained
// pedagogical use; production code uses pfc/core.hpp).

template<typename S>
concept BitSink = requires(S& s, bool b) {
    { s.write(b) } -> std::same_as<void>;
};

template<typename S>
concept BitSource = requires(S& s) {
    { s.read() } -> std::same_as<bool>;
};

// ---- to_zeckendorf -- greedy Zeckendorf decomposition ----------------------
//
// Zeckendorf's theorem: every positive integer n has a unique representation
// as a sum of non-consecutive Fibonacci numbers (F_2=1, F_3=2, F_4=3, F_5=5,
// F_6=8, ...). The greedy algorithm finds this representation by subtracting
// the largest Fibonacci number <= n at each step.
//
// Returns a bit vector where bits[i] == true iff F_{i+2} is in the sum.
// Index 0 corresponds to F_2 = 1, index 1 to F_3 = 2, etc.
//
// Example: to_zeckendorf(4) -> {true, false, true}
//          meaning 4 = F_2 + F_4 = 1 + 3.

inline std::vector<bool> to_zeckendorf(std::uint64_t n) {
    assert(n >= 1 && "Zeckendorf is undefined for n = 0");
    // Build the Fibonacci sequence up to n. Start with F_2=1, F_3=2.
    std::vector<std::uint64_t> fibs{1, 2};
    while (fibs.back() <= n) {
        fibs.push_back(fibs[fibs.size() - 1] + fibs[fibs.size() - 2]);
    }
    // The last entry is > n; remove it so all entries are <= n.
    fibs.pop_back();
    // Greedy decomposition: subtract the largest Fibonacci number <= remaining n.
    std::vector<bool> bits(fibs.size(), false);
    for (std::size_t i = fibs.size(); i-- > 0;) {
        if (n >= fibs[i]) {
            bits[i] = true;
            n -= fibs[i];
        }
    }
    return bits;  // bits[i] = true iff fibs[i] is in the Zeckendorf sum
}

}  // namespace fibonacci
