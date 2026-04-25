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

// Implementation arrives in Tasks 9 and 10.

}  // namespace fibonacci
