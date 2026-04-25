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

*Huffman codes one symbol at a time. Arithmetic coding encodes the whole sequence as a single number. The difference is a factor of twelve, at least on the right source.*

## The Last Bit of Redundancy

[Huffman coding](/post/2024-08-huffman-wire-formats/) gets expected codeword length within one bit of entropy. That is the best it can do, because codeword lengths must be integers while entropy is a real number.

The waste is structural. A symbol with probability $p = 0.7$ has optimal (fractional) length $-\log_2(0.7) \approx 0.515$ bits. Huffman rounds that up to 1 bit: 0.485 bits wasted per occurrence. For a nearly-deterministic source with $p_0 = 0.99$ and $p_1 = 0.01$, the entropy is $H \approx 0.081$ bits per symbol. Huffman is stuck at 1 bit per symbol. That is a factor-of-twelve gap, and Huffman cannot close it: a symbol that appears 99% of the time still gets a complete codeword.

Arithmetic coding steps back from per-symbol codewords entirely. It encodes an entire sequence as a single rational number in $[0, 1)$. The bit-length of that number converges to the entropy of the sequence as the sequence grows. No integer rounding, no per-symbol overhead.

This post builds an integer range coder in C++23 and demonstrates the factor-of-twelve improvement on the Bernoulli(0.99) source.

---

## The Continuous View

Start with the unit interval $[0, 1)$. For a two-symbol source, partition it by probability: symbol 0 gets $[0, p_0)$ and symbol 1 gets $[p_0, 1)$.

To encode a sequence, begin with the full interval and narrow it with each symbol. After symbol $s_1$, restrict to the corresponding sub-interval. After $s_2$, apply the same proportional rule inside that sub-interval. After $L$ symbols, the interval has width $\prod_{i=1}^{L} p_{s_i}$.

Any number inside the final interval is a valid encoding. The shortest such number in binary requires approximately $-\log_2(\prod p_{s_i}) = \sum_{i=1}^{L} (-\log_2 p_{s_i})$ bits. As $L \to \infty$, bits per symbol approaches $H(p) = -\sum_k p_k \log_2 p_k$ exactly.

Decoding is the inverse: given the encoded number, determine at each step which sub-interval it falls in, recover the symbol, narrow the interval, and repeat.

The theory is complete. The practice is not. A real interval narrows exponentially fast: after a few dozen symbols you need arbitrary precision. The integer range coder fixes this with 32-bit arithmetic and a renormalization step.

---

## The Integer Implementation

Real coders use 32-bit unsigned integers. Four boundary constants define the working range:

```cpp
constexpr std::uint32_t TOP_VALUE     = 0xFFFFFFFFu;
constexpr std::uint32_t HALF          = 0x80000000u;
constexpr std::uint32_t QUARTER       = 0x40000000u;
constexpr std::uint32_t THREE_QUARTER = 0xC0000000u;
```

The encoder maintains `low_` (lower bound), `high_` (upper bound), and `underflow_count_` (pending bits). Initially `low_ = 0` and `high_ = TOP_VALUE`, representing the full interval.

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

The intermediate product is 64-bit to avoid overflow before the division. After shrinking, `renormalize()` extracts any bits that are now settled.

The renormalization loop is where the fiddly part lives:

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

Three cases. When both bounds are below the midpoint, the high bit is 0: emit it and double both bounds. When both are above, the high bit is 1: subtract HALF, emit, double. After emitting a bit, the working range is restored. The third case is the tricky one.

When `low_ >= QUARTER` and `high_ < THREE_QUARTER`, the interval straddles the midpoint and is narrowing toward it. Neither branch applies: the high bit is not yet resolved. The encoder defers by incrementing `underflow_count_` and centering the interval. When the interval eventually escapes to one side, `emit_bit_and_underflow` emits the resolved bit followed by `underflow_count_` complementary bits:

```cpp
void emit_bit_and_underflow(bool bit) {
    sink_.write(bit);
    while (underflow_count_ > 0) {
        sink_.write(!bit);
        --underflow_count_;
    }
}
```

This is Elias-Gallager underflow correction. Without it, the encoder stalls whenever the interval converges toward $1/2$ without resolving to either half. It is not complicated in retrospect, but getting the invariants right in the first place requires care.

The decoder mirrors the encoder exactly. It maintains `low_`, `high_`, and a 32-bit `code_` register primed from the first 32 bits of the compressed stream. To decode a symbol, it scales `code_` into $[0, \text{total})$, looks up which cumulative interval it falls in, updates `low_` and `high_` identically to the encoder, then shifts in a new bit from the source:

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

Both sides apply identical interval arithmetic. They stay synchronized without any out-of-band length information.

---

## Tests and the Compelling Example

The test suite verifies round-trip correctness across distributions and sequence lengths. The number worth looking at is the Bernoulli(0.99) demo: 1000 symbols, each drawn independently with $P(\text{sym0}) = 0.99$ and $P(\text{sym1}) = 0.01$.

```
H(0.99, 0.01) = -(0.99 log2 0.99 + 0.01 log2 0.01)
              ≈ 0.081 bits/symbol
```

Huffman cannot compress a binary source below 1 bit per symbol. The arithmetic coder, after encoding 1000 symbols, emits approximately 82 bits total. Huffman needs 1000. The arithmetic coder gets better as the sequence grows; Huffman stays stuck.

The `BinarySourceDemoTest.Bernoulli99OneThousandSymbols` test confirms:

```
bits_per_symbol < 1.0      (beats Huffman's floor)
bits_per_symbol < H + 1.0  (within 1 bit/symbol of entropy)
```

and verifies lossless round-trip on the full 1000-symbol sequence.

---

## The Adaptive Variant

The encoder above uses a fixed probability model. Adaptive arithmetic coding generalizes to unknown or changing distributions by updating the cumulative-frequency table after each symbol. Both encoder and decoder apply identical updates in the same order, so they stay synchronized without any transmitted model.

The update is simple: after encoding or decoding symbol $s$, increment its frequency count by 1, update the cumulative totals, and rescale if the total exceeds a threshold. This is the semi-adaptive (online) model used in practice.

The structural advantage over adaptive Huffman is real. Adaptive Huffman requires rebalancing a tree after each symbol: $O(\log n)$ with non-trivial bookkeeping. Adaptive arithmetic coding only increments a count and recomputes cumulative sums: $O(\text{alphabet size})$, cache-friendly.

Production arithmetic coders in JPEG XL and AV1 use more sophisticated context models. The arithmetic stage itself is unchanged. The probability estimate feeding it becomes a function of recent history. The coder only needs a cumulative-frequency interval; how the model produces $p$ is not its concern.

---

## The Theoretical Endpoint

Shannon's source-coding theorem says that for a memoryless source with distribution $p$, no prefix-free code can achieve expected length below $H(p)$ bits per symbol. Arithmetic coding achieves $H(p)$ in the limit. The bound is tight, and arithmetic coding reaches it.

That settles compression for memoryless sources. Sources with memory are a different problem. A Markov source of order $k$ can be handled by conditioning on the last $k$ symbols: one arithmetic coder per context. Generalizing further, context-mixing predictors maintain an ensemble of models and blend their probability estimates. The arithmetic coding stage stays unchanged throughout: it only needs a good $p$.

The best general-purpose lossless compressors in use (PAQ, ZPAQ, and derivatives) are context-mixing predictors driving an arithmetic coder. The predictor is where the innovation happens. The coder is the fixed, information-theoretically optimal back end.

What arithmetic coding cannot do is compress below $H(p)$. That is not an implementation limitation. It is a theorem. The next tool in the series, Succinct Bit Vectors and Rank/Select, shifts from compressing data to indexing it space-efficiently: a different kind of optimality.

---

## Cross-references

**Back:** [Huffman Coding](/post/2024-08-huffman-wire-formats/) (post 9) showed that integer codeword lengths bound compression at one bit above entropy. Arithmetic coding removes that bound.

[Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3) introduced entropy and redundancy measurements; the `priors::entropy()` function used in the convergence tests comes from `priors.hpp` in that post's directory.

**Forward:** Succinct Bit Vectors and Rank/Select (post 11) shifts from entropy coding to space-efficient indexing, where the goal is not compression but constant-time rank and select queries on compressed representations.

**Cross-series:** In the Bits Follow Types framing, arithmetic coding is the entropy-optimal realization of the Either combinator's tag bit. The Either codec tags a choice with 1 bit regardless of probability; arithmetic coding replaces the tag with a fractional contribution proportional to the symbol's true information content.

**Footnote:** The production implementation lives at `include/pfc/arithmetic_coding.hpp` in the [PFC library](https://github.com/queelius/wire-formats/tree/master/lib/pfc). It includes both the integer range coder developed here and a higher-level adaptive variant with configurable context models.
