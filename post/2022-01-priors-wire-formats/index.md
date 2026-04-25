---
title: "Universal Codes as Priors"
date: 2022-01-15
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- Shannon
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 3
math: true
description: "Every prefix-free code is a hypothesis about the source. The codeword lengths determine an implicit probability distribution; the code is optimal when that prior matches the true source."
linked_project:
- pfc
- wire-formats
---

## Universal Codes as Priors

You want to compress a stream of positive integers. Which code should you use?

The question feels open-ended, but it has a structure. A code for integers assigns a codeword to each integer. The codeword for 1 is short, for 2 a bit longer, for 100 much longer. The relative lengths encode an implicit bet: what fraction of the stream will be 1s? What fraction will be 100s? If your bet matches the source, you win: the average codeword length will be close to the theoretical minimum (the entropy). If your bet is wrong, you pay an overhead proportional to how wrong you are.

The bet is not a separate parameter. It lives in the codeword lengths themselves. This is the central claim of this post:

**Every prefix-free code is a prior over the integers.** The codeword lengths determine, up to normalization, a probability distribution. The code is optimal for exactly the sources that match that distribution.

This post makes that claim precise and implements the tools to measure it. The rest of the series (posts 4 through 12) examines ten specific codes and the priors they embody.

---

## The Correspondence: Lengths to Priors

For a prefix-free code with codeword lengths \((l_1, l_2, \ldots, l_n)\), define the *unnormalized weight* of symbol \(i\) as \(w_i = 2^{-l_i}\). This is the fraction of the Kraft budget consumed by that codeword.

If the code saturates Kraft (meaning \(\sum_i 2^{-l_i} = 1\)), then the weights are already a valid probability distribution: \(p_i = 2^{-l_i}\). If the code does not saturate (meaning \(\sum_i 2^{-l_i} < 1\)), normalize: \(p_i = 2^{-l_i} / \sum_j 2^{-l_j}\).

This is the inverse of Shannon's prescription. Shannon tells us: given a distribution \(p_i\), the optimal codeword length is \(\lceil -\log_2 p_i \rceil\) bits. We reverse the direction: given a length \(l_i\), the implied probability is \(2^{-l_i}\).

The function `implied_prior` computes this map:

```cpp
inline std::vector<double> implied_prior(const std::vector<std::size_t>& lengths) {
    std::vector<double> probs;
    probs.reserve(lengths.size());
    double total = 0.0;
    for (std::size_t l : lengths) {
        double p = std::ldexp(1.0, -static_cast<int>(l));
        probs.push_back(p);
        total += p;
    }
    // Normalize if Kraft sum is less than 1.
    if (total < 1.0) {
        for (double& p : probs) p /= total;
    }
    return probs;
}
```

Two examples illustrate the range of priors:

**Unary:** lengths \(\{1, 2, 3, 4, \ldots\}\). Kraft sum \(= \sum_{n=1}^\infty 2^{-n} = 1\) (saturating). The implied prior is \(p_n = 2^{-n}\): a geometric distribution with ratio \(1/2\). Symbol 1 gets probability 1/2, symbol 2 gets probability 1/4, and so on. Every symbol is half as likely as the previous one.

**Elias gamma:** for a symbol \(n\) in the block \([2^k, 2^{k+1})\), the codeword length is \(2k+1\). So \(n=1\) gets length 1, \(n \in \{2,3\}\) get length 3, \(n \in \{4,5,6,7\}\) get length 5, and so on. Kraft sum \(= \sum_{k=0}^\infty 2^k \cdot 2^{-(2k+1)} = \sum_{k=0}^\infty 2^{-(k+1)} = 1\) (saturating). The implied prior assigns the same probability to all symbols in the same block, and the probability falls roughly as \(1/n^2\).

These two examples are the subject of post 4. The remaining codes appear in the table at the end of this post.

---

## Optimality: Shannon's Theorem and Expected Length

Shannon's source-coding theorem says: for any distribution \(p_i\) and any prefix-free code with integer lengths \(l_i\), the expected codeword length satisfies

$$\mathbb{E}[l] = \sum_i p_i l_i \geq H(p) = -\sum_i p_i \log_2 p_i.$$

Equality holds if and only if \(l_i = -\log_2 p_i\) for all \(i\), which requires the distribution to be *dyadic* (each \(p_i\) a power of \(1/2\)) and the Kraft sum to be exactly 1.

This gives us three functions worth naming:

```cpp
// Shannon entropy of a distribution.
inline double entropy(const std::vector<double>& probs) {
    double h = 0.0;
    for (double p : probs) {
        if (p > 0.0) h -= p * std::log2(p);
    }
    return h;
}

// Expected codeword length under a distribution.
inline double expected_length(const std::vector<double>& probs,
                              const std::vector<std::size_t>& lengths) {
    assert(probs.size() == lengths.size());
    double L = 0.0;
    for (std::size_t i = 0; i < probs.size(); ++i) {
        L += probs[i] * static_cast<double>(lengths[i]);
    }
    return L;
}

// Excess bits above the Shannon minimum.
inline double redundancy(const std::vector<double>& probs,
                         const std::vector<std::size_t>& lengths) {
    return expected_length(probs, lengths) - entropy(probs);
}
```

The function `redundancy` is always non-negative (Shannon), and equals zero precisely when the code is exactly optimal for that distribution. The tests in `test_priors.cpp` verify this: for any prefix-free code and any distribution, `redundancy >= 0`, with equality at the dyadic optimum.

The connection to the prior is direct. When you evaluate `redundancy(true_source, code_lengths)`, you are measuring how well the code's implicit prior matches the true source. A good code (low redundancy) is one whose prior is close to the truth.

---

## Two Concrete Examples: Unary and Gamma

Two concrete cases anchor the intuition before the full table.

**Unary on a geometric source.** The unary code's implied prior is geometric(1/2): \(p_n = 2^{-n}\) (truncated and normalized for a finite alphabet). Because the implied prior is dyadic and saturates Kraft, the code achieves entropy exactly on this prior. We can verify this numerically:

```cpp
// Build geometric(1/2) truncated to K symbols.
const std::size_t K = 30;
std::vector<std::size_t> unary_lens(K);
for (std::size_t i = 0; i < K; ++i) unary_lens[i] = i + 1;

std::vector<double> geo(K);
double z = 0.0;
for (std::size_t i = 0; i < K; ++i) {
    geo[i] = std::ldexp(1.0, -static_cast<int>(i + 1));
    z += geo[i];
}
for (double& p : geo) p /= z;

// expected_length(geo, unary_lens) equals entropy(geo) to within ~0.01.
assert(std::abs(expected_length(geo, unary_lens) - entropy(geo)) < 0.01);
```

The result: expected length equals entropy to within 0.01 bits. The mismatch is truncation error (the tail probability is \(\approx 2^{-30}\)).

**Gamma on a power-law source.** The Elias gamma code's implied prior falls roughly as \(1/n^2\). A power-law source with exponent 2 (\(p_n \propto 1/n^2\), normalized) is a good match. The code achieves bounded redundancy on this source:

```cpp
// Build power-law(2) source normalized to 64 symbols.
const std::size_t N = 64;
std::vector<std::size_t> gamma_lens(N);
for (std::size_t i = 0; i < N; ++i) {
    std::size_t n = i + 1;
    std::size_t k = std::bit_width(n) - 1;
    gamma_lens[i] = 2 * k + 1;
}

std::vector<double> pl(N);
double z = 0.0;
for (std::size_t i = 0; i < N; ++i) {
    double n = static_cast<double>(i + 1);
    pl[i] = 1.0 / (n * n);
    z += pl[i];
}
for (double& p : pl) p /= z;

// Redundancy is small (< 3 bits) even for a 64-symbol truncation.
double r = redundancy(pl, gamma_lens);  // measured: ~0.4 bits
```

The redundancy is small but nonzero. This is expected: gamma's block structure (all symbols in \([2^k, 2^{k+1})\) get the same codeword length) means it cannot exactly match the smooth \(1/n^2\) decay. The redundancy is bounded by a constant (about 2 bits per symbol for most power-law sources), which is what we mean by *universal*.

The key comparison: unary is catastrophic on a power-law source (because its lengths grow linearly, not logarithmically), and gamma is catastrophic on a geometric source (because its lengths grow logarithmically, much slower than needed for the sharp geometric decay). Each code is optimal in its own regime.

---

## The Table of Priors

Here is a summary of the eight codes developed across this series, with their implied priors and the source they are best for:

| Code | Codeword length for \(n\) | Implied prior \(p_n \propto\) | Best for |
|---|---|---|---|
| Unary | \(n\) | \(2^{-n}\) (geometric, \(r=1/2\)) | Geometric source; most values are 1 or 2 |
| Elias gamma | \(2\lfloor\log_2 n\rfloor + 1\) | \(\approx 1/n^2\) (power law, exp. 2) | Power-law source; word frequencies |
| Elias delta | \(\approx \log_2 n + 2\log_2\log_2 n\) | \(\approx 1/(n\log^2 n)\) | Heavier tails than \(1/n^2\) |
| Fibonacci | \(\approx 1.44\log_\phi n\) | \(\propto \phi^{-n}\) | Robust; single-bit errors detectable |
| Rice(\(k\)) / Golomb(\(m\)) | \(\lfloor n/m\rfloor + k + 1\) | Geometric, tunable rate | Tunable geometric; pick \(k\) from data |
| VByte / Varint | 8, 16, 24, or 32 bits | Approximately step-uniform | Byte-aligned; most values fit in 1 byte |
| Huffman | Matches the source exactly | Exact for any finite distribution | Known alphabet with known frequencies |
| Arithmetic | Real-valued bits per symbol | Exact for any distribution | Universal; non-integer bits per symbol |

The progression moves from the most constrained (unary: one specific geometric prior) to the most general (arithmetic: any distribution). Universal codes (unary through VByte) need no prior knowledge. Entropy-optimal codes (Huffman, arithmetic) match any distribution but need the distribution as input.

Choosing a code is choosing a prior. The table is a map from priors to codes.

---

## Universality

What does it mean for a code to be *universal*?

An entropy-optimal code (Huffman, arithmetic) achieves expected length exactly equal to entropy, but only when the distribution is known. If you do not know the distribution, you cannot build the Huffman tree or set the arithmetic coder's interval boundaries.

A universal code is different: it achieves expected length within a constant additive overhead of entropy across a wide class of distributions, without knowing the distribution in advance. The overhead is the *minimax redundancy*: the worst-case redundancy over the class, minimized over all possible codes.

For Elias gamma, the class is all distributions where \(p_n = O(1/n^2)\). For any source in this class, gamma's redundancy is bounded by a constant (roughly 2 bits per symbol). For unary, the class is all geometric distributions; unary achieves entropy exactly for one specific geometric prior, and bounded redundancy for nearby geometric distributions.

The word "universal" in "universal codes" means: the code is good enough for any source in a broad class, even without knowing which specific source you have. This is different from, and much weaker than, being optimal for one specific source.

The tradeoff: universality costs bits. Gamma uses \(2\lfloor\log_2 n\rfloor + 1\) bits for \(n\), while an optimal Huffman code for a known power-law(2) source might use \(\lceil\log_2 n + \log_2\log_2 n\rceil\) bits, which is shorter. The universal code pays a constant overhead to avoid needing the distribution.

Posts 4 through 8 develop the universal codes from this table, each with its implied prior and a test showing bounded redundancy. Posts 9 and 10 develop the entropy-optimal codes (Huffman and arithmetic), which close the gap but require the distribution as input.

---

## Cross-references

The next post, [Unary and Elias Gamma](/post/2022-06-elias-gamma-wire-formats/), implements the first two codes from the table above with full round-trip tests and prior analysis.

The foundations of this series: [Kraft's Inequality](/post/2020-03-kraft-wire-formats/) characterizes which length vectors are achievable as prefix-free codes (and therefore which implied priors are realizable). [McMillan's Converse](/post/2020-09-mcmillan-wire-formats/) gives the constructive direction: for any achievable length vector, a prefix-free code exists.

This series develops the bit-level information-theory side of prefix-free coding. The type-algebra side is developed in [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) and [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/), which treat codecs as functors and show how prefix-free codes arise from algebraic structure.

---

*The production version of these codes is in PFC's `include/pfc/codecs.hpp`, which implements the full catalogue of universal codes with zero-copy algebraic type support. This series develops them pedagogically, one at a time.*
