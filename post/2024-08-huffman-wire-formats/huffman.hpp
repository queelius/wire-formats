// huffman.hpp
// Pedagogical implementation for the post "Huffman Coding" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (huffman.hpp)
//
// Gotcha: build_huffman_tree uses const_cast on pq.top() to enable moving
// out of a std::priority_queue. std::priority_queue::top() returns const&
// because the queue must not be mutated through the reference (that would
// break the heap invariant). Once we call pq.pop() immediately after, the
// invariant is maintained; the const_cast is safe here. Do not remove it or
// replace the std::move with a copy -- the tree uses std::unique_ptr and
// cannot be copied.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace huffman {

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

// Implementation arrives in Tasks 3 through 5.

}  // namespace huffman
