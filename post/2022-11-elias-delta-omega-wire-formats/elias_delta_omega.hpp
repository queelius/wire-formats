// elias_delta_omega.hpp
// Pedagogical implementation for the post "Elias Delta and Omega" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: EliasDelta, EliasOmega)
//
// Loose-coupling note: Gamma is re-implemented here rather than included
// from unary_gamma.hpp. Each post's header stands alone.

#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stack>
#include <utility>
#include <vector>

namespace elias_delta_omega {

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

// Implementation arrives in Tasks 3, 4, and 5.

}  // namespace elias_delta_omega
