---
title: "Huffman Coding"
date: 2024-08-04
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- huffman
- entropy-optimal
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 9
math: true
description: "Given a finite distribution, Huffman's algorithm builds the prefix-free code with minimum expected length. It is the first entropy-optimal code in this series."
linked_project:
- pfc
- wire-formats
---

## From Universal to Optimal

Every code in this series so far has been universal: it requires no prior knowledge of the source distribution. Elias gamma assigns shorter codewords to smaller integers regardless of which integers actually appear. Fibonacci coding does the same. VByte packs smaller values into fewer bytes without knowing whether your data is heavy at the low end or the high end. Universal codes are defensive: they perform acceptably across a broad class of inputs without committing to any particular one.

Huffman coding flips this stance. You bring a finite probability distribution, and Huffman finds the prefix-free code with the minimum expected codeword length for that distribution. There is no free ride: the code is tuned to the distribution you provide, and it will perform poorly on any other. Call this the move from defensive to offensive coding.

The payoff is significant. Shannon's source coding theorem says no prefix-free code can achieve an expected length below the Shannon entropy $H(p) = -\sum_i p_i \log_2 p_i$. Huffman gets within 1 bit of that bound. The bound is tight: for any prefix-free code, the expected length satisfies

$$H(p) \le L(\text{code}) \le H(p) + 1.$$

The upper bound comes from the integer-length constraint: each codeword must be a whole number of bits, and the ceiling of $-\log_2 p_i$ is at most $-\log_2 p_i + 1$. Huffman is optimal subject to this constraint, and no prefix-free code can do better without abandoning whole-bit codewords.

That last clause points to the limit of this post and the opening of the next. Arithmetic coding, forthcoming, breaks the integer-length constraint by assigning fractional bits in effect, reaching entropy exactly in the limit.

## The Algorithm

The four steps of Huffman's construction:

1. Create one leaf node per symbol, weighted by its probability.
2. Push all leaves into a min-priority queue (lowest weight first).
3. While the queue contains more than one node: extract the two lowest-weight nodes, merge them into a new internal node whose weight is their sum, push the internal node back.
4. The remaining node is the root. The path from root to each leaf encodes that leaf's codeword ("0" for left, "1" for right).

Here is the complete implementation from `huffman.hpp`:

```cpp
struct Node {
    double freq;
    int symbol = -1;  // -1 for internal nodes
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
};

inline std::unique_ptr<Node> build_huffman_tree(const std::vector<double>& freqs) {
    assert(!freqs.empty());

    auto cmp = [](const std::unique_ptr<Node>& a, const std::unique_ptr<Node>& b) {
        return a->freq > b->freq;  // min-heap
    };
    std::priority_queue<std::unique_ptr<Node>,
                        std::vector<std::unique_ptr<Node>>,
                        decltype(cmp)> pq(cmp);

    for (std::size_t i = 0; i < freqs.size(); ++i) {
        auto node    = std::make_unique<Node>();
        node->freq   = freqs[i];
        node->symbol = static_cast<int>(i);
        pq.push(std::move(node));
    }

    while (pq.size() > 1) {
        auto a = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();
        auto b = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();
        auto parent      = std::make_unique<Node>();
        parent->freq     = a->freq + b->freq;
        parent->left     = std::move(a);
        parent->right    = std::move(b);
        pq.push(std::move(parent));
    }

    return std::move(const_cast<std::unique_ptr<Node>&>(pq.top()));
}
```

One subtlety appears in the `const_cast` lines. `std::priority_queue::top()` returns `const&` to protect the heap invariant: if a caller could mutate the top element through the reference, the heap property would break. Here we call `pq.pop()` immediately after the move, restoring the invariant before any other operation can observe the moved-from state. The `const_cast` is safe precisely because of that pairing. It is the standard workaround for moving out of a priority queue. Do not replace the `std::move` with a copy: `Node` owns `std::unique_ptr` children and is not copyable.

The codebook is extracted by a recursive depth-first walk:

```cpp
inline std::map<int, std::string> tree_to_codebook(const Node* root,
                                                    std::string prefix = "") {
    if (root->symbol >= 0 && !root->left && !root->right)
        return {{root->symbol, prefix.empty() ? "0" : prefix}};

    std::map<int, std::string> result;
    if (root->left) {
        auto L = tree_to_codebook(root->left.get(), prefix + "0");
        result.insert(L.begin(), L.end());
    }
    if (root->right) {
        auto R = tree_to_codebook(root->right.get(), prefix + "1");
        result.insert(R.begin(), R.end());
    }
    return result;
}
```

Encoding looks up the symbol's codeword string and writes each character into a `BitSink`. Decoding walks the tree from the root, branching on each incoming bit, until a leaf is reached:

```cpp
template<BitSink S>
inline void encode(int symbol, const std::map<int, std::string>& codebook, S& sink) {
    for (char c : codebook.at(symbol)) sink.write(c == '1');
}

template<BitSource S>
inline int decode(const Node* root, S& source) {
    const Node* cur = root;
    while (cur->symbol < 0) {
        bool bit = source.read();
        cur = bit ? cur->right.get() : cur->left.get();
    }
    return cur->symbol;
}
```

Because the code is prefix-free, the decoder knows exactly when one codeword ends and the next begins without any delimiter. This is the defining property of the prefix-free constraint from [Kraft's Inequality](/post/2020-03-kraft-wire-formats/) and [McMillan's Converse](/post/2020-09-mcmillan-wire-formats/).

## Optimality

The proof that Huffman is optimal among prefix-free codes rests on three lemmas. Together they support an induction argument on the alphabet size.

**Lemma 1: the two least-frequent symbols get the longest codewords (or are tied for longest).** Suppose for contradiction that some symbol $x$ with frequency $p_x$ has a strictly longer codeword than the two least-frequent symbols $a$ and $b$ (with $p_a \le p_b \le p_x$). Swapping the codewords of $a$ and $x$ would shorten the average length by $(p_a - p_x)(|cw_a| - |cw_x|) < 0$ (since $p_a < p_x$ and $|cw_a| > |cw_x|$). That contradicts optimality.

**Lemma 2: the two least-frequent symbols can always be assigned sibling codewords (codewords that differ only in the last bit).** Any prefix-free code has a binary tree representation. If the two longest-codeword nodes are not siblings, we can restructure the tree to make them siblings without increasing average length. This restructuring is always possible because the deepest level of a prefix-free code tree always contains pairs of sibling leaves.

**Lemma 3 (the merger step): combining the two least-frequent symbols into a single merged symbol yields a strictly smaller problem whose optimal solution lifts back to the original.** If $C^*$ is an optimal code for the merged alphabet (with merged symbol having probability $p_a + p_b$), then the code for the original alphabet formed by adding one bit to the merged codeword (assigning $cw_a = cw^* + 0$ and $cw_b = cw^* + 1$) is optimal for the original alphabet. The expected length increases by exactly $p_a + p_b$ (one extra bit for the pair), which is unavoidable.

Induction closes the argument: a one-symbol alphabet has trivially optimal coding; the merger step reduces n symbols to n-1 symbols; by induction hypothesis, the merged problem is solved optimally; by Lemma 3, the solution lifts back optimally to the n-symbol problem.

The consequence: Huffman's expected length $L$ satisfies $H(p) \le L \le H(p) + 1$. The lower bound is Shannon's theorem (no prefix-free code beats entropy). The upper bound follows from the fact that assigning codeword length $\lceil -\log_2 p_i \rceil$ to each symbol satisfies the Kraft inequality and yields $L \le H(p) + 1$, so the Huffman code (which is at most as long as this code) also satisfies the upper bound.

For dyadic distributions (where each $p_i$ is a negative power of 2), $-\log_2 p_i$ is already an integer, so Huffman achieves entropy exactly. For all other distributions, there is a gap of up to 1 bit.

Arithmetic coding removes the gap by encoding sequences of symbols jointly, amortizing the integer-rounding slack over arbitrarily long blocks. The per-symbol redundancy approaches zero as block length grows.

## McMillan's Construction Returns

Post 2 showed that any Kraft-satisfying length vector $(\ell_1, \ldots, \ell_n)$ can be turned into a prefix-free code by [McMillan's construction](/post/2020-09-mcmillan-wire-formats/). Huffman solves the complementary problem: given a distribution $(p_1, \ldots, p_n)$, find the Kraft-satisfying length vector that minimizes $\sum_i p_i \ell_i$.

Viewed this way, Huffman is a two-stage machine. Stage one identifies the optimal lengths. Stage two applies McMillan's construction to produce the code. The Huffman tree algorithm compresses both stages into one pass, building the tree and the code simultaneously, but the logical separation is illuminating.

The chain of ideas in this series now reads:

- Post 1, Kraft: which length vectors are valid (they satisfy the Kraft sum)?
- Post 2, McMillan: given a valid length vector, how do you build the code?
- Post 3, Priors: what distribution does a code implicitly assume (the one that makes the code optimal)?
- Post 9, Huffman: given an explicit distribution, find the optimal length vector.

The first three posts ran from codes to distributions. This post runs the other direction: from distributions to codes.

## Tests and Demonstrations

The test suite in `test_huffman.cpp` covers the theoretical claims directly. A few highlights:

**Uniform alphabets:** For a uniform distribution over $2^k$ symbols, entropy is exactly $k$ bits and Huffman produces a fixed-width $k$-bit code (all codewords the same length). The suite checks $k = 2$ (4 symbols) and $k = 3$ (8 symbols).

**Dyadic distributions:** For $p = (0.5, 0.25, 0.125, 0.125)$, each $p_i$ is a power of 2, so entropy is $1.75$ bits and Huffman achieves it exactly. The test verifies `EXPECT_NEAR(L, H, 1e-9)`.

**Highly skewed binary:** For $(0.99, 0.01)$, entropy is about 0.081 bits, but Huffman cannot use fewer than 1 bit per symbol (both symbols need at least one bit). The test confirms $L = 1.0$ and $L \le H + 1$.

**Comparison with gamma:** On a power-law-2 distribution over 16 symbols, Huffman's expected length is no greater than Elias gamma's expected length. Gamma is universal but pays for that universality.

The `codebook_lengths` helper bridges the Huffman codebook to the `priors` library from post 3:

```cpp
static std::vector<std::size_t> codebook_lengths(
        const std::map<int, std::string>& cb, std::size_t n) {
    std::vector<std::size_t> lens(n);
    for (const auto& [sym, cw] : cb)
        lens[sym] = cw.size();
    return lens;
}
```

With `lens` in hand, `priors::entropy(freqs)` and `priors::expected_length(freqs, lens)` compute the quantities needed to verify the $H \le L \le H + 1$ bound directly.

## Limitations

Huffman has three practical limitations worth naming before moving on.

**First, it requires the distribution as input.** For text or binary data whose statistics are unknown, you must either estimate the distribution first (a two-pass approach: count frequencies on the training data, then encode the actual data) or use an adaptive variant that updates the code as it goes. Two-pass schemes transmit the codebook alongside the compressed data, adding overhead. Adaptive schemes avoid the codebook overhead but carry computational cost per symbol.

**Second, the integer-length constraint is binding for non-dyadic distributions.** A symbol with probability 0.37 ideally gets a codeword of length $-\log_2 0.37 \approx 1.43$ bits. Huffman must assign either 1 or 2 bits. For some distributions, notably a binary source with $p$ far from 0.5, the gap between entropy and Huffman length approaches 1 bit per symbol.

**Third, block Huffman only partially helps.** Encoding $k$-symbol tuples reduces the per-symbol redundancy from at most 1 bit to at most $1/k$ bits, because the per-block redundancy is still bounded by 1 bit. But the alphabet size grows as $n^k$, making the codebook exponentially large. For $n = 256$ bytes and $k = 3$, the alphabet has $16{,}777{,}216$ entries. In practice, block Huffman is limited to very small $k$ (typically 2 or 3) or very small alphabets.

Arithmetic coding, the subject of the next post, resolves all three limitations without a block-size explosion. It works on a per-symbol basis like Huffman but achieves arbitrary precision by representing the encoder state as a shrinking real interval. The integer-rounding slack disappears because no individual symbol is assigned a codeword at all; the compressed output emerges from the interval arithmetic directly.

## Cross-References

**Forthcoming:** Arithmetic Coding (post 10) extends the ideas here by removing the integer-length constraint and approaching entropy exactly.

**Back-references (live):**
- [McMillan's Converse](/post/2020-09-mcmillan-wire-formats/) (post 2): the construction that turns any Kraft-satisfying length vector into a prefix-free code. Huffman finds the optimal length vector; McMillan builds the code.
- [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3): the `priors::entropy` and `priors::expected_length` functions used in the optimality tests come from this post's `priors.hpp`.

**Cross-series:** [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) (Stepanov series): when a `PackedEither` combinator chooses among $N$ type variants, it pays $\lceil \log_2 N \rceil$ tag bits per value. Huffman provides the entropy-optimal alternative when the variant frequencies are known, replacing the fixed $\log_2 N$ cost with a code tuned to the actual distribution of variants.

**Production implementation:** The pedagogical `huffman.hpp` here is a self-contained teaching tool. The production version lives at `include/pfc/huffman.hpp` in the [PFC library](https://github.com/queelius/pfc).
