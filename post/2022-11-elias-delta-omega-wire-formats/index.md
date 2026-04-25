---
title: "Elias Delta and Omega"
date: 2022-11-13
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- elias-delta
- elias-omega
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 5
math: true
description: "Elias delta and omega extend Elias gamma by recursively encoding the length prefix. Each step yields shorter codewords for large integers at a small constant cost for small ones."
linked_project:
- pfc
- wire-formats
---

*Elias gamma spends too many bits saying how many bits it will use. Delta fixes that. Omega takes the fix one step further. This post is about what happens when you apply recursion to the length prefix.*

## Where Gamma Stops Being Good

Elias gamma, from [the previous post](/post/2022-06-elias-gamma-wire-formats/), encodes a positive integer $n$ in $2\lfloor \log_2 n \rfloor + 1$ bits: a unary count of $\lfloor \log_2 n \rfloor$ zeros, then a stop bit, then the $\lfloor \log_2 n \rfloor$ trailing binary bits of $n$. For small $n$ this is fine. For large $n$, nearly half the bits are spent on the unary prefix alone.

The unary prefix is the bottleneck. It encodes the length $L = \lfloor \log_2 n \rfloor + 1$ in the most wasteful possible way: one bit per unit. For $n = 256$, that is 8 zero bits just to say "the payload is 8 bits long." The payload itself is also 8 bits, so you are paying a 100% overhead on the length announcement. That is bad, and it gets worse as $n$ grows.

The fix is obvious once you see it: encode $L$ itself in some shorter code instead of unary. Elias delta does exactly this, replacing the unary length prefix with a gamma-coded length. Elias omega takes the idea one step further and applies the recursion to itself, all the way down.

Both codes are universal: they assign finite codewords to every positive integer, and the expected codeword length is within a constant factor of optimal for any source whose probabilities decrease with $n$. The improvement over gamma is real and measurable once $n$ grows past a few dozen.

This post shows both implementations, their implied priors, and the crossover points where each code wins. As in the rest of this series, the code is pedagogical: each header stands alone and the struct-with-encode/decode pattern maps directly onto the [PFC library](https://github.com/queelius/pfc)'s `EliasDelta` and `EliasOmega` in `codecs.hpp`.

---

## Elias Delta

**Algorithm.** Let $L = \lfloor \log_2 n \rfloor + 1$ (the bit-width of $n$, equivalently `std::bit_width(n)`).

1. Encode $L$ in Elias gamma.
2. Write the $L - 1$ trailing bits of $n$ after its implicit leading 1, MSB first.

Gamma encodes $L$ (a small integer) in $O(\log \log n)$ bits instead of $O(\log n)$ bits for the unary prefix. The payload is identical to gamma's: the trailing bits of $n$. The total length is $O(\log n + \log \log n)$.

**Examples.** Let us trace $n \in \{1, 2, 3, 4\}$:

| $n$ | $L = \text{bit\_width}(n)$ | $\gamma(L)$ | Trailing bits | Delta codeword |
|-----|-----------------------------|-------------|---------------|----------------|
| 1   | 1                           | `1`         | (none)        | `1`            |
| 2   | 2                           | `010`       | `0`           | `0100`         |
| 3   | 2                           | `010`       | `1`           | `0101`         |
| 4   | 3                           | `011`       | `00`          | `01100`        |

For $n = 1$, the two codes agree: both emit a single `1` bit. For $n = 4$, gamma would emit `00100` (5 bits); delta emits `01100` (5 bits as well, but it will pull ahead soon). The crossover is not immediate, but it is real.

**Implementation.**

```cpp
struct Delta {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1);
        std::size_t bits = std::bit_width(n);  // L = floor(log2(n)) + 1
        Gamma::encode(static_cast<value_type>(bits), sink);
        // Write the (bits - 1) trailing bits of n, MSB first, skipping the
        // implicit leading 1.
        for (std::size_t i = bits - 1; i > 0; --i) {
            sink.write(((n >> (i - 1)) & 1u) != 0u);
        }
    }

    template<BitSource S>
    static value_type decode(S& source) {
        std::size_t bits = static_cast<std::size_t>(Gamma::decode(source));
        value_type result = 1;
        for (std::size_t i = 1; i < bits; ++i) {
            result = (result << 1) | (source.read() ? value_type{1} : value_type{0});
        }
        return result;
    }
};
```

Decoding inverts exactly: read $L$ from a gamma-coded prefix, then read $L - 1$ payload bits, prepending the implicit leading 1. The structure mirrors the encoder step for step. There is nothing subtle here; the recursion is one level deep and terminates immediately.

**Implied prior.** The delta length for $n$ is approximately $\log_2 n + 2 \log_2 \log_2 n + C$ bits. The implied probability is roughly $1 / (n \log^2 n)$, a heavier tail than gamma's $\sim 1/n^2$. Delta is better matched to sources where large values are somewhat more likely than a squared power law would predict.

---

## Elias Omega

**Algorithm.** Omega applies the recursion all the way down:

1. While $n > 1$:
   a. Record $(n, \text{bit\_width}(n))$ on a stack.
   b. Replace $n$ with $\text{bit\_width}(n) - 1$.
2. Write the stacked values from outermost to innermost, each in its full binary representation.
3. Append a terminating 0 bit.

Decoding reads forward: start with `current = 1`. At each step, read the next bit. If it is 0, stop and return `current`. If it is 1, it is the MSB of a `(current + 1)`-bit value; read the remaining `current` bits, form the value, set `current` to it, and repeat.

**Implementation.**

```cpp
struct Omega {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1);
        std::vector<std::pair<value_type, std::size_t>> stack;
        while (n > 1) {
            std::size_t w = static_cast<std::size_t>(std::bit_width(n));
            stack.emplace_back(n, w);
            n = static_cast<value_type>(w - 1);
        }
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            auto [val, width] = *it;
            for (std::size_t i = width; i > 0; --i) {
                sink.write(((val >> (i - 1)) & 1u) != 0u);
            }
        }
        sink.write(false);  // terminating 0 bit
    }

    template<BitSource S>
    static value_type decode(S& source) {
        value_type current = 1;
        while (true) {
            bool b = source.read();
            if (!b) break;
            value_type next = 1;
            for (value_type i = 0; i < current; ++i) {
                next = (next << 1) | (source.read() ? value_type{1} : value_type{0});
            }
            current = next;
        }
        return current;
    }
};
```

The stack-based encoding and forward-reading decode are mirror images of each other. The terminating 0 bit is the base case of the recursion: it tells the decoder there is nothing more to unfold.

**Implied prior.** Omega's length grows as $O(\log^* n)$, the iterated logarithm. This is the theoretical limit for self-delimiting integer codes. The implied probability is slightly heavier-tailed than delta's $1/(n \log^2 n)$.

---

## The Crossover Points

Where does each code win?

| $n$     | Unary | Gamma | Delta | Omega |
|---------|-------|-------|-------|-------|
| 1       | 1     | 1     | 1     | 1     |
| 4       | 4     | 5     | 5     | 6     |
| 16      | 16    | 9     | 9     | 11    |
| 256     | 256   | 17    | 15    | 16    |
| $2^{16}$| 65536 | 33    | 25    | 28    |
| $2^{32}$| ~4B   | 65    | 43    | 45    |

For small $n$ (say, $n \leq 8$), gamma beats delta. Both codes use 1 bit for $n=1$; delta is not shorter until $n$ grows to the point where the gamma-coded length prefix pays for itself. That crossover occurs around $n = 16$ for delta versus gamma (they tie there; delta pulls ahead from $n = 32$ onward).

Omega has more overhead at small $n$ because its recursive structure always adds at least one extra bit beyond delta. Omega beats delta only for very large $n$: roughly $n > 1000$. For any $n$ that fits in 64 bits (up to about $1.8 \times 10^{19}$), the iterated logarithm reaches at most 4 or 5 levels, so the practical advantage of omega over delta is small.

The takeaway: delta is the default choice for most practical work. Omega is interesting as a theoretical endpoint.

---

## The Implied Prior Ladder

Each step in the encoding hierarchy shifts the implied prior toward a heavier tail:

- **Unary**: implied prior $\sim 1/2^n$ (geometric, extremely light tail).
- **Gamma**: implied prior $\sim 1/n^2$ (power law with exponent 2).
- **Delta**: implied prior $\sim 1/(n \log^2 n)$ (power law with a logarithmic correction).
- **Omega**: implied prior slightly heavier than delta's.

A heavier tail means the code assigns shorter codewords to large $n$ relative to what a pure power law would give. This reduces average code length when the source occasionally produces large outliers. The cost is a small constant overhead for the small values that make up the bulk of typical data.

If your source is well-modeled by $1/n^2$, gamma is the best match. If large values appear more often than $1/n^2$ predicts (heavier tail), delta is better. Omega is useful primarily as a theoretical reference: the code that matches the heaviest practically encodable tail.

Choosing a code is choosing a prior. You should at least know what prior you are choosing.

The [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) post gives the machinery for computing redundancy given a source distribution and a code length sequence. The integration tests in `test_elias_delta_omega.cpp` use that library to verify that delta's redundancy on a $1/n^2$ source stays bounded, and that gamma beats delta on exactly that source.

---

## The Limit

Omega achieves $O(\log^* n)$ codeword length, where $\log^*$ is the iterated logarithm: the number of times you must apply $\log_2$ before the result drops to 1 or below. For all 64-bit integers, $\log^* n \leq 5$.

This is the theoretical lower bound for self-delimiting codes on positive integers with no distributional assumptions. No prefix-free code can do asymptotically better than $O(\log^* n)$.

In practice, the constant in that $O$ matters. Omega's codewords for $n \leq 1000$ are frequently longer than delta's. The overhead of adding one more recursion level costs bits even when the recursion only saves bits asymptotically. For any practical $n$ in the range a 64-bit system can produce, delta already achieves most of omega's compression advantage at lower constant overhead.

So omega is the theoretical ceiling, delta is the practical ceiling, and gamma is the workhorse for distributions where large values are rare. Each code is the right choice for a different source model. That is not a deficiency of any of them; it is the point.

---

## Cross-References and Notes

**Forward:** [Fibonacci Coding](/post/2023-04-fibonacci-wire-formats/) (post 6) takes a completely different approach: it uses Zeckendorf's theorem rather than recursive length encoding, and produces self-synchronizing codewords with an interesting error-locality property.

**Back:** [Unary and Elias Gamma](/post/2022-06-elias-gamma-wire-formats/) (post 4) covers the base case of this recursion: the gamma code that delta and omega build on. [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3) gives the prior-redundancy framework used in the integration tests.

**Cross-series:** The Stepanov bridge posts ([Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) and [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/)) show how these codecs become functors over algebraic types, connecting the universal-code theory to the type-algebra layer.

**Production code:** `include/pfc/codecs.hpp` in the [PFC library](https://github.com/queelius/pfc) contains `EliasDelta` and `EliasOmega`, production implementations with the same encode/decode interface but zero-copy byte-buffer I/O.
