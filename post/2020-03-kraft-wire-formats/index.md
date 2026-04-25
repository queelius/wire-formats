---
title: "Kraft's Inequality"
date: 2020-03-22
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- combinatorics
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 1
math: true
description: "Which codeword-length vectors are achievable by prefix-free codes? Kraft's inequality is the answer."
linked_project:
- pfc
- wire-formats
---
*Every prefix-free code satisfies one inequality. That inequality is also sufficient. This post develops the necessary direction.*

## Kraft's Inequality

I want a code where each symbol maps to a bit string, and where any concatenation of codewords can be decoded unambiguously. The simplest way to guarantee that is prefix-freeness: no codeword is a prefix of any other. A prefix-free code is self-delimiting. The decoder reads bits left-to-right and knows exactly when each codeword ends, with no lookahead and no length headers.

The question I keep returning to is: which collections of lengths are actually achievable? If I want four codewords of lengths 1, 2, 3, and 3, can I build a prefix-free code with those lengths? What if I want two codewords of length 1? (No: there are only two 1-bit strings, and they are prefixes of everything longer.)

Kraft's inequality is the answer. A length vector \((l_1, l_2, \ldots, l_n)\) is achievable by a prefix-free binary code only if

$$\sum_{i=1}^{n} 2^{-l_i} \leq 1.$$

This is the constraint you cannot escape. Any prefix-free code satisfies it. Any length vector that violates it cannot be realized as a prefix-free code, full stop.

The converse is also true: any length vector satisfying Kraft is realizable by some prefix-free code. That is McMillan's theorem, and it is the subject of the [next post in this series](/post/2020-09-mcmillan-wire-formats/). This post develops the necessary direction: every prefix-free code satisfies Kraft.

The right tool for understanding why is the binary tree.

## The Trie View

Represent each codeword as a path in a binary tree. Start at the root. For each bit, go left (0) or right (1). The codeword ends at a node, which I mark as a terminal. A code is prefix-free if and only if no terminal node has any descendants that are also terminals. Once you reach a terminal on the way down, you stop.

The example code \(\{A \to \texttt{0},\ B \to \texttt{10},\ C \to \texttt{110},\ D \to \texttt{111}\}\) has lengths \((1, 2, 3, 3)\). Its trie looks like this:

```
         root
        /    \
       0      1
      [A]    / \
            0   1
           [B]  / \
               0   1
              [C] [D]
```

A is at depth 1, left branch. B is at depth 2, right-then-left. C and D share a parent at depth 2, then split at depth 3. No codeword's node is an ancestor of another's: the code is prefix-free.

The `BinaryTree` class in `kraft.hpp` implements exactly this structure. It supports inserting codewords (as strings of `'0'` and `'1'` characters), checking membership, and verifying prefix-freeness by a recursive scan:

```cpp
class BinaryTree {
public:
    BinaryTree() : root_(std::make_unique<Node>()) {}

    // Insert a codeword (a string of '0' and '1' characters).
    // Idempotent: inserting the same codeword twice is a no-op.
    void insert(const std::string& codeword) {
        Node* cur = root_.get();
        for (char c : codeword) {
            assert((c == '0' || c == '1') && "codeword must be over {0,1}");
            std::unique_ptr<Node>& child = (c == '0') ? cur->left : cur->right;
            if (!child) child = std::make_unique<Node>();
            cur = child.get();
        }
        cur->is_codeword = true;
    }

    // Returns true iff the codeword is in the tree.
    bool contains(const std::string& codeword) const {
        const Node* cur = root_.get();
        for (char c : codeword) {
            const std::unique_ptr<Node>& child = (c == '0') ? cur->left : cur->right;
            if (!child) return false;
            cur = child.get();
        }
        return cur->is_codeword;
    }

    // Returns true iff no codeword in the tree is a prefix of another.
    // Equivalently: no terminal node has any descendants that are terminal.
    [[nodiscard]] bool is_prefix_free() const {
        return is_prefix_free_recursive(root_.get(), false);
    }

private:
    struct Node {
        std::unique_ptr<Node> left;   // '0' branch
        std::unique_ptr<Node> right;  // '1' branch
        bool is_codeword = false;
    };

    std::unique_ptr<Node> root_;

    static bool is_prefix_free_recursive(const Node* node, bool ancestor_is_codeword) {
        if (!node) return true;
        // If this node is a codeword AND we passed through a codeword on the
        // way down, the ancestor codeword is a prefix of this one. Violation.
        // Equivalently: if this node is a codeword AND has any children that
        // are also codewords, this codeword is a prefix of those.
        if (node->is_codeword && ancestor_is_codeword) return false;
        bool below = node->is_codeword;
        if (!is_prefix_free_recursive(node->left.get(), ancestor_is_codeword || below)) return false;
        if (!is_prefix_free_recursive(node->right.get(), ancestor_is_codeword || below)) return false;
        return true;
    }
};
```

`is_prefix_free_recursive` passes a flag `ancestor_is_codeword` down the tree. If the current node is a terminal and an ancestor was also a terminal, that ancestor's codeword is a prefix of the current one: violation. This catches both directions of the prefix relationship in a single pass.

## The Inequality

Think of the unit interval as a budget. Each codeword of length \(l_i\) claims a fraction \(2^{-l_i}\) of that budget. Kraft's inequality says the total claim is at most 1:

$$\sum_{i=1}^{n} 2^{-l_i} \leq 1.$$

For the example code with lengths \((1, 2, 3, 3)\):

$$\frac{1}{2} + \frac{1}{4} + \frac{1}{8} + \frac{1}{8} = 1.$$

This code saturates the budget exactly. The fractions are \(2^{-1} = 0.5\) for A, \(2^{-2} = 0.25\) for B, and \(2^{-3} = 0.125\) for each of C and D.

The `kraft_sum` function computes the left-hand side. It uses `std::ldexp` to compute \(2^{-l}\) in floating point without overflow or underflow issues:

```cpp
inline double kraft_sum(const std::vector<std::size_t>& lengths) {
    double sum = 0.0;
    for (std::size_t l : lengths) {
        sum += std::ldexp(1.0, -static_cast<int>(l));
    }
    return sum;
}
```

Some examples from the test suite:

- Lengths \(\{1, 2, 3, 3\}\): sum is 1.0. (The example code above. Saturates.)
- Lengths \(\{2, 2, 2, 2\}\): sum is \(4 \times 0.25 = 1.0\). (All four 2-bit codewords. Also saturates.)
- Lengths \(\{1, 2, 3, 4, 5\}\): sum is \(\frac{1}{2} + \frac{1}{4} + \frac{1}{8} + \frac{1}{16} + \frac{1}{32} = \frac{31}{32} < 1\). (A prefix of the unary code. Strictly below 1.)
- Lengths \(\{1, 1, 1\}\): sum is \(1.5 > 1\). (Three 1-bit codewords are impossible: only "0" and "1" exist at depth 1.)

That last case violates Kraft, so no prefix-free code with those lengths exists. The check `is_kraft_satisfying` wraps this with a small floating-point tolerance:

```cpp
inline bool is_kraft_satisfying(const std::vector<std::size_t>& lengths) {
    constexpr double kTolerance = 1e-9;
    return kraft_sum(lengths) <= 1.0 + kTolerance;
}
```

## The Proof

The proof is a counting argument on the binary tree, made concrete by the `trie_embedding` function.

Fix a code with lengths \(l_1, l_2, \ldots, l_n\) and let \(l_{\max} = \max_i l_i\). Consider the complete binary tree of depth \(l_{\max}\): it has \(2^{l_{\max}}\) leaves, one for each binary string of length \(l_{\max}\).

Each codeword of length \(l_i\) is a string of length \(l_i\). In the depth-\(l_{\max}\) tree, it corresponds to a subtree: all strings of length \(l_{\max}\) that begin with the codeword. That subtree has \(2^{l_{\max} - l_i}\) leaves.

Prefix-freeness says: no codeword is a prefix of another. Equivalently, no string of length \(l_{\max}\) can begin with two distinct codewords. So the subtrees are disjoint: each leaf belongs to at most one codeword's subtree.

The total number of leaves in all subtrees is \(\sum_{i=1}^{n} 2^{l_{\max} - l_i}\). Since the subtrees are disjoint and all fit inside the depth-\(l_{\max}\) tree, this total is at most \(2^{l_{\max}}\):

$$\sum_{i=1}^{n} 2^{l_{\max} - l_i} \leq 2^{l_{\max}}.$$

Divide both sides by \(2^{l_{\max}}\):

$$\sum_{i=1}^{n} 2^{-l_i} \leq 1.$$

That is Kraft's inequality.

The `trie_embedding` function computes the subtree sizes directly, making the proof a concrete calculation:

```cpp
struct TrieEmbeddingInfo {
    std::size_t l_max;
    std::size_t total_leaves;            // 2^l_max
    std::size_t occupied_leaves;         // sum of subtree_sizes
    std::vector<std::size_t> subtree_sizes;  // 2^{l_max - l_i} for each codeword
};

inline TrieEmbeddingInfo trie_embedding(const std::vector<std::size_t>& lengths) {
    TrieEmbeddingInfo info;
    info.l_max = 0;
    for (std::size_t l : lengths) {
        if (l > info.l_max) info.l_max = l;
    }
    info.total_leaves = std::size_t{1} << info.l_max;
    info.subtree_sizes.reserve(lengths.size());
    info.occupied_leaves = 0;
    for (std::size_t l : lengths) {
        std::size_t size = std::size_t{1} << (info.l_max - l);
        info.subtree_sizes.push_back(size);
        info.occupied_leaves += size;
    }
    return info;
}
```

For the example code with lengths \(\{1, 2, 3, 3\}\), the embedding gives:

- \(l_{\max} = 3\), so the complete tree has \(2^3 = 8\) leaves.
- Codeword A (length 1): subtree size \(2^{3-1} = 4\). A claims half the tree.
- Codeword B (length 2): subtree size \(2^{3-2} = 2\). B claims a quarter.
- Codeword C (length 3): subtree size \(2^{3-3} = 1\). A single leaf.
- Codeword D (length 3): subtree size 1. Another single leaf.
- Occupied: \(4 + 2 + 1 + 1 = 8 = 2^3\). The code saturates the budget.

The test suite checks this exactly:

```cpp
TEST(KraftTest, TrieEmbeddingComputesSubtreeSizes) {
    auto info = trie_embedding({1, 2, 3, 3});
    EXPECT_EQ(info.l_max, 3u);
    EXPECT_EQ(info.total_leaves, 8u);
    EXPECT_EQ(info.subtree_sizes[0], 4u);  // length 1 -> 4 leaves
    EXPECT_EQ(info.subtree_sizes[1], 2u);  // length 2 -> 2 leaves
    EXPECT_EQ(info.subtree_sizes[2], 1u);  // length 3 -> 1 leaf
    EXPECT_EQ(info.subtree_sizes[3], 1u);  // length 3 -> 1 leaf
    EXPECT_EQ(info.occupied_leaves, 8u);   // saturates
}
```

An unsaturated case: lengths \(\{2, 2, 3\}\) give occupied \(2 + 2 + 1 = 5\) out of 8 leaves. Three leaves are unoccupied: you could add up to one more codeword of length 3, or a shorter codeword that doesn't conflict with the existing ones.

## What Kraft Gives Us

Three consequences fall out of Kraft's inequality directly.

**First: a budget.** Each codeword consumes a share of the unit budget. A codeword of length 1 costs 1/2. A codeword of length 3 costs 1/8. Once the budget is exhausted, no more codewords can be added without violating prefix-freeness. This is not a practical limitation but a mathematical fact: the fractions must sum to at most 1.

The budget framing makes trade-offs visible. If you want symbol A to have a very short codeword (large budget share), the remaining symbols must share whatever is left. The total is fixed. You are allocating a finite resource.

**Second: a trade-off, with an optimal point.** Shorter codewords for some symbols means longer codewords for others. The optimal trade-off, under a known probability distribution \((p_1, \ldots, p_n)\) over the symbols, is to assign length \(-\log_2 p_i\) to symbol \(i\). This minimizes the expected codeword length \(\sum_i p_i l_i\), and it saturates Kraft when all the \(p_i\) are dyadic (powers of 2). The lengths \(-\log_2 p_i\) are not always integers, which is why practical codes like Huffman (which uses integer lengths) incur a small overhead over the theoretical minimum, and why arithmetic coding can approach the minimum more closely by working below the integer boundary.

Kraft's inequality is what makes this optimization well-defined. The constraint \(\sum_i 2^{-l_i} \leq 1\) defines the feasible region; the optimization finds the best point in that region.

**Third: a diagnostic.** A length vector that violates Kraft has no prefix-free realization. This is a hard constraint, not a heuristic. If someone proposes a code with lengths that sum past 1 in Kraft's sense, no amount of clever codeword assignment will fix it: the tree does not have enough leaves.

Conversely, a length vector satisfying Kraft always has a prefix-free realization. That is McMillan's theorem, and it is where the story becomes constructive.

## The Converse, Foreshadowed

Kraft's inequality is necessary. McMillan's theorem (1956) says it is also sufficient: any length vector satisfying the inequality is realizable by some prefix-free binary code. You can always build the code.

The proof is constructive. Given a Kraft-satisfying length vector, you walk the binary tree left-to-right, assigning the next available node at the right depth to each symbol. The budget guarantee ensures you never run out of room before all symbols are placed.

This constructive direction is what makes Kraft practically useful: it turns a feasibility question ("does a code with these lengths exist?") into a simple arithmetic check. Compute the Kraft sum. If it is at most 1, the code exists. If not, it does not.

The [next post in this series](/post/2020-09-mcmillan-wire-formats/) proves McMillan's theorem and gives the construction explicitly.

## Cross-References

The [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) post in the Stepanov series develops codec combinators built on the algebraic structure of types. Each combinator, `Opt`, `Either`, `Pair`, `Vec`, relies on the element codecs being prefix-free for the decoding to be unambiguous. Kraft's inequality is what certifies that a codec's codeword-length assignment is internally consistent: any prefix-free codec's lengths satisfy it.

The [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/) post in the same series shows that prefix-freeness is exactly the condition that makes the free-monoid lift into bit space injective: the `Vec<C>` combinator works because `C` is prefix-free, and prefix-freeness is precisely what Kraft characterizes. Kraft and the free-monoid construction are two views of the same constraint.

---

PFC ([github.com/queelius/pfc](https://github.com/queelius/pfc)) provides production codecs that all satisfy Kraft. The whole library is structured around Kraft-satisfying universal codes; see `include/pfc/codecs.hpp` for the full catalog including Elias gamma, Elias delta, Fibonacci, Rice, VByte, and Exp-Golomb codes.
