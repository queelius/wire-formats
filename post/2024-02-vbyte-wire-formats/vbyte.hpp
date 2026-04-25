// vbyte.hpp
// Pedagogical implementation for the post "VByte / Varint" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: VByte)
//
// Note: real VByte implementations operate on bytes directly, not individual
// bits. This bit-level implementation is for consistency with the rest of the
// series. See the post's section B aside for an explanation.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace vbyte {

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

// Implementation arrives in Task 10.

}  // namespace vbyte
