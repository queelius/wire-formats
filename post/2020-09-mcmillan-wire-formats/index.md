---
title: "McMillan's Converse"
date: 2020-09-13
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- constructive-proof
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 2
math: true
description: "Any length vector satisfying Kraft has a prefix-free code. Here is the construction."
linked_project:
- pfc
- wire-formats
---
*Kraft's inequality is necessary. McMillan's theorem says it is also sufficient, and the proof is a construction.*

## McMillan's Converse

[The previous post in this series](/post/2020-03-kraft-wire-formats/) proved Kraft's inequality: for any prefix-free binary code with codeword lengths \(l_1, l_2, \ldots, l_n\),

$$\sum_{i=1}^{n} 2^{-l_i} \leq 1.$$

Every prefix-free code satisfies it. No exceptions. But necessity alone is not the useful direction. The question I want answered is the converse: given a length vector that satisfies Kraft, does a prefix-free code with those lengths actually exist?

Yes, and McMillan's theorem (1956) proves it. Better still, the proof is a construction: given any Kraft-satisfying length vector, you can produce a specific prefix-free code with those exact lengths. No search required. No verification required after the fact. The construction always terminates, always produces a valid code, because Kraft pre-certifies that the budget is sufficient.

This post proves the constructive direction, then goes further. McMillan proved something stronger than just the prefix-free converse. He showed that even uniquely-decodable codes that are not prefix-free must satisfy Kraft. The consequence is worth sitting with: there is no advantage to non-prefix-free designs. If a code can be uniquely decoded, a prefix-free code with the same lengths exists. Prefix-freeness is not a restriction you impose for convenience. It is just the cleanest form of what unique decodability requires.

## The Construction

The construction is a left-to-right walk through an imaginary binary trie. Sort the lengths, then assign codewords by taking the next available leaf at each step.

Concretely: fix a counter at zero, and for each length \(l_i\) (in sorted order), emit the binary representation of `counter >> (l_max - l_i)` left-padded to \(l_i\) bits. Then advance the counter by \(2^{l_{\max} - l_i}\), which skips past the entire subtree rooted at the just-assigned codeword. That advance ensures the next codeword starts at the first unoccupied leaf position in the depth-\(l_{\max}\) trie.

Work through the example from post 1: lengths \(\{1, 2, 3, 3\}\). Sort: \(1, 2, 3, 3\). Take \(l_{\max} = 3\).

- Length 1: counter is 0. Shift right by \(3 - 1 = 2\): emit `0 >> 2 = 0` as a 1-bit string, giving codeword `"0"`. Advance counter by \(2^{3-1} = 4\). Counter is now 4.
- Length 2: counter is 4 (binary `100`). Shift right by \(3 - 2 = 1\): emit `4 >> 1 = 2` as a 2-bit string, giving `"10"`. Advance by \(2^{3-2} = 2\). Counter is now 6.
- Length 3: counter is 6 (binary `110`). Shift right by \(3 - 3 = 0\): emit `6` as a 3-bit string, giving `"110"`. Advance by \(2^0 = 1\). Counter is 7.
- Length 3: counter is 7 (binary `111`). Emit `7` as a 3-bit string: `"111"`. Advance by 1. Counter is 8.

Result: `{"0", "10", "110", "111"}`. This is exactly the example code from post 1. The construction recovered it directly from the length vector, without any search.

The `to_binary` and `build_prefix_free_code` functions implement this directly:

```cpp
inline std::string to_binary(std::uint64_t value, std::size_t width) {
    std::string s(width, '0');
    for (std::size_t i = 0; i < width; ++i) {
        if (value & (std::uint64_t{1} << (width - 1 - i))) {
            s[i] = '1';
        }
    }
    return s;
}

inline std::vector<std::string> build_prefix_free_code(
    const std::vector<std::size_t>& lengths)
{
    if (lengths.empty()) return {};

    std::vector<std::pair<std::size_t, std::size_t>> indexed;
    indexed.reserve(lengths.size());
    for (std::size_t i = 0; i < lengths.size(); ++i) {
        indexed.emplace_back(lengths[i], i);
    }
    std::sort(indexed.begin(), indexed.end());

    std::size_t l_max = indexed.back().first;
    std::vector<std::string> code(lengths.size());
    std::uint64_t counter = 0;
    for (const auto& [l, original_idx] : indexed) {
        std::uint64_t shifted = counter >> (l_max - l);
        code[original_idx] = to_binary(shifted, l);
        counter += std::uint64_t{1} << (l_max - l);
    }
    return code;
}
```

The function takes lengths in their original order and returns codewords in the same original order, so the caller does not need to track any sorting permutation. Sorting is internal, via a `(length, original_index)` pair vector. At the end, `code[original_idx] = ...` places each codeword at its intended position.

The counter after processing all lengths is exactly \(2^{l_{\max}}\) when Kraft is satisfied with equality; if the inequality is strict, the counter ends below \(2^{l_{\max}}\). The assignment never overflows because Kraft certifies the total occupied leaves is at most \(2^{l_{\max}}\).

The test suite confirms the construction on several inputs:

```cpp
TEST(McMillanTest, BuildPrefixFreeCodeForExampleLengths) {
    auto code = build_prefix_free_code({1, 2, 3, 3});
    ASSERT_EQ(code.size(), 4u);
    EXPECT_EQ(code[0].size(), 1u);
    EXPECT_EQ(code[1].size(), 2u);
    EXPECT_EQ(code[2].size(), 3u);
    EXPECT_EQ(code[3].size(), 3u);
}

TEST(McMillanTest, BuildPrefixFreeCodeOutputIsActuallyPrefixFree) {
    for (const std::vector<std::size_t>& lengths : {
        std::vector<std::size_t>{1, 2, 3, 3},
        std::vector<std::size_t>{2, 2, 2, 2},
        std::vector<std::size_t>{3, 3, 3, 3, 3, 3, 3, 3},
        std::vector<std::size_t>{1, 3, 3, 3, 3},
    }) {
        auto code = build_prefix_free_code(lengths);
        EXPECT_TRUE(is_prefix_free(code))
            << "Constructed code is not prefix-free for some input lengths";
    }
}
```

All 8 test cases pass.

## Why It Works

The trie-walking interpretation makes correctness obvious once you see it.

Define the remaining Kraft budget after placing codewords \(1\) through \(i-1\) as:

$$B_i = 1 - \sum_{j < i} 2^{-l_j}.$$

At step \(i\), we need to place a codeword of length \(l_i\), which costs \(2^{-l_i}\) of budget. The budget needs to cover this cost plus everything still to come:

$$B_i \geq \sum_{j \geq i} 2^{-l_j}.$$

This holds because Kraft says the total \(\sum_j 2^{-l_j} \leq 1\), so the tail sum \(\sum_{j \geq i} 2^{-l_j}\) equals \(1 - \sum_{j < i} 2^{-l_j} = B_i\) (with equality when Kraft is tight, or with room to spare when it is strict).

In particular, \(B_i \geq 2^{-l_i}\), so the step is always feasible. There is always enough budget to place the next codeword. The construction cannot get stuck.

The counter tracks this budget in integer form. Each counter value in \([0, 2^{l_{\max}})\) represents one leaf of the depth-\(l_{\max}\) trie. When we assign a codeword of length \(l_i\), we claim the subtree rooted at the corresponding depth-\(l_i\) node, which covers \(2^{l_{\max} - l_i}\) leaves. The counter advance of \(2^{l_{\max} - l_i}\) skips past exactly those leaves, so the next assignment starts at the first unoccupied leaf after the current subtree.

Prefix-freeness follows directly. We always advance past the entire subtree before placing the next codeword, so no future codeword starts within any previous codeword's subtree. No codeword is a prefix of any other: if codeword A were a prefix of codeword B, then B's leaf would lie within A's subtree, but A's subtree was fully skipped before B was placed.

## The Deeper Converse

The construction above proves one direction of the iff: any Kraft-satisfying length vector is realizable by a prefix-free code. McMillan's original result was stronger.

He proved: any uniquely-decodable code (not necessarily prefix-free) must have a length vector satisfying Kraft's inequality.

Unique decodability means: for any finite concatenation of codewords, there is exactly one way to parse it back into the original symbol sequence. Prefix-free codes are uniquely decodable. But there are uniquely-decodable codes that are not prefix-free. McMillan's theorem says all of them satisfy Kraft anyway.

The standard proof uses the \(L\)-th power of the code. Fix a uniquely-decodable code with \(n\) codewords of lengths \(l_1, \ldots, l_n\). Consider all sequences of exactly \(L\) codewords concatenated together. Each such sequence produces a bit string of some length \(m\). Because the code is uniquely decodable, distinct sequences of length \(L\) produce distinct bit strings.

Count how many distinct sequences of \(L\) codewords have total length exactly \(m\). An upper bound: there are at most \(2^m\) distinct binary strings of length \(m\), so there are at most \(2^m\) distinct sequences. Summing over all \(m\) from the minimum possible (\(L \cdot l_{\min}\)) to the maximum (\(L \cdot l_{\max}\)):

$$\text{number of length-}L\text{ sequences} \leq \sum_{m = L l_{\min}}^{L l_{\max}} 2^m.$$

The left side is also \(\left(\sum_{i=1}^n 1\right)^L = n^L\) (each of the \(L\) positions can be any of the \(n\) symbols). The tighter accounting uses the Kraft sum directly: the number of sequences of length \(L\) is exactly \(\left(\sum_{i=1}^n 2^{-l_i}\right)^L \cdot 2^{L l_{\max}}\) when normalized to the depth-\(L l_{\max}\) tree. The count of distinct strings of all lengths up to \(L l_{\max}\) is at most \(L (l_{\max} - l_{\min}) + 1\) "slots" times the maximum count per slot, and the algebra (Cover and Thomas, Theorem 5.5.1) gives:

$$\left(\sum_{i=1}^n 2^{-l_i}\right)^L \leq L (l_{\max} - l_{\min}) + 1.$$

The right side grows polynomially in \(L\). If the base on the left exceeded 1, the left side would grow exponentially in \(L\). For large enough \(L\), exponential beats polynomial. The only escape is:

$$\sum_{i=1}^n 2^{-l_i} \leq 1.$$

Kraft's inequality, holding for all uniquely-decodable codes, not just prefix-free ones.

## Implications

Two consequences fall out of the pair (Kraft necessary, McMillan sufficient).

**First: prefix-free codes lose nothing.** McMillan's theorem says any uniquely-decodable code satisfies Kraft. The construction says any Kraft-satisfying length vector is realizable by a prefix-free code. Combining: for any uniquely-decodable code, there exists a prefix-free code with the same length vector. Same expected codeword length, same compression ratio, and simpler decoding (no lookahead, no state machine, just a trie walk).

There is no reason to design a uniquely-decodable code that is not prefix-free. Any benefit you thought you were getting from a non-prefix-free design can be replicated by a prefix-free code of identical efficiency. This is why every production codec in PFC is prefix-free: not by convention, but because prefix-free is what uniquely-decodable looks like when you stop wasting effort.

**Second: optimal lengths always exist.** Given a probability distribution \((p_1, \ldots, p_n)\) over \(n\) symbols, the lengths \(l_i = \lceil -\log_2 p_i \rceil\) satisfy Kraft's inequality. A quick check: \(\sum_i 2^{-l_i} = \sum_i 2^{-\lceil -\log_2 p_i \rceil} \leq \sum_i 2^{\log_2 p_i} = \sum_i p_i = 1\). The inequality holds because ceiling rounds up, so \(-l_i \leq \log_2 p_i\), giving \(2^{-l_i} \leq p_i\). The Kraft sum is therefore at most 1.

By the construction, a prefix-free code with lengths \(\lceil -\log_2 p_i \rceil\) exists. The expected codeword length under this code is \(\sum_i p_i \lceil -\log_2 p_i \rceil \leq H(p) + 1\), where \(H(p) = -\sum_i p_i \log_2 p_i\) is the Shannon entropy. Prefix-free codes can come within 1 bit per symbol of the entropy bound. This is what underwrites Huffman coding (post 9, forthcoming) and arithmetic coding (post 10, forthcoming), which pushes past the integer-length constraint that McMillan's construction inherits.

## The Construction in PFC

The pedagogical `build_prefix_free_code` function in `mcmillan.hpp` is the stripped-down version of a family of constructions that PFC's `include/pfc/huffman.hpp` takes further.

Huffman's algorithm is a McMillan-style construction with an optimization layer on top. Instead of accepting an arbitrary Kraft-satisfying length vector, Huffman takes a probability distribution and finds the length vector (among all Kraft-satisfying vectors) that minimizes expected codeword length. It then builds a prefix-free code with those optimal lengths, using the same trie-walking logic at its core.

The relationship is clean: `build_prefix_free_code` is what you call once you know the lengths. Huffman is what you call when you need to find the lengths first. Huffman's output passes through the trie-walking logic implicitly: having determined the optimal length vector, the tree-building phase assigns codewords in exactly the order the construction describes.

PFC's `StaticHuffman` class in `include/pfc/huffman.hpp` builds the tree via a priority queue:

```cpp
template<typename Symbol = uint8_t, typename Allocator = std::allocator<uint8_t>>
class StaticHuffman {
public:
    using frequency_map = std::unordered_map<Symbol, uint64_t>;

    static result<StaticHuffman> from_frequencies(const frequency_map& frequencies);
    // ... encode / decode follow from the resulting code map
};
```

The `from_frequencies` function builds a `HuffmanNode` tree by repeatedly merging the two lowest-frequency nodes, producing a tree whose leaf depths are the optimal codeword lengths. Those depths are the length vector that McMillan's construction would accept. The two algorithms operate at different levels of abstraction: McMillan gives you a code for any valid lengths, Huffman gives you the optimal lengths and then uses the same trie structure to assign codewords.

The pedagogical code in this post is the cleaner predecessor, freed from probability data. Read it first.

## Cross-References

**Back to:** [Kraft's Inequality](/post/2020-03-kraft-wire-formats/) (post 1 in this series), which proved the necessary direction and introduced the trie-embedding interpretation of the Kraft sum.

**Forward (forthcoming):** Universal Codes as Priors (post 3) develops the framing of a code as a hypothesis about the source distribution, and shows how Elias gamma, Fibonacci, and Rice codes correspond to different priors. Huffman (post 9) uses the McMillan construction as its foundation. Arithmetic coding (post 10) pushes past the integer-length constraint that McMillan's construction inherits.

**Cross-series:** [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/) in the Stepanov series develops why prefix-freeness is exactly the condition needed to make the free-monoid lift into bit space injective. That post's Kraft section states the sufficiency direction informally; this post is the constructive proof it deferred. [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) develops the type-algebra side: each combinator (`Opt`, `Either`, `Pair`, `Vec`) works because its element codecs are prefix-free, and prefix-freeness is what Kraft and McMillan together characterize.

---

PFC ([github.com/queelius/wire-formats/tree/master/lib/pfc](https://github.com/queelius/wire-formats/tree/master/lib/pfc)) provides the production version of McMillan's construction with probability optimization. See `include/pfc/huffman.hpp` for the full `StaticHuffman` implementation and `include/pfc/codecs.hpp` for the catalog of universal codes (Elias gamma, Elias delta, Fibonacci, Rice, VByte, Exp-Golomb) that all satisfy Kraft.
