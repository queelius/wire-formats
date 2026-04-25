---
title: "Rice / Golomb"
date: 2023-09-17
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- rice
- golomb
- geometric-distribution
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 7
math: true
description: "Rice and Golomb codes are parametric: a single parameter k (or m) tunes the code to a specific geometric distribution. Choosing k is choosing your prior precisely."
linked_project:
- pfc
- wire-formats
---

## The First Parametric Code

Every code examined so far in this series has been monolithic. Unary coding is just unary coding. Elias gamma is just Elias gamma. Each one encodes all non-negative integers with a single fixed strategy. You do not get to choose anything about the code beyond whether to use it.

Rice and Golomb codes break this pattern. They are *parametric*: a single integer parameter, $k$ for Rice or $m$ for Golomb, tunes the code to a specific source distribution. Rice$(k)$ is not one code but a family of codes, one per value of $k$. Each member of the family is optimal for a specific geometric distribution. Choosing $k$ is choosing your prior precisely.

This matters because data sources are rarely uniform. Run-length encodings, inter-frame video differences, and the gap sequences in inverted indexes are all approximately geometrically distributed. If you know the mean of your source, you can pick $k$ so that Rice$(k)$ performs near-optimally, without the overhead of a Huffman table or arithmetic coding.

The key insight: for a geometric source with mean approximately $2^k$, Rice$(k)$ is within a small constant of entropy. No other universal code discussed in this series achieves this: Elias gamma and delta codes perform well asymptotically but can be far from optimal for a specific geometric distribution with a known mean.

---

## Rice Coding

Rice coding splits a non-negative integer $n$ into two parts: a quotient $q = \lfloor n / 2^k \rfloor = n \gg k$ and a remainder $r = n \bmod 2^k = n \mathbin{\&} (2^k - 1)$.

The quotient is encoded in unary: $q$ zero bits followed by a stop bit of `1`. The remainder is encoded in exactly $k$ bits, MSB first. The total codeword is the concatenation of these two parts.

Codeword examples for $k = 2$ (remainder is always 2 bits):

| $n$ | $q$ | $r$ | Codeword | Bits |
|-----|-----|-----|----------|------|
| 0   | 0   | 0   | `1 00`   | 3    |
| 1   | 0   | 1   | `1 01`   | 3    |
| 2   | 0   | 2   | `1 10`   | 3    |
| 3   | 0   | 3   | `1 11`   | 3    |
| 4   | 1   | 0   | `0 1 00` | 4    |
| 5   | 1   | 1   | `0 1 01` | 4    |

Codeword length: $(n \gg k) + 1 + k$ bits. The Kraft sum saturates to 1, so Rice is a complete prefix-free code.

The implied prior is geometric: $P(n) \propto (2^k / (2^k + 1))^{n/2^k}$. For a geometric source with mean $\mu$, the optimal $k$ satisfies $k \approx \log_2(\mu)$.

```cpp
template<std::size_t K>
struct Rice {
    using value_type = std::uint64_t;
    static_assert(K > 0 && K < 64, "K must be in [1, 63]");

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        std::uint64_t q = n >> K;
        std::uint64_t r = n & ((std::uint64_t{1} << K) - 1);
        for (std::uint64_t i = 0; i < q; ++i) sink.write(false);
        sink.write(true);
        for (std::size_t i = 0; i < K; ++i)
            sink.write(((r >> (K - 1 - i)) & 1) != 0);
    }

    template<BitSource S>
    static value_type decode(S& source) {
        std::uint64_t q = 0;
        while (!source.read()) ++q;
        std::uint64_t r = 0;
        for (std::size_t i = 0; i < K; ++i)
            r = (r << 1) | (source.read() ? std::uint64_t{1} : std::uint64_t{0});
        return (q << K) | r;
    }
};
```

The decode mirrors the encode exactly: count zeros for $q$, read $k$ bits for $r$, reconstruct $n = (q \ll k) \mid r$.

---

## Golomb Coding

Rice coding requires $m = 2^k$ (a power-of-two divisor). For sources where the optimal mean is not a power of two, this forces a choice: round $k$ down (underfit) or round $k$ up (overfit). Golomb coding removes this restriction by allowing any positive integer $m$ as the divisor.

The quotient part is the same: $q = \lfloor n / m \rfloor$ zero bits, then a stop `1`. The remainder $r = n \bmod m$ is encoded in *truncated binary* rather than a fixed-width field.

Truncated binary encodes values in $[0, m)$ using the minimal-length prefix-free code. Let $b = \lceil \log_2 m \rceil$ and $c = 2^b - m$ (the "cutoff"). Values $r < c$ are encoded in $b - 1$ bits; values $r \geq c$ are encoded in $b$ bits as $r + c$.

For $m = 5$: $b = 3$, $c = 3$. Values 0, 1, 2 use 2 bits; values 3, 4 use 3 bits. The codewords:

| $n$ | $q$ | $r$ | Codeword | Bits |
|-----|-----|-----|----------|------|
| 0   | 0   | 0   | `1 00`   | 3    |
| 1   | 0   | 1   | `1 01`   | 3    |
| 2   | 0   | 2   | `1 10`   | 3    |
| 3   | 0   | 3   | `1 110`  | 4    |
| 4   | 0   | 4   | `1 111`  | 4    |
| 5   | 1   | 0   | `01 00`  | 4    |

When $m$ is a power of two, $c = 0$ and truncated binary reduces to fixed-width $k$-bit encoding, making Golomb$(m)$ identical to Rice$(k)$ where $k = \log_2 m$.

```cpp
template<std::size_t M>
struct Golomb {
    using value_type = std::uint64_t;
    static_assert(M >= 1, "M must be at least 1");

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        std::uint64_t q = n / static_cast<std::uint64_t>(M);
        std::uint64_t r = n % static_cast<std::uint64_t>(M);
        for (std::uint64_t i = 0; i < q; ++i) sink.write(false);
        sink.write(true);
        detail::truncated_binary_encode(r, M, sink);
    }

    template<BitSource S>
    static value_type decode(S& source) {
        std::uint64_t q = 0;
        while (!source.read()) ++q;
        std::uint64_t r = detail::truncated_binary_decode(M, source);
        return q * static_cast<std::uint64_t>(M) + r;
    }
};
```

---

## The Parameter Selection

Knowing the code is not enough: you need to pick $k$ or $m$. Gallager and van Voorhis (1975) derived the optimal Golomb parameter for a geometric source.

For a geometric distribution over non-negative integers with mean $\mu$ (so $P(n) = (1-p)^n p$ with $p = 1/\mu$), the optimal Golomb parameter is:

$$m^* = \left\lceil \frac{-1}{\log_2\!\left(\frac{\mu - 1}{\mu}\right)} \right\rceil$$

For Rice, which requires $m = 2^k$, the optimal $k$ is simply $k \approx \log_2(\mu)$:

$$k^* = \text{round}(\log_2 \mu), \quad k \in [1, 62]$$

```cpp
inline std::size_t optimal_golomb_m(double mean) {
    double p = (mean - 1.0) / mean;
    double m_star = -1.0 / std::log2(p);
    std::size_t m = static_cast<std::size_t>(std::round(m_star));
    return std::max(m, std::size_t{1});
}

inline std::size_t optimal_rice_k(double mean) {
    if (mean <= 2.0) return 1;
    double k_real = std::log2(mean);
    std::size_t k = static_cast<std::size_t>(std::max(1.0, std::round(k_real)));
    return std::min(k, std::size_t{62});
}
```

The optimality property is verifiable: on a geometric source with mean $\approx 2^3 = 8$, Rice$\langle 3 \rangle$ has much lower redundancy than Rice$\langle 1 \rangle$. The redundancy test in the companion test suite (using the priors library from [post 3](/post/2022-01-priors-wire-formats/)) confirms this: `r3 < r1` by a wide margin, around 2 bits vs 10 bits of expected excess.

The takeaway: knowing the data's mean lets you pick a near-optimal code, and the selection formula is a one-liner.

---

## Use Cases

Rice coding is the standard for lossless audio compression. FLAC (Free Lossless Audio Codec) encodes the residuals of a linear prediction filter using Rice codes. Audio residuals, after prediction, are approximately geometrically distributed with a mean that can be estimated from recent samples. FLAC selects $k$ adaptively per frame, achieving near-optimal compression with simple, fast decode.

Golomb coding appears in image compression. JPEG-LS (the lossless variant of JPEG) uses Golomb codes for its residuals. The non-power-of-2 divisors let JPEG-LS adapt more precisely to the local statistics of image data, squeezing out the small efficiency loss from rounding $m$ to a power of two.

Bitmap formats and run-length encodings also use Golomb codes. A run of identical pixels has a geometrically distributed length (under standard assumptions), and Golomb codes match this distribution exactly.

The pattern: when your data is genuinely geometrically distributed and you can estimate the mean, Rice or Golomb beats every other universal code discussed in this series. The estimating-the-mean step is the engineering work. Once you have the mean, the code selection is trivial.

---

## The Connection to Huffman

Rice and Golomb are parametric codes: given $k$ or $m$, the codewords are determined by a formula. Huffman coding, which we will examine in post 9, is a *constructed* code: given the source probabilities, Huffman builds an optimal prefix-free code by a greedy tree construction.

For a geometric source with the optimal $k$, Rice coding is asymptotically as efficient as Huffman coding on that source. Rice has a small constant overhead (at most 1 bit above entropy per symbol) because it cannot achieve the exact codeword lengths that Huffman can. But Rice does not require building a tree at run time, transmitting the tree to the decoder, or storing the probability model. Rice decode is a single shift, mask, and unary scan. Huffman decode is a tree traversal.

The trade is real: Huffman is slightly more efficient on average, Rice is simpler and faster. For sources that are approximately geometric (which covers a wide swath of practical data), Rice is the better engineering choice. For sources with arbitrary distributions, you need Huffman (or arithmetic coding, which we examine later in the series).

Forthcoming: post 9 covers Huffman coding.

---

## Cross-references

**Next in series:** [VByte / Varint](/post/2024-02-vbyte-wire-formats/) (post 8), which trades bit-level precision for byte-alignment. VByte is the workhorse of production columnar databases, not because it is theoretically optimal but because byte operations are fast.

**Earlier in series:**
- [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3): the framework for measuring how well a code matches a source, including the redundancy metric used in the optimality tests here.
- [Unary and Elias Gamma](/post/2022-06-elias-gamma-wire-formats/) (post 4): the unary coding underlying the quotient part of Rice/Golomb.
- [Elias Delta and Omega](/post/2022-11-elias-delta-omega-wire-formats/) (post 5): bit-length-based universal codes.
- [Fibonacci Coding](/post/2023-04-fibonacci-wire-formats/) (post 6): the Zeckendorf representation approach to self-delimiting codes.

**Cross-series links:** The Stepanov bridge posts ([Codecs as Functors](/post/2026-05-codecs-functors-stepanov/) and [Prefix-Free Codes and Generic Programming](/post/2026-05-prefix-free-stepanov/)) connect this parametric design to the algebraic type-codec story. The parameter $k$ in Rice is conceptually the "tag-bit-width" choice in the `Either` sum-type combinator: just as `Either<L, R>` allocates a fixed number of tag bits to distinguish the two cases, Rice allocates $k$ bits to the fixed-width remainder field. The quotient field is the "which slot" selector, the remainder is the "value within the slot."

**Production code:** The `Rice<K>` and `Golomb<M>` structs in this post correspond directly to the production implementations in [PFC](https://github.com/queelius/pfc): see `include/pfc/codecs.hpp`. The production version adds fixed-size queries, concept constraints, and integration with the `BitReader`/`BitWriter` infrastructure from `core.hpp`. The pedagogical versions here are self-contained and use the same algorithmic logic.
