---
title: "Arithmetic Coding"
date: 2025-01-12
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- entropy
- arithmetic-coding
- range-coding
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 10
math: true
description: "Arithmetic coding closes the gap between Huffman's per-symbol integer lengths and true entropy. A single number in the unit interval encodes an entire sequence; 32-bit integer arithmetic makes it practical."
linked_project:
- pfc
- wire-formats
---

## The Last Bit of Redundancy

[Huffman coding](/post/2024-08-huffman-wire-formats/) achieves expected codeword length within one bit of entropy. That bound is tight: Huffman cannot do better in general because codeword lengths must be integers, while entropy is a real number.

Where does the slack come from? Consider a symbol with probability $p = 0.7$. Its optimal (fractional) codeword length is $-\log_2(0.7) \approx 0.515$ bits. Huffman rounds this up to 1 bit, wasting roughly 0.485 bits per occurrence. Over a long sequence the waste accumulates. For a nearly-deterministic source with $p_0 = 0.99$ and $p_1 = 0.01$, Huffman is stuck at 1 bit per symbol while true entropy is only $H \approx 0.081$ bits per symbol: a factor-of-twelve gap.

The waste is structural. Huffman assigns one codeword per symbol. A symbol that appears 99% of the time still gets a complete codeword, even though its information content is barely a tenth of a bit.

Arithmetic coding dissolves this constraint by stepping back from per-symbol codewords entirely. Instead of assigning a bit pattern to each symbol, it encodes an entire sequence as a single rational number in the unit interval $[0, 1)$. The length of that number, in bits, converges to the entropy of the sequence as the sequence grows. There is no rounding residual: fractional bits become real.

This post builds an integer range coder in C++23, step by step, and demonstrates the factor-of-twelve improvement on the Bernoulli(0.99) source that Huffman cannot touch.

---

## The Continuous View

Start with the unit interval $[0, 1)$. Partition it by symbol probabilities: symbol 0 occupies $[0, p_0)$ and symbol 1 occupies $[p_0, 1)$ for a two-symbol source.

To encode a sequence, begin with the full interval and iteratively restrict it. After seeing symbol $s_1$, narrow to the corresponding sub-interval. After seeing $s_2$, narrow the sub-interval by the same proportional rule. After $L$ symbols, the current interval has width exactly $\prod_{i=1}^{L} p_{s_i}$.

Any rational number inside this final interval is a valid encoding. The shortest such number, in binary, requires approximately $-\log_2(\prod p_{s_i}) = \sum_{i=1}^{L} (-\log_2 p_{s_i})$ bits. As $L \to \infty$, bits per symbol approaches $H(p) = -\sum_k p_k \log_2 p_k$ exactly. No rounding, no integer constraint.

Decoding is the inverse: given the encoded number, determine which sub-interval it falls in at each step, recover the symbol, narrow the interval, and repeat.

The elegance is complete but impractical as stated. A real interval narrows exponentially fast, requiring arbitrary precision after a few dozen symbols. The integer range coder replaces infinite precision with 32-bit arithmetic and a renormalization trick.

---

## The Integer Implementation

Real coders work with 32-bit unsigned integers. Define four boundary constants:

```cpp
constexpr std::uint32_t TOP_VALUE     = 0xFFFFFFFFu;
constexpr std::uint32_t HALF          = 0x80000000u;
constexpr std::uint32_t QUARTER       = 0x40000000u;
constexpr std::uint32_t THREE_QUARTER = 0xC0000000u;
```

The encoder maintains three state variables: `low_` (lower bound), `high_` (upper bound), and `underflow_count_` (pending bits). Initially `low_ = 0` and `high_ = TOP_VALUE`, representing the full interval.

Encoding symbol $s$ with cumulative frequency range $[\text{lo\_cum}, \text{hi\_cum})$ out of total $T$ shrinks the interval:

```cpp
void encode_symbol(std::uint32_t low_cum, std::uint32_t high_cum,
                   std::uint32_t total) {
    std::uint64_t range = static_cast<std::uint64_t>(high_) - low_ + 1;
    high_ = low_ + static_cast<std::uint32_t>((range * high_cum) / total - 1);
    low_  = low_ + static_cast<std::uint32_t>((range * low_cum)  / total);
    renormalize();
}
```

The intermediate product uses 64-bit arithmetic to avoid overflow before the division. After shrinking, `renormalize()` extracts any bits that are now determined.

The renormalization loop handles three cases:

```cpp
void renormalize() {
    while (true) {
        if (high_ < HALF) {
            // Both bounds below the midpoint: the high bit is 0.
            emit_bit_and_underflow(false);
        } else if (low_ >= HALF) {
            // Both bounds above the midpoint: the high bit is 1.
            emit_bit_and_underflow(true);
            low_  -= HALF;
            high_ -= HALF;
        } else if (low_ >= QUARTER && high_ < THREE_QUARTER) {
            // Underflow: interval straddles the midpoint and is shrinking
            // toward it. Neither high bit is agreed. Count the pending bit.
            ++underflow_count_;
            low_  -= QUARTER;
            high_ -= QUARTER;
        } else {
            break;
        }
        low_  <<= 1;
        high_ = (high_ << 1) | 1u;
    }
}
```

After emitting a bit, both bounds are doubled (shifted left by one), restoring the working range. The underflow case requires special care: when `low_ >= QUARTER` and `high_ < THREE_QUARTER`, neither the 0 nor the 1 branch applies, yet the interval is narrowing toward the midpoint. The encoder defers these bits by incrementing `underflow_count_`. When the interval eventually escapes to one side, `emit_bit_and_underflow` emits the real bit followed by `underflow_count_` complementary bits:

```cpp
void emit_bit_and_underflow(bool bit) {
    sink_.write(bit);
    while (underflow_count_ > 0) {
        sink_.write(!bit);
        --underflow_count_;
    }
}
```

This is the Elias-Gallager underflow correction. Without it, the encoder would stall whenever the interval converges toward $1/2$ without resolving to either half.

The decoder mirrors the encoder exactly. It maintains `low_`, `high_`, and a 32-bit `code_` register primed from the first 32 bits of the compressed stream. Decoding a symbol scales `code_` into $[0, \text{total})$, looks up which cumulative interval it falls in, updates `low_` and `high_` identically to the encoder, then shifts in a new bit from the source:

```cpp
template <typename FreqCb, typename RangeCb>
std::size_t decode_symbol(FreqCb&& get_freq_cb, RangeCb&& cum_range_cb,
                          std::uint32_t total) {
    std::uint64_t range  = static_cast<std::uint64_t>(high_) - low_ + 1;
    std::uint32_t scaled = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(code_ - low_) * total) / range);

    std::size_t sym = get_freq_cb(scaled);
    auto [lo_cum, hi_cum] = cum_range_cb(sym);

    high_ = low_ + static_cast<std::uint32_t>((range * hi_cum) / total - 1);
    low_  = low_ + static_cast<std::uint32_t>((range * lo_cum) / total);
    decoder_renormalize();
    return sym;
}
```

The decoder's renormalize reads one new bit from the source each time it shifts, keeping `code_` synchronized with the encoder's bit stream. Because both sides apply identical interval arithmetic, they stay in lock-step without any out-of-band length information.

---

## Tests and the Compelling Example

The test suite verifies round-trip correctness across a range of distributions and sequence lengths. The critical test is the Bernoulli(0.99) demo: 1000 symbols, each independently drawn with $P(\text{sym0}) = 0.99$, $P(\text{sym1}) = 0.01$.

```
H(0.99, 0.01) = -(0.99 log2 0.99 + 0.01 log2 0.01)
              ≈ 0.081 bits/symbol
```

Huffman cannot compress a binary source below 1 bit per symbol. The arithmetic coder, after encoding 1000 symbols, emits approximately 82 bits total. That is a factor-of-twelve improvement, and the compression continues to improve as the sequence grows.

The `BinarySourceDemoTest.Bernoulli99OneThousandSymbols` test confirms:

```
bits_per_symbol < 1.0      (beats Huffman's floor)
bits_per_symbol < H + 1.0  (within 1 bit/symbol of entropy)
```

and verifies lossless round-trip on the full 1000-symbol sequence.

---

## The Adaptive Variant

The encoder above uses a fixed probability model. Adaptive arithmetic coding generalizes to unknown or changing distributions by updating the cumulative-frequency table after each symbol. Both encoder and decoder apply identical updates in the same order, so they remain synchronized without any transmitted model.

The update rule is simple: after encoding or decoding symbol $s$, increment its frequency count by 1, update the cumulative totals, and rescale if the total exceeds a threshold. This is the "semi-adaptive" or "online" model used in practice.

The advantage over adaptive Huffman coding is structural. Adaptive Huffman requires rebalancing a tree after each symbol update, an $O(\log n)$ operation with non-trivial bookkeeping. Adaptive arithmetic coding only increments a count and recomputes cumulative sums, which is $O(\text{alphabet size})$ and cache-friendly.

Production arithmetic coders in JPEG XL and AV1 use more sophisticated context models. The arithmetic stage itself is unchanged; the probability estimate feeding it becomes a function of recent history. The coder does not care how the model produces $p$: it only needs a cumulative-frequency interval.

---

## The Theoretical Endpoint

Shannon's source-coding theorem states that for a memoryless source with distribution $p$, no prefix-free code can achieve expected length less than $H(p)$ bits per symbol. Arithmetic coding achieves $H(p)$ in the limit: the bound is tight, and arithmetic coding reaches it.

This settles compression for memoryless sources. The remaining frontier is sources with memory: symbols that depend on context. A Markov source of order $k$ can be handled by conditioning on the last $k$ symbols; a separate arithmetic coder runs for each context. Generalizing further, "context mixing" predictors maintain a large ensemble of models and blend their probability estimates. The arithmetic coding stage remains unchanged: it only needs a good $p$.

The best general-purpose lossless compressors in use today (PAQ, ZPAQ, and derivatives) are context-mixing predictors driving an arithmetic coder. The predictor is the innovation; the coder is the fixed, information-theoretically optimal back end.

What arithmetic coding cannot do is compress below $H(p)$. That is not a limitation of the implementation: it is a theorem. The next tool in the series, Succinct Bit Vectors and Rank/Select, shifts from compressing data to indexing it space-efficiently: a different kind of optimality.

---

## Cross-references

**Back:** [Huffman Coding](/post/2024-08-huffman-wire-formats/) (post 9) showed that integer codeword lengths bound compression at one bit above entropy. Arithmetic coding removes that bound.

[Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3) introduced entropy and redundancy measurements; the `priors::entropy()` function used in the convergence tests comes from `priors.hpp` in that post's directory.

**Forward:** Succinct Bit Vectors and Rank/Select (post 11) shifts from entropy coding to space-efficient indexing, where the goal is not compression but constant-time rank and select queries on compressed representations.

**Cross-series:** In the Bits Follow Types framing, arithmetic coding is the entropy-optimal realization of the Either combinator's tag bit. The Either codec tags a choice with 1 bit regardless of probability; arithmetic coding replaces the tag with a fractional contribution proportional to the symbol's true information content.

**Footnote:** The production implementation lives at `include/pfc/arithmetic_coding.hpp` in the [PFC library](https://github.com/queelius/pfc). It includes both the integer range coder developed here and a higher-level adaptive variant with configurable context models.
