---
title: "Fibonacci Coding"
date: 2023-04-23
draft: false
tags:
- C++
- information-theory
- coding-theory
- prefix-free
- universal-codes
- fibonacci
- zeckendorf
categories:
- Computer Science
- Mathematics
series:
- wire-formats
series_weight: 6
math: true
description: "Fibonacci coding uses Zeckendorf's representation to produce self-synchronizing codewords. Every codeword ends in two consecutive ones; a single bit flip corrupts at most two codewords."
linked_project:
- pfc
- wire-formats
---
*Every code in this series so far has optimized expected length under some implied prior. Fibonacci coding does something different: it gives the decoder a way to recover from errors without help from a lower layer.*

## A Different Design Goal

All the codes in this series have aimed at the same target: assign short codewords to frequent symbols, with length growing roughly as $\log n$ for the $n$-th symbol under some implied prior. Elias gamma minimizes expected length for power-law distributions; delta and omega extend the recursion for heavier tails.

Fibonacci coding has a different goal. It does not optimize for average codeword length under a specific distribution. It optimizes for error resilience. In a stream of gamma-coded integers, a single bit flip in a codeword's length prefix causes the decoder to misread that codeword's length, then misread every subsequent codeword. The error propagates without limit until the decoder somehow reacquires sync. On a reliable channel this is a nonissue. On a noisy one, or in stored data that may have silently rotted, it is a serious problem.

Fibonacci coding avoids this. Every Fibonacci codeword ends in two consecutive 1 bits ("11"). This double-one marker appears nowhere else in the codeword. A single bit flip corrupts the codeword it hits, possibly spills into the next codeword, and then the decoder finds the next "11" and resynchronizes. At most two codewords are corrupted per error. The rest of the stream is intact.

The price is length overhead: Fibonacci codewords are approximately $1.44 \times \log_2 n$ bits long, compared to $\log_2 n$ bits for the entropy lower bound. On a reliable channel, that overhead is not worth paying. On a noisy channel, or in a long-running stream where rare bit errors must not lose the entire tail, the self-synchronization property is worth it.

---

## Zeckendorf's Theorem

Fibonacci numbers starting from $F_2 = 1$: $1, 2, 3, 5, 8, 13, 21, 34, \ldots$

Zeckendorf's theorem: every positive integer $n$ has a unique representation as a sum of non-consecutive Fibonacci numbers. The greedy algorithm produces it by repeatedly subtracting the largest Fibonacci number that does not exceed $n$.

Examples:

| $n$ | Zeckendorf representation | Bit vector (index 0 = $F_2$) |
|-----|---------------------------|-------------------------------|
| 1   | $F_2 = 1$                 | `{1}`                         |
| 4   | $F_4 + F_2 = 3 + 1$       | `{1, 0, 1}`                   |
| 10  | $F_6 + F_3 = 8 + 2$       | `{0, 1, 0, 0, 1}`             |
| 11  | $F_6 + F_4 = 8 + 3$       | `{0, 0, 1, 0, 1}`             |

The non-adjacency condition is the key. No two consecutive Fibonacci numbers appear in the sum. That means no two consecutive Zeckendorf bits are both 1. So appending a 1 after the last Zeckendorf bit creates the only "11" in the entire codeword. The terminator is safe precisely because of the structural property of Zeckendorf representations.

**Implementation.**

```cpp
inline std::vector<bool> to_zeckendorf(std::uint64_t n) {
    assert(n >= 1 && "Zeckendorf is undefined for n = 0");
    // Build the Fibonacci sequence up to n. Start with F_2=1, F_3=2.
    std::vector<std::uint64_t> fibs{1, 2};
    while (fibs.back() <= n) {
        fibs.push_back(fibs[fibs.size() - 1] + fibs[fibs.size() - 2]);
    }
    // The last entry is > n; remove it so all entries are <= n.
    fibs.pop_back();
    // Greedy decomposition: subtract the largest Fibonacci number <= remaining n.
    std::vector<bool> bits(fibs.size(), false);
    for (std::size_t i = fibs.size(); i-- > 0;) {
        if (n >= fibs[i]) {
            bits[i] = true;
            n -= fibs[i];
        }
    }
    return bits;
}
```

---

## The Fibonacci Codeword

To encode $n$:

1. Compute the Zeckendorf bits of $n$: `bits = to_zeckendorf(n)`.
2. Write each bit from index 0 (corresponding to $F_2$) through the last set index.
3. Append a terminating 1 bit.

The codeword ends with the last Zeckendorf bit (which is always 1, since the highest Fibonacci number in the sum is always included) followed by the terminating 1. That final "11" is the synchronization marker. Nothing in the payload can look like it.

Codeword examples:

| $n$ | Zeckendorf bits | Codeword |
|-----|-----------------|----------|
| 1   | `{1}`           | `1 1`    |
| 2   | `{0, 1}`        | `0 1 1`  |
| 3   | `{0, 0, 1}`     | `0 0 1 1`|
| 4   | `{1, 0, 1}`     | `1 0 1 1`|
| 8   | `{0, 0, 0, 0, 1}`| `0 0 0 0 1 1`|

Decoding reads bits until it sees two consecutive 1s. The second 1 is the terminator. All preceding bits, including the first 1 of that terminal pair, are Zeckendorf bits. Reconstruct $n$ from the Fibonacci sum. The decoder needs no length field, no lookahead, and no external framing.

**Implementation.**

```cpp
struct Fibonacci {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1 && "Fibonacci is undefined for n = 0");
        auto bits = to_zeckendorf(n);
        for (bool b : bits) sink.write(b);
        sink.write(true);  // terminating '1' bit
    }

    template<BitSource S>
    static value_type decode(S& source) {
        std::vector<bool> bits;
        bool prev = false;
        while (true) {
            bool cur = source.read();
            if (cur && prev) break;  // two consecutive 1s: stop
            bits.push_back(cur);
            prev = cur;
        }
        // Reconstruct n from the Fibonacci sum.
        std::vector<std::uint64_t> fibs{1, 2};
        while (fibs.size() < bits.size())
            fibs.push_back(fibs[fibs.size()-1] + fibs[fibs.size()-2]);
        value_type n = 0;
        for (std::size_t i = 0; i < bits.size(); ++i)
            if (bits[i]) n += fibs[i];
        return n;
    }
};
```

The decoder has no length information in the stream. It reads until it finds "11" and stops. That is the entire decoder. The self-delimiting structure comes from the codeword itself, not from any external framing.

---

## The Implied Prior

The Fibonacci number $F_k$ grows roughly as $\phi^k / \sqrt{5}$ where $\phi = (1 + \sqrt{5})/2 \approx 1.618$ is the golden ratio. Because the Zeckendorf representation of $n$ uses roughly $\log_\phi n$ Fibonacci numbers, the codeword length is approximately:

$$\ell(n) \approx \log_\phi n + 1 = \frac{\log_2 n}{\log_2 \phi} + 1 \approx 1.44 \log_2 n + 1 \text{ bits.}$$

This is about 44% longer than the Shannon entropy lower bound of $\log_2 n$ bits. The implied prior is geometric with golden-ratio base: $p_n \sim \phi^{-n}$. It sits between the geometric prior of unary coding ($p_n \sim 2^{-n}$, very light tail) and the power-law priors of the Elias codes.

Compared to Elias gamma ($\sim 2 \log_2 n$ bits), Fibonacci is shorter for large $n$; for small $n$ (below about 16), gamma can be shorter. Neither code is optimal for the other's implied prior. That is not the point. Fibonacci's value is not in its compression ratio. It is in the structural guarantee that comes from the "11" terminator.

---

## Self-Synchronization

Consider a long stream of Fibonacci-coded integers and a single bit flip somewhere in the stream. Two cases.

**Case 1: the flip is within the Zeckendorf payload of codeword $c$.** The payload changes, so $c$ decodes to a wrong value. But the "11" terminator was not touched; the decoder finds it and resynchronizes correctly before codeword $c+1$. One codeword corrupted.

**Case 2: the flip hits a 1 bit that was part of "11".** The terminator "11" is damaged. The decoder either misses it (turning "11" into "10" or "01") or creates a spurious "11" inside the payload. If it misses the terminator, the decoder merges codeword $c$ with $c+1$, reads both as one wrong value, then finds the terminator of $c+1$ and resynchronizes. Two codewords corrupted at most.

In both cases, the stream recovers. Compare this to Elias gamma: a flip in a gamma codeword's unary prefix tells the decoder the wrong length, and every subsequent codeword shifts by the wrong amount. Error propagation is unbounded.

The test in `test_fibonacci.cpp` demonstrates the bounded-damage property concretely: encode `{3, 5, 7, 9, 11, 13}`, flip bit 0 (within the codeword for 3), and verify that at least 4 of the 6 values are recovered intact:

```cpp
TEST(FibonacciTest, BitFlipStaysLocal) {
    const std::vector<uint64_t> original = {3, 5, 7, 9, 11, 13};
    auto encoded = encode_sequence(original);
    auto corrupted = flip_bit(encoded, 0);  // flip within codeword for 3
    // Decode with EOF guard to avoid infinite loops on corrupted streams.
    // Verify at least 4 of 6 values are recovered.
    EXPECT_GE(matches, 4);
}
```

---

## When to Use Fibonacci

Fibonacci is a niche code. Its 44% overhead relative to entropy is real: for most applications, the Elias codes or Huffman coding achieve much better compression on reliable channels. Fibonacci makes sense when:

- The channel has occasional bit errors, and losing an entire stream after the first error is unacceptable.
- The data is long-lived or stored in environments where isolated bit rot can occur.
- The decoder has no error-correcting layer and must resynchronize from the content itself.

In practice, most systems handle errors at a lower layer (CRC, FEC codes, RAID parity) rather than at the codec level. Where those exist, Fibonacci's self-sync property is irrelevant and the Elias codes or Huffman are better choices.

The [PFC library](https://github.com/queelius/pfc) includes `Fibonacci` in `codecs.hpp` alongside the Elias codes. It is there because the use cases are real, even if they are uncommon.

---

## Cross-References and Notes

**Forward:** Post 7 ("Rice / Golomb") covers the first parametric code family in this series, where a parameter $k$ tunes the code for geometrically distributed sources. No link yet (forthcoming).

**Back:** [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3) gives the redundancy framework used in the integration tests. [Unary and Elias Gamma](/post/2022-06-elias-gamma-wire-formats/) (post 4) covers the codes that Fibonacci is the error-resilient alternative to.

**Cross-series:** Fibonacci's self-synchronizing "11" marker is a structural property of the codeword, not of the algebraic type system. The Stepanov bridge posts cover how codecs compose over types. Fibonacci's error properties are orthogonal to that story and are not discussed there.

**Production code:** `include/pfc/codecs.hpp` in the [PFC library](https://github.com/queelius/pfc) has `Fibonacci`, with the same encode/decode interface but zero-copy byte-buffer I/O.
