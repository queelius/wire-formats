// rice_golomb.hpp
// Pedagogical implementation for the post "Rice / Golomb" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: Rice<K>, Golomb<M>)

#pragma once

#include <bit>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rice_golomb {

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

// ---- Rice<K> -- parametric code for geometric distributions -----------------
//
// Encodes non-negative integer n >= 0 by splitting into quotient q = n >> K
// and remainder r = n & ((1 << K) - 1):
//   1. Write q zeros (the unary-coded quotient, offset by 1: unary(q+1) minus
//      the final '1' is q zeros, then a '1').
//   2. Write a '1' bit (the unary terminator).
//   3. Write the K-bit binary representation of r, MSB first.
//
// Codeword examples for K=2 (r is always 2 bits):
//   n=0: q=0, r=0 -> "1 00"    (3 bits)
//   n=1: q=0, r=1 -> "1 01"    (3 bits)
//   n=4: q=1, r=0 -> "01 00"   (4 bits)
//   n=5: q=1, r=1 -> "01 01"   (4 bits)
//
// Codeword length: (n >> K) + 1 + K bits.
// Kraft sum: sum_{q=0}^{inf} 2^K * 2^{-(q+1+K)} = 1 (saturates).
// Implied prior: geometric with rate parameter p = 2^K / (2^K + 1), tuned by K.
// Optimal source: geometric distribution with mean mu satisfying K ~ log2(mu).
//
// K must satisfy 0 < K < 64. The template parameter K is the number of
// remainder bits (equivalently, the divisor is 2^K).

template<std::size_t K>
struct Rice {
    using value_type = std::uint64_t;
    static_assert(K > 0 && K < 64, "K must be in [1, 63]");

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        std::uint64_t q = n >> K;
        std::uint64_t r = n & ((std::uint64_t{1} << K) - 1);
        // Write q zero bits (quotient in unary-stop-bit form).
        for (std::uint64_t i = 0; i < q; ++i) sink.write(false);
        // Write the stop bit.
        sink.write(true);
        // Write the K-bit remainder, MSB first.
        for (std::size_t i = 0; i < K; ++i) {
            sink.write(((r >> (K - 1 - i)) & 1) != 0);
        }
    }

    template<BitSource S>
    static value_type decode(S& source) {
        // Count zero bits to get q.
        std::uint64_t q = 0;
        while (!source.read()) ++q;
        // Read K remainder bits, MSB first.
        std::uint64_t r = 0;
        for (std::size_t i = 0; i < K; ++i) {
            r = (r << 1) | (source.read() ? std::uint64_t{1} : std::uint64_t{0});
        }
        return (q << K) | r;
    }
};

}  // namespace rice_golomb
