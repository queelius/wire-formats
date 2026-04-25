// unary_gamma.hpp
// Pedagogical implementation for the post "Unary and Elias Gamma" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: Unary, EliasGamma)

#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace unary_gamma {

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

// ---- Unary -- the simplest universal code -----------------------------------
//
// Encodes positive integer n >= 1 as (n-1) zero bits followed by one '1' bit.
// Examples: 1 -> "1", 2 -> "01", 3 -> "001", 4 -> "0001".
//
// Length of codeword for n: n bits.
// Kraft sum: sum_{n=1}^{inf} 2^{-n} = 1 (saturates).
// Implied prior: p_n = 2^{-n} (geometric distribution with parameter 1/2).
// Optimal source: geometric(1/2), i.e., each value is half as likely as the
//                 previous. Unary achieves entropy exactly on this prior.

struct Unary {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1 && "Unary is undefined for n = 0");
        for (value_type i = 1; i < n; ++i) sink.write(false);
        sink.write(true);
    }

    template<BitSource S>
    static value_type decode(S& source) {
        value_type n = 1;
        while (!source.read()) ++n;
        return n;
    }
};

}  // namespace unary_gamma
