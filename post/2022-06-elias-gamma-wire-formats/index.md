---
title: "Unary and Elias Gamma"
date: 2022-06-19
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- unary
- elias-gamma
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 4
math: true
description: "Unary and Elias gamma are the two simplest universal codes. Unary encodes n in n bits; gamma in 2 log2(n)+1 bits. Each implies a different prior over the integers."
linked_project:
- pfc
- wire-formats
---

## Unary and Elias Gamma

Unary is the oldest code in this series. It predates information theory by centuries: a shepherd counting sheep on a stick is using unary. Mark one notch per sheep; count the notches to decode. The codeword for \(n\) is \(n\) tally marks. Its information-theoretic content came later, when Shannon showed it is exactly optimal for a geometric source.

Elias gamma is the 1975 extension by Peter Elias. It brings the codeword length from \(O(n)\) to \(O(\log n)\), making it practical for numbers beyond small single digits, while keeping the prefix-free property that makes self-delimiting streams possible.

Both codes are instances of the claim from [Universal Codes as Priors](/post/2022-01-priors-wire-formats/): every prefix-free code is a bet about the source. Unary bets on a geometric distribution with parameter \(1/2\). Gamma bets on a power-law distribution with exponent \(\approx 2\). This post implements both, derives their implied priors, and shows numerically what the bets mean.

---

## Unary: Geometric Prior

The encoding rule for unary is simple: to encode integer \(n \geq 1\), write \((n-1)\) zero bits followed by one `1` bit. The decoder reads bits until it sees the `1`; the number of bits read is the decoded value.

Examples: \(1 \to\) `1`, \(2 \to\) `01`, \(3 \to\) `001`, \(4 \to\) `0001`.

```cpp
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
```

**Length analysis.** The codeword for \(n\) has length \(n\). The Kraft sum is \(\sum_{n=1}^{\infty} 2^{-n} = 1\): unary saturates Kraft exactly. The implied prior is \(p_n = 2^{-n}\): a geometric distribution with parameter \(1/2\), where each value is half as likely as the previous.

**Optimality test.** Because the implied prior is dyadic (all probabilities are powers of \(1/2\)) and Kraft saturates, unary achieves entropy exactly on this prior. Numerically, for a 30-symbol truncation of geometric(1/2), the expected unary length equals the entropy to within the truncation tail (\(\approx 2^{-30}\)):

```cpp
const std::size_t K = 30;
auto lens = unary_lengths(K);
auto probs = priors::implied_prior(lens);  // geometric(1/2), normalized
double r = priors::redundancy(probs, lens);
// r is < 1e-6: unary is exactly optimal for its own implied prior.
```

The redundancy is not exactly zero because the probability mass beyond symbol 30 has been renormalized into the first 30 symbols, introducing a small correction. As \(K \to \infty\), the redundancy goes to zero.

---

## Elias Gamma: Power-Law Prior

Gamma's encoding rule has three parts: write the *length prefix* in unary, then write the *payload bits*.

Specifically, for \(n \geq 1\), let \(k = \lfloor \log_2 n \rfloor\) (the position of the leading bit). Then:

1. Write \(k\) zero bits (the unary encoding of the block index, minus the terminator).
2. Write a `1` bit (the unary terminator and implicit leading 1 of the value).
3. Write the \(k\) bits of \(n\) after the leading 1 bit, MSB first.

Examples: \(1 \to\) `1`, \(2 \to\) `010`, \(3 \to\) `011`, \(4 \to\) `00100`, \(5 \to\) `00101`, \(6 \to\) `00110`, \(7 \to\) `00111`, \(8 \to\) `0001000`.

```cpp
struct Gamma {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1 && "Gamma is undefined for n = 0");
        std::size_t k = std::bit_width(n) - 1;  // floor(log2(n))
        for (std::size_t i = 0; i < k; ++i) sink.write(false);
        sink.write(true);
        for (std::size_t i = k; i > 0; --i) {
            sink.write(((n >> (i - 1)) & 1u) != 0u);
        }
    }

    template<BitSource S>
    static value_type decode(S& source) {
        std::size_t k = 0;
        while (!source.read()) ++k;
        value_type n = 1;
        for (std::size_t i = 0; i < k; ++i) {
            n = (n << 1) | (source.read() ? value_type{1} : value_type{0});
        }
        return n;
    }
};
```

**Length analysis.** The codeword for \(n\) has length \(2\lfloor\log_2 n\rfloor + 1\). The block \([2^k, 2^{k+1})\) contains \(2^k\) symbols, each with length \(2k+1\). Each block contributes \(2^k \cdot 2^{-(2k+1)} = 2^{-(k+1)}\) to the Kraft sum. Summing: \(\sum_{k=0}^{\infty} 2^{-(k+1)} = 1\). Gamma saturates Kraft.

The implied prior is \(p_n = 2^{-(2\lfloor\log_2 n\rfloor + 1)}\). All symbols in a block get the same probability, which falls by a factor of 4 each time we move to the next block. For \(n\) near a power of 2, this approximates \(1/(2n^2)\). For \(n\) midway through a block, the approximation is within a factor of 2.

**Bounded redundancy test.** For a power-law(2) source (where \(p_n \propto 1/n^2\)), gamma's redundancy is bounded by a small constant regardless of how many symbols we consider:

```cpp
const std::size_t N = 128;
auto lens = gamma_lengths(N);
// Build power-law(2): p_n = C/n^2, normalized.
std::vector<double> pl(N);
double z = 0.0;
for (std::size_t i = 0; i < N; ++i) {
    double n = static_cast<double>(i + 1);
    pl[i] = 1.0 / (n * n);
    z += pl[i];
}
for (double& p : pl) p /= z;
double r = priors::redundancy(pl, lens);
// r is < 3.0: gamma has bounded redundancy on power-law(2) sources.
```

The measured redundancy for \(N=128\) is about 0.5 bits per symbol. This is the "bounded overhead" that makes gamma a universal code for the class of power-law(2) distributions.

---

## Length Characteristics: O(n) vs O(log n)

The two codes have fundamentally different length growth rates. Here is a comparison:

| \(n\) | Unary length | Gamma length |
|---|---|---|
| 1 | 1 | 1 |
| 2 | 2 | 3 |
| 4 | 4 | 5 |
| 8 | 8 | 7 |
| 16 | 16 | 9 |
| 100 | 100 | 13 |
| 1024 | 1024 | 21 |

The crossover happens at \(n = 3\): for \(n \leq 2\), unary is shorter or equal; for \(n \geq 3\), gamma eventually dominates. By \(n = 8\), gamma is already using 7 bits against unary's 8. By \(n = 100\), gamma is 87 bits shorter per codeword.

The asymptotic consequence: if your source puts even a small fraction of probability mass on values above, say, 50, the average cost of unary will be dominated by those rare large values. Gamma's logarithmic growth rate bounds the damage.

Both codes decode in time proportional to the codeword length, which is proportional to \(n\) for unary and \(\log n\) for gamma.

---

## When to Use Which

The implied-prior framing gives concrete guidance.

Use **unary** when your data is very strongly left-skewed toward 1. If 90% or more of your values are 1 or 2, unary's simplicity and exact optimality for geometric(1/2) make it the right choice. Many compressors use unary internally for length-of-run or tag-bit sequences, where most runs are length 1 or 2.

Use **Elias gamma** when values are left-skewed but the tail extends. Natural language word ranks, file sizes in a large corpus, and degree distributions in social networks all follow approximate power laws with exponents near 2. Gamma is a good default for any positive integer that has no known maximum and follows roughly the "larger values are rarer" pattern.

If you have enough data to estimate the source distribution precisely, neither code is the right answer: a Huffman code for the estimated distribution will do better. But Huffman requires knowing the distribution in advance. Gamma requires nothing: it works without any training data and achieves bounded redundancy on a large class of natural distributions.

---

## The Recursive Idea

Look at gamma's encoding again. The leading zero bits encode the block index \(k = \lfloor \log_2 n \rfloor\). That encoding is unary. The trailing bits encode the position of \(n\) within its block.

What if we encoded the block index in gamma instead of unary? We would use \(O(\log\log n)\) bits for the block index and \(\lfloor\log_2 n\rfloor\) bits for the position, giving a total of about \(\log_2 n + 2\log_2\log_2 n\) bits. This is Elias delta.

What if we encoded the block index of the block index? One more recursion gives Elias omega, with length about \(\log_2 n + \log_2\log_2 n + \log_2\log_2\log_2 n + \ldots\) bits (stopping at the first term that reaches 1).

Each recursion improves the asymptotic length for very large \(n\) at the cost of a constant overhead for small \(n\). The crossover point moves further and further out. For most practical applications (where values rarely exceed \(10^6\)), gamma's length of about \(2\log_2 n\) bits is already near-optimal; the recursive codes matter mainly for truly unbounded streams.

The recursive idea is developed in the next post, Elias Delta and Omega.

---

## Cross-references

The framing: [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3) introduces the length-to-prior correspondence that this post instantiates for unary (geometric prior) and gamma (power-law prior). Reading that post first is recommended.

The foundations: [Kraft's Inequality](/post/2020-03-kraft-wire-formats/) characterizes the space of achievable length vectors; both unary and gamma sit exactly on the Kraft boundary (Kraft sum = 1).

Cross-series: [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/) uses gamma as its running example for prefix-free length encoding when building self-delimiting integer sequences. The codec-as-functor perspective there complements the prior-theoretic view here.

---

*PFC's `include/pfc/codecs.hpp` contains both `Unary` and `EliasGamma` structs with the same encode/decode interface as above, wired into the full algebraic type system. This post develops the pedagogical version; the production code handles edge cases and integrates with bit-level I/O abstractions.*
