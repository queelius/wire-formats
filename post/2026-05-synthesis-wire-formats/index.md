---
title: "Synthesis: Codecs as Structure"
date: 2026-05-15
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- synthesis
- algebra
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 13
math: true
description: "The series closes by restating the codes-as-priors thesis across all twelve instances and connecting the wire-format side to the Stepanov type-algebra side."
linked_project:
- pfc
- wire-formats
---
*Twelve posts, twelve codes, one thesis that refused to change. This is the closing summary.*

## A. The Twelve Codes Together

Every post in this series answered a version of the same question: given a source of positive integers, how do you represent its values compactly as a sequence of bits? The answers differ in shape, in assumptions, and in which distribution each code implicitly expects.

| Post | Code | Implied prior (one phrase) |
|------|------|---------------------------|
| 1-2 | Foundations | Prefix-free codes are possible iff Kraft's inequality holds |
| 3 | Priors framework | Any code defines a prior; the best code matches the source |
| 4 | Unary | Geometric(1/2): value 1 is twice as likely as value 2, etc. |
| 5a | Elias Gamma | Power-law: probability falls as 1/n^2 |
| 5b | Elias Delta | Heavier-tailed power law: slower decay for large values |
| 5c | Elias Omega | Recursive structure: no fixed polynomial decay rate |
| 6 | Fibonacci | Near-geometric with Zeckendorf structure; good for Zeckendorf-sparse integers |
| 7 | Rice / Golomb | Geometric with known parameter m; optimal when m divides entropy |
| 8 | VByte | Roughly uniform over byte-aligned ranges; engineering favorite |
| 9 | Huffman | Source-optimal given the exact symbol distribution |
| 10 | Arithmetic coding | Approaches entropy to an arbitrary fraction of a bit |
| 11 | Succinct bit vectors | Not a code for integers: a representation that answers rank/select queries |
| 12 | RoaringBitmap | Polyalgorithm: picks array, bitset, or run-length per container chunk |

Posts 1 and 2 ([Kraft's Inequality](/post/2020-03-kraft-wire-formats/) and [McMillan's Converse](/post/2020-09-mcmillan-wire-formats/)) established why prefix-free codes are the right unit of analysis. Post 3 ([Universal Codes as Priors](/post/2022-01-priors-wire-formats/)) named the frame: a code is a hypothesis about the source. Posts 4 through 10 filled in the catalogue. Posts 11 and 12 extended from integer coding to set representation, where the questions shift from "how long is this codeword?" to "how do you store membership?" and "how do you answer rank/select?"

Looking across all twelve, the main lesson is not that one code dominates. It is that the question "which code?" is always empirically answerable given a sample.

## B. The Unifying Frame Restated

[Post 3](/post/2022-01-priors-wire-formats/) introduced the codes-as-priors thesis with two instances behind it. We now have twelve. The thesis has not changed; it has only become more evidently true.

Here is the six-clause version.

**Clause 1: a code is a hypothesis.** Every prefix-free code for positive integers assigns a codeword length $\ell(n)$ to each value $n$. By Kraft's equality for complete codes, the implied probability is $p(n) = 2^{-\ell(n)}$. A code is not neutral: it says something about how likely each value is.

**Clause 2: lengths determine a prior.** If you choose Elias gamma, you are implicitly betting that values are power-law distributed with exponent 2. If you choose Unary, you are betting on geometric(1/2). If you choose VByte, you are betting on a near-uniform distribution over byte-aligned ranges. The bet is in the lengths, whether or not you named it as a bet.

**Clause 3: choosing a code is choosing a prior.** This is just clauses 1 and 2 combined, but it bears stating as a unit: the act of picking a codec is a modeling decision, not an engineering neutral choice. Unary and Delta will produce radically different wire sizes on the same data, because they assume radically different sources.

**Clause 4: the best code matches the actual source.** The code that minimizes expected description length for a source $p$ is the one whose implied prior most closely matches $p$. Formally, expected length under code $c$ for source $p$ equals entropy $H(p)$ plus the KL divergence $D_\text{KL}(p \,\|\, q_c)$ where $q_c$ is the code's implied prior. Minimum expected length occurs at $D_\text{KL} = 0$, i.e., when the code and source agree.

**Clause 5: universal codes give bounded redundancy when the source is unknown.** Elias gamma and delta are universal: for any source with a finite entropy upper-bound, their redundancy per symbol is $O(\log \log n)$ relative to the optimal code for that source. You pay a small overhead to avoid committing to a specific prior. The overhead is worth paying when you cannot measure the source in advance.

**Clause 6: polyalgorithms adapt per-chunk rather than committing globally.** RoaringBitmap is the clearest instance of this: it measures each 65536-element chunk and picks the container type (sorted array, bitset, run-length) that minimizes storage for that chunk's observed density. This is not "pick the best universal code" but "pick the best structure from a menu, per block." The principle generalizes: any large dataset may be non-stationary, and a single global codec will be suboptimal for every region where the source differs from the global average.

## C. Composition with Type Algebra

Two posts in the companion Stepanov series made claims that interlock with everything above. Working through them changed how I think about codec design.

The post [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) argued that codecs compose along the algebraic structure of types: a codec for `Either<A, B>` is built from codecs for `A` and `B` by prepending a tag bit and dispatching; a codec for `Vec<A>` is built from a codec for `A` by iterating. The composition law is structural. It follows the constructors of the algebraic data type, not the distribution of the data.

The post [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/) argued that prefix-freeness lifts the free-monoid construction into bit space. Concatenating prefix-free codewords is unambiguous because each codeword is self-delimiting. The monoid of bit strings decomposes along codec boundaries exactly as the monoid of abstract tokens decomposes along type boundaries.

This series adds a third claim: the choice of leaf codec in any composite type is determined by the prior over that leaf's data, while the composition structure is determined by the type. These are two orthogonal axes of design freedom.

Consider `Either<uint32_t, std::string>`. The composition structure says: write one bit for the variant tag, then write the payload for the selected variant. That part is fixed by the type. What is not fixed is which codec you use for the tag bit, and which codec you use for the `uint32_t` payload.

If 90% of values are `uint32_t` and 10% are `std::string`, a Huffman code over the two-symbol alphabet `{left, right}` gives you the tag bit nearly for free (0.47 bits expected vs. 1 bit for the flat tag). And if the `uint32_t` values cluster in `[0, 127]`, VByte costs exactly 8 bits per value. Swap those observations and you swap those choices.

The type algebra determines the composition; the empirical distribution determines the leaf codecs. Neither axis constrains the other. Attending to both is what makes codec design non-trivial.

## D. The Codec-Selection Library

The `synthesis.hpp` library distills the selection process into three functions.

`empirical_distribution` counts occurrences of each value in a sample and normalizes by the total count to produce a probability map.

`redundancy_for` computes the redundancy of a named code on a given distribution: expected length minus entropy. By Shannon's source-coding theorem, this is always non-negative. A lower redundancy means the code's implied prior is a closer fit to the actual source.

`recommend_code` ties them together: it estimates the empirical distribution from a sample, computes the redundancy of all six candidate universal codes (Unary, Gamma, Delta, Omega, Fibonacci, VByte), and returns the name of the code with the smallest redundancy.

```cpp
inline std::string recommend_code(const std::vector<std::uint64_t>& sample)
{
    auto dist = empirical_distribution(sample);
    constexpr std::string_view candidates[] = {
        "Unary", "Gamma", "Delta", "Omega", "Fibonacci", "VByte"
    };
    double best_redundancy = std::numeric_limits<double>::infinity();
    std::string best_code;
    for (std::string_view candidate : candidates) {
        double r = redundancy_for(candidate, dist);
        if (r < best_redundancy) {
            best_redundancy = r;
            best_code = std::string(candidate);
        }
    }
    return best_code;
}
```

On a strongly geometric sample (value 1 appears much more often than value 2, which appears more often than value 3, and so on), `recommend_code` returns `"Unary"` or `"Fibonacci"`. Both codes are matched to geometric-like priors; their redundancy on geometric data is low. On a sample of values in `[500, 1000]`, it returns `"VByte"` or `"Delta"`: byte-aligned codes have low overhead for values that cluster in that range, and neither Unary nor Gamma would be tolerable there. Unary would require hundreds of bits per value.

The point is not that `recommend_code` is a production-quality selector. It is that it makes the selection process concrete and mechanical. There is no best code in general. There is a best code given a sample.

## E. The Six Principles

These are the things I now believe about coding, after working through twelve instances.

**1. A code is a prior.** Every codeword length implies a probability. Choosing a code is choosing what you believe about the source. The choice is never neutral, even when it is unconscious. When you reach for the first codec you know, you are betting on a prior you may never have examined.

**2. Universality is robustness.** A universal code performs well across many priors, not just one. Elias gamma is asymptotically optimal for any source from a broad class of power-law distributions; VByte is a pragmatic universal for byte-oriented hardware. Use universal codes when you do not know the prior in advance, when the prior may shift over time, or when the engineering cost of measuring the source outweighs the compression gain. Use Huffman or arithmetic coding when you do know the prior and the gain from exploiting it justifies the overhead.

**3. Optimality is measurable.** Shannon's source-coding theorem gives the lower bound: no uniquely-decodable code can have expected length below the entropy $H(p)$ of the source. Every code's redundancy is then measurable as expected-length minus entropy. This is not a theoretical abstraction; you can compute it from a sample in a few lines of code, as `redundancy_for` demonstrates. Pick codes by minimizing redundancy on the actual source, not by intuition about which code sounds right for the problem.

**4. Engineering trades dominate at scale.** Theoretical optima (Elias gamma, delta, arithmetic coding) are outperformed in throughput benchmarks by byte-aligned approximations (VByte) when decode speed is the binding constraint. The reason is cache line alignment and SIMD: modern CPUs process data in 16-byte to 64-byte chunks, and a code that crosses byte boundaries pays a penalty that no redundancy saving can overcome at typical data volumes. Figure out where the binding constraint lives (compression ratio, decode throughput, encode throughput, memory, latency) before you decide which axis to optimize.

**5. Polyalgorithms beat single algorithms.** When source characteristics vary across the data, adapt per-chunk rather than committing to a single global code. RoaringBitmap does this for integer sets: it observes each 65536-element chunk and picks the container type that minimizes storage for that chunk's density. Zstd does this for byte streams: it switches between LZ77, Huffman, and ANS depending on local symbol statistics. The principle is general. Non-stationarity is the norm in real data, and a globally optimal code is locally suboptimal everywhere the source deviates from the global average.

**6. The algebra of composition is orthogonal to the choice of leaf code.** Type structure dictates how codecs compose: `Either` needs a tag, `Vec` needs a count or terminator, `Product` concatenates. These are structural facts about the algebraic data type and they do not change with the data distribution. What does change with the distribution is which codec you assign to each leaf. You can swap leaf codecs without changing the composition law, and you can change the composition structure without touching the leaf codecs. Keeping these two axes separate is what makes a codec library composable rather than a pile of special cases.

## F. What Comes Next

This series covered prefix-free codes from their theoretical foundations through twelve instances, ending at polyalgorithms. I stopped here because this is where the foundations end and the engineering starts getting domain-specific. The frontier extends in three directions I have not covered.

**Context mixing.** The PAQ and ZPAQ family of compressors achieve near-arithmetic-limits on general data by running many predictive models in parallel and mixing their probability estimates with weights that are themselves adapted to the data. The key insight is that no single model is best everywhere; a mixture that weights models by their recent prediction accuracy outperforms any single model on non-stationary sources. Context mixing is the logical culmination of the codes-as-priors idea: you do not pick one prior, you maintain a portfolio.

**Asymmetric Numeral Systems (ANS).** Developed by Jaroslaw Duda around 2014, ANS achieves the compression ratio of arithmetic coding while decoding at 5 to 10 times the speed. It underlies LZ4's entropy backend (FSE), Zstd's entropy coder, Apple's LZFSE, and several video compression standards. ANS works by maintaining a single integer state that encodes the accumulated probability of the symbols seen so far, updated with each new symbol using a lookup table rather than interval arithmetic. If you want to understand how modern fast compressors actually work at the symbol level, ANS is the answer.

**Rate-distortion theory.** Everything in this series assumed lossless compression: the decoder must recover the exact original bit sequence. Rate-distortion theory introduces a different class of trade, where you bound the allowable reconstruction error and ask how few bits you can use subject to that bound. This is the foundation of audio (MP3, AAC, Opus), video (H.264, H.265, AV1), and image compression (JPEG, WebP). The information-theoretic results here are just as clean as Shannon's source-coding theorem, but the engineering is richer because the distortion measure (perceptual quality for audio and video, pixel error for images) shapes every design decision.

The principles from this series carry forward into all three directions: a code is a prior, universality is robustness, optimality is measurable, engineering trades dominate at scale. The new settings add new structure (context histories, state machines, distortion metrics) but not new foundations.

## G. Cross-references

Posts in this series (in order):
[Kraft's Inequality](/post/2020-03-kraft-wire-formats/),
[McMillan's Converse](/post/2020-09-mcmillan-wire-formats/),
[Universal Codes as Priors](/post/2022-01-priors-wire-formats/),
[Unary and Elias Gamma](/post/2022-06-elias-gamma-wire-formats/),
[Elias Delta and Omega](/post/2022-11-elias-delta-omega-wire-formats/),
[Fibonacci Coding](/post/2023-04-fibonacci-wire-formats/),
[Rice / Golomb](/post/2023-09-rice-golomb-wire-formats/),
[VByte / Varint](/post/2024-02-vbyte-wire-formats/),
[Huffman](/post/2024-08-huffman-wire-formats/),
[Arithmetic Coding](/post/2025-01-arithmetic-coding-wire-formats/),
[Succinct Bit Vectors](/post/2025-06-succinct-wire-formats/),
[RoaringBitmap](/post/2025-12-roaring-bitmap-wire-formats/).

Cross-series: [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) and
[When Lists Become Bits](/post/2026-05-prefix-free-stepanov/).

**PFC footnote:** The production implementation of all 12 codes (and several more)
is at [github.com/queelius/pfc](https://github.com/queelius/pfc). This series
develops the theory; PFC is the practice.
