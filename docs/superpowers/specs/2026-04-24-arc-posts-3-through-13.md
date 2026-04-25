# Design: Algebra over Wire Formats, posts 3 through 13 (sub-project 3 master spec)

**Date:** 2026-04-24
**Status:** Awaiting approval
**Project:** Per-post outlines for the 11 remaining posts in the wire-formats series
**Repo:** `~/github/metafunctor-series/wire-formats/`
**Predecessor:** sub-project 2 (posts 1 and 2, Kraft and McMillan, shipped as `4786824` and `6bd7b13`)

## Goal

Cover the full per-post content for posts 3 through 13 of the Algebra over Wire Formats series so that the 7 sub-sub-project plans (3a through 3g) can be written from a single authoritative source. Each post's outline carries enough specificity that its prose can be drafted directly from the outline.

The series's central thesis (locked in during the post 3 brainstorm): **a code is a hypothesis about the source.** For a prefix-free code with codeword lengths $(l_1, \ldots, l_n)$, the code implicitly assigns probability $p_i \propto 2^{-l_i}$ to symbol $i$. By Shannon's source-coding theorem, the code achieves expected length equal to entropy precisely when its implicit prior matches the source. Each universal code corresponds to a different prior; each entropy-optimal code (Huffman, arithmetic) is best for a different reason; each succinct data structure is the right answer under different access patterns.

This spec records what each of the remaining 11 posts says, what code it implements, what tests verify it, and how it fits the arc.

## Scope decomposition

This spec covers all of sub-project 3, decomposed into 7 sub-sub-projects:

| Sub-project | Posts | Title cluster |
|---|---|---|
| 3a | 3, 4 | Framing + first codes (Universal Codes as Priors, Unary and Elias Gamma) |
| 3b | 5, 6 | Elias Delta/Omega + Fibonacci |
| 3c | 7, 8 | Rice/Golomb + VByte |
| 3d | 9 | Huffman |
| 3e | 10 | Arithmetic Coding |
| 3f | 11, 12 | Succinct Bit Vectors + RoaringBitmap |
| 3g | 13 | Synthesis |

Each sub-sub-project gets its own implementation plan (the 7 plans this spec hands off to).

## Architectural decisions inherited from sub-project 2

These are not re-derived here. See sub-project 2's spec at `docs/superpowers/specs/2026-04-24-bootstrap-and-posts-1-2-design.md` for the original decisions.

| Decision | Choice |
|---|---|
| Series scope | Medium (10-15 posts), MacKay-shaped |
| Repo location | `~/github/metafunctor-series/wire-formats/` |
| Series title | Algebra over Wire Formats |
| Per-post directory layout | Flat: `index.md`, `<topic>.hpp`, `test_<topic>.cpp` |
| Code style | C++23, headers-only, GoogleTest |
| Voice | Alex's, soul-checked, no em-dashes |
| Math | LaTeX (display `$$...$$`, inline `\(...\)`) |
| Dating | Backdated 2020-2026, ~2 posts/year |
| Sync | FIXED Makefile (per-directory rsync, scoped --delete) |

## The series-wide framing (recapped)

Each subsequent post (4 through 12) develops a specific instance of the post 3 claim. The pattern in each post:

1. Introduce the code (history, motivation, what it looks like).
2. Implement it (~100-200 lines of pedagogical code, mirrors PFC's production version).
3. Derive its implied prior (the distribution `p_i ∝ 2^{-l_i}` corresponding to its lengths).
4. Verify the implied prior is what the code is optimal for (test: encoding from this prior achieves entropy).
5. Discuss when to use it (which sources match the prior closely enough).
6. Cross-reference: forward to next post in the arc, back to post 3 for the framing.

Post 13 (Synthesis) closes by recapitulating the whole pattern across all 12 instances and connecting back to the Stepanov-side codecs-as-functors framing.

## Voice, code, test conventions

Same as sub-project 2 (no changes):

- Each post draws code from the corresponding PFC header where possible (e.g., post 9 references `pfc/huffman.hpp`).
- Each post's implementation is in `namespace <topic>` to match the per-post namespacing already used (`namespace kraft`, `namespace mcmillan`).
- Tests follow the `<TopicNameTest>` GoogleTest suite convention (`KraftTest`, `McMillanTest`, `UnaryGammaTest`, etc.).
- Each post adds one row to wire-formats's `docs/about.md` (the arc page) by changing its row from "Forthcoming" to "Published".
- Each post adds itself to wire-formats's `mkdocs.yml` nav under the appropriate section ("Foundations", "Universal Codes", "Entropy-Optimal", "Succinct Data Structures", "Synthesis").

## Cross-reference map

Each post's section G (cross-references) follows this pattern:

- **Backward** to all previously-published posts that this post extends (typically post 3 for the framing, post 1/2 for Kraft/McMillan, the immediate predecessor in the arc).
- **Forward** to the next post in the arc (no link if not yet shipped, plain text "forthcoming").
- **Cross-series** to the Stepanov bridge posts ([Bits Follow Types](/post/2026-05-codecs-functors-stepanov/), [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/)) when the post connects to type algebra (most do; the entropy-optimal and succinct posts connect less directly).
- **PFC footnote** at the end pointing to the production version (specific PFC headers per post).

A consolidated cross-reference table:

| Post | Forward links | Back links | Cross-series |
|---|---|---|---|
| 3 | post 4 | posts 1, 2 | both bridges |
| 4 | post 5 | post 3, post 1 | When Lists Become Bits (Gamma example) |
| 5 | post 6 | post 4, post 3 | both bridges |
| 6 | post 7 | post 4, post 3 | (none, Fibonacci is its own animal) |
| 7 | post 8 | post 4, post 3 | both bridges |
| 8 | post 9 | post 4, post 3 | (none, byte-alignment is orthogonal) |
| 9 | post 10 | post 2 (McMillan construction), post 3 | Bits Follow Types (Either's tag-bit cost note) |
| 10 | post 11 | post 9, post 3 | Bits Follow Types (Either's tag-bit cost note) |
| 11 | post 12 | post 1 (Kraft as space bound), post 3 | (none, succinct is orthogonal to coding) |
| 12 | post 13 | post 11, post 3 | (none) |
| 13 | (none, end of arc) | every previous post | both bridges (closes the loop) |

---

## Per-post outlines

Each outline below specifies: title, date, code budget, prose budget, opening H2, and per-section content (A through G).

---

## Post 3: "Universal Codes as Priors"

**Slot weight:** 3
**Date:** 2022-01-15
**Code budget:** ~150 lines (`priors.hpp` containing `implied_prior`, `entropy`, `expected_length`, `redundancy`, plus a few helpers)
**Prose budget:** ~2000 words
**Opening H2:** "Universal Codes as Priors"

### Sections

#### A. The Question (~200 words, no code)

You want to compress a stream of integers. Which code should you use? The answer depends on the source. A geometric source (where small integers dominate) wants a short codeword for 1, longer for 2, longer still for 3. A power-law source (where small integers dominate but the tail matters) wants a different schedule. A uniform source over `{1, ..., 256}` wants 8-bit fixed-width codewords. Each choice corresponds to a hypothesis about the source.

Foreshadow the thesis: every prefix-free code is a hypothesis about the integer distribution it expects. The rest of this series unpacks that claim across 10 specific codes.

#### B. The Correspondence: Lengths to Priors (~300 words + ~50 lines code)

For a prefix-free code with codeword lengths $(l_1, \ldots, l_n)$ saturating Kraft (sum equals 1), the implicit probability of symbol $i$ is $p_i = 2^{-l_i}$. If the code does not saturate Kraft (sum < 1), normalize: $p_i = 2^{-l_i} / \sum_j 2^{-l_j}$.

This is the inverse of Shannon's prescription: given a distribution $p_i$, the optimal codeword length is $-\log_2 p_i$ (rounded up to integer). Going backward, given a length, the implicit probability is $2^{-l_i}$.

Implement `implied_prior(lengths) -> probabilities`:

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

Plus tests verifying the implied prior for unary `{1, 2, 3, ...}` is geometric(1/2) and for gamma `{1, 3, 3, 5, 5, 5, 5, 7, ...}` is approximately $1/n^2$ (with a small correction for non-saturating Kraft sums).

#### C. Optimality: Shannon's Theorem and Expected Length (~300 words + ~80 lines code)

State Shannon's source-coding theorem: for any distribution $p_i$ and any prefix-free code with lengths $l_i$, the expected length $\sum_i p_i l_i$ is at least the entropy $H(p) = -\sum_i p_i \log_2 p_i$. Equality holds iff $l_i = -\log_2 p_i$ for all $i$ (which requires $p_i$ to be a power of 1/2 and the lengths to saturate Kraft).

Define and implement:

```cpp
inline double entropy(const std::vector<double>& probs) {
    double h = 0.0;
    for (double p : probs) {
        if (p > 0) h -= p * std::log2(p);
    }
    return h;
}

inline double expected_length(const std::vector<double>& probs,
                              const std::vector<std::size_t>& lengths) {
    assert(probs.size() == lengths.size());
    double L = 0.0;
    for (std::size_t i = 0; i < probs.size(); ++i) {
        L += probs[i] * static_cast<double>(lengths[i]);
    }
    return L;
}

inline double redundancy(const std::vector<double>& probs,
                         const std::vector<std::size_t>& lengths) {
    return expected_length(probs, lengths) - entropy(probs);
}
```

Test that for any prior and any code: redundancy ≥ 0 (Shannon), with equality at the dyadic optimum.

#### D. Two Concrete Examples: Unary and Gamma (~300 words + ~30 lines code)

**Unary:** lengths $1, 2, 3, \ldots$. Saturates Kraft (sum = 1). Implied prior is geometric(1/2): $p_i = 2^{-i}$. Test that `expected_length(geometric_half, unary_lengths(K))` equals `entropy(geometric_half)` to within floating-point tolerance for any truncation $K$. This is the case where the code achieves entropy *exactly* (because the prior happens to be dyadic).

**Gamma:** lengths $2k+1$ for integers in the block $[2^k, 2^{k+1})$. The Kraft sum equals 1 in the limit (gamma is asymptotically saturating). The implied prior is approximately $p_n \propto 1/n^2$. Test that `expected_length(power_law(2), gamma_lengths(N))` is close to `entropy(power_law(2))` (within a small constant additive overhead, the universal-code redundancy).

The point: unary is exactly optimal for one specific prior; gamma is approximately optimal for a class of priors that decay like $1/n^2$.

#### E. The Table of Priors (~250 words, no code)

A summary table mapping each universal code in the rest of the series to its implied prior:

| Code | Implied prior | Optimal source |
|---|---|---|
| Unary | $p_n = 2^{-n}$ (geometric, $r=1/2$) | Source where each value is half as likely as the next |
| Elias gamma | $p_n \propto 1/n^2$ (power law, exponent 2) | Power-law source like word frequency |
| Elias delta | $p_n \propto 1/(n \log^2 n)$ (slower decay) | Sources with heavier tails than $1/n^2$ |
| Fibonacci | $p_n \propto \phi^{-n}$ where $\phi = (1+\sqrt{5})/2$ | Robust against single-bit corruption |
| Rice($k$) / Golomb($m$) | Geometric with parameter tied to $k$ or $m$ | Tunable: pick $k$ to match the source's expected value |
| VByte / Varint | Approximately step-uniform over byte boundaries | Most values fit in 1-2 bytes; byte-aligned for speed |
| Huffman | Any explicit finite distribution | Known finite alphabet with known frequencies |
| Arithmetic | Any distribution | Universal: only achieves entropy in the limit (non-integer length per symbol) |

The point: each code is optimal somewhere. Choosing a code is choosing a prior.

#### F. Universality (~250 words, no code)

A code is *universal* if it performs within a constant factor of the entropy across a wide class of distributions, not just one. Universal codes solve a different problem than the entropy-optimal Huffman or arithmetic codes: you don't need to know the distribution in advance.

Discuss the minimax framing: a universal code minimizes the worst-case redundancy over a class of distributions. Elias gamma is universal over $\{p : p_n = O(1/n^2)\}$. Elias delta is universal over a slightly larger class (heavier tails). The *universality* in "universal codes" means "robust to your prior being approximately right rather than exactly right."

Forward-pointer: posts 4 through 8 develop the universal codes; posts 9 and 10 develop the entropy-optimal codes (Huffman and arithmetic), which need the distribution as input.

#### G. Cross-references and footnote (~120 words)

- Forward: [Unary and Elias Gamma](/post/2022-06-elias-gamma-wire-formats/) (post 4) develops the first concrete instances.
- Back: [Kraft's Inequality](/post/2020-03-kraft-wire-formats/) characterizes which length vectors (and therefore which implicit priors) are achievable. [McMillan's Converse](/post/2020-09-mcmillan-wire-formats/) gives the construction.
- Cross-series: [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) and [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/) develop the type-algebra side; this post and the rest of the wire-formats series develop the bit-level information-theory side.
- Footnote: PFC's `include/pfc/codecs.hpp` is the production catalogue of universal codes; this series develops them one at a time.

---

## Post 4: "Unary and Elias Gamma"

**Slot weight:** 4
**Date:** 2022-06-19
**Code budget:** ~180 lines (`unary_gamma.hpp` containing `Unary` and `Gamma` codecs plus their length-vector generators)
**Prose budget:** ~2000 words
**Opening H2:** "Unary and Elias Gamma"

### Sections

#### A. The Two Simplest Universal Codes (~200 words, no code)

Unary and Elias gamma are the two simplest universal codes. Unary is so simple it predates information theory by centuries (tally marks). Gamma is Peter Elias's 1975 extension that brings logarithmic length while preserving prefix-freeness.

Frame both as instances of post 3's claim. Unary corresponds to a geometric prior (each symbol half as likely as the previous). Gamma corresponds to a power-law prior with exponent 2 (each symbol's probability decays as $1/n^2$).

#### B. Unary: Geometric Prior (~300 words + ~30 lines code)

The code: integer $n \geq 1$ encodes as $(n-1)$ zero bits followed by a one bit. So $1 \to "1"$, $2 \to "01"$, $3 \to "001"$, etc.

Implementation:

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

Length analysis: codeword for $n$ has length $n$. Kraft sum: $\sum_{n=1}^{\infty} 2^{-n} = 1$ (saturates). Implied prior: $p_n = 2^{-n}$, the geometric distribution with parameter 1/2.

Test: `expected_length(geometric_half_truncated(K), unary_lengths(K))` equals `entropy(geometric_half_truncated(K))` for any $K$, modulo the truncation tail.

#### C. Elias Gamma: Power-Law Prior (~300 words + ~50 lines code)

The code: integer $n \geq 1$ encodes as $\lfloor \log_2 n \rfloor$ zero bits, then a one bit, then the binary representation of $n$ minus its leading 1 (MSB first). Length: $2 \lfloor \log_2 n \rfloor + 1$ bits.

Examples: $1 \to "1"$, $2 \to "010"$, $3 \to "011"$, $4 \to "00100"$, $5 \to "00101"$, $6 \to "00110"$, $7 \to "00111"$, $8 \to "0001000"$.

Implementation: re-state Gamma from post 1 (or 2) verbatim. The implementation is the same; the framing is new.

Length analysis: code has length $2 \lfloor \log_2 n \rfloor + 1$. For each block $n \in [2^k, 2^{k+1})$ (size $2^k$), each codeword has length $2k+1$, contributing $2^k \cdot 2^{-(2k+1)} = 2^{-(k+1)}$ to the Kraft sum. Summing over $k$: $\sum_{k=0}^{\infty} 2^{-(k+1)} = 1$ (saturates).

Implied prior: $p_n = 2^{-(2 \lfloor \log_2 n \rfloor + 1)}$. Approximates $1/(2n^2)$ for $n$ a power of 2, with a $\leq 2\times$ deviation for other $n$. Conclusion: gamma is approximately optimal for a $1/n^2$ prior.

Test: `redundancy(power_law(2, N), gamma_lengths(N))` is small (within a constant additive bound).

#### D. Length Characteristics: O(n) vs O(log n) (~250 words, no code)

Unary's length grows linearly: encoding $n$ takes $n$ bits. Gamma's length grows logarithmically: $2 \log_2 n + 1$ bits. The crossover is at $n = 2$: both give 1 bit for $n=1$, but unary takes 2 bits for $n=2$ versus gamma's 3 bits. From $n=3$ onward, gamma starts winning.

Show a small comparison table:

| $n$ | Unary length | Gamma length |
|---|---|---|
| 1 | 1 | 1 |
| 2 | 2 | 3 |
| 4 | 4 | 5 |
| 8 | 8 | 7 |
| 16 | 16 | 9 |
| 100 | 100 | 13 |
| 1024 | 1024 | 21 |

The lesson: unary is right when very small values dominate (most encodings are $1$ or $2$); gamma is right when small values dominate but the tail extends.

#### E. When to Use Which (~250 words, no code)

Concrete guidance: use unary when you expect 90%+ of values to be 1 or 2. Use gamma when you expect a heavy left-skew with a long tail (typical for word frequencies, file sizes, and many natural distributions). For exact optimality, fit your data to one of these priors first; if the data fits geometric(1/2), unary; if it fits a power law with exponent ~2, gamma.

Discuss the decoding cost: both decode in time linear in the codeword length, which is therefore linear in the value being decoded.

#### F. The Recursive Idea (~200 words, no code)

Gamma's code is a self-delimiting binary representation of $n$: write the length, then the bits. Notice that the "length" part of gamma is itself encoded in unary. What if we encoded the length in gamma instead? That gives Elias delta. What if we recursed: encode the length of the length in gamma? That gives Elias omega.

Each recursion halves the asymptotic overhead for very large $n$ at the cost of slightly more bits for small $n$. The crossover point shifts further out with each recursion.

Forward to post 5: this recursive idea is the key to delta and omega.

#### G. Cross-references and footnote (~120 words)

- Forward: [Elias Delta and Omega](/post/2022-11-elias-delta-omega-wire-formats/) (post 5) develops the recursive elaborations.
- Back: [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3) is the framing this post instantiates twice.
- Cross-series: [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/) uses Gamma as its running example for prefix-free length encoding.
- Footnote: PFC's `include/pfc/codecs.hpp` has both `Unary` and `Gamma` along with the rest of the universal-codes catalogue.

---

## Post 5: "Elias Delta and Omega"

**Slot weight:** 5
**Date:** 2022-11-13
**Code budget:** ~200 lines (`elias_delta_omega.hpp` containing `Delta` and `Omega` codecs)
**Prose budget:** ~2000 words
**Opening H2:** "Recursive Elaborations of Gamma"

### Sections

#### A. Where Gamma Stops Being Good (~200 words, no code)

Recap: gamma encodes $n$ in $2 \log_2 n + 1$ bits. The "length prefix" part (the leading zeros plus the single 1) takes $\log_2 n + 1$ bits, which is half of gamma's total. For very large $n$, that's wasteful: the length prefix is itself just a small integer, and we have a perfectly good code (gamma!) for small integers.

Elias delta is the answer: encode the length in gamma instead of unary. Elias omega goes further: encode recursively until the count is 1.

#### B. Elias Delta (~300 words + ~60 lines code)

The code for $n$: write the length $L = \lfloor \log_2 n \rfloor + 1$ in gamma, then the bits of $n$ minus its leading 1.

Examples: $1 \to "1"$ (gamma encoding of $L=1$). $2 \to$ gamma($2$) $\cdot$ "0" = $"010" \cdot "0" = "0100"$. $3 \to$ gamma($2$) $\cdot$ "1" = $"0101"$. $4 \to$ gamma($3$) $\cdot$ "00" = $"01100"$.

Implementation:

```cpp
struct Delta {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1);
        std::size_t bits = std::bit_width(n);  // L = log2(n) + 1
        Gamma::encode(static_cast<std::uint64_t>(bits), sink);
        for (std::size_t i = bits - 1; i > 0; --i) {
            sink.write(((n >> (i - 1)) & 1) != 0);
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

Length: `L_delta(n) = L_gamma(L_gamma_of_length) + (L - 1) = O(log n + log log n)`. Concretely: $\log_2 n + 2 \lfloor \log_2 \log_2 n \rfloor + 1$ bits in the asymptotic.

Implied prior: roughly $1/(n \log^2 n)$, slightly heavier tail than gamma's $1/n^2$.

#### C. Elias Omega (~300 words + ~60 lines code)

The recursive version: encode $\lfloor \log_2 n \rfloor + 1$ in omega instead of gamma, with the recursion ending at 1.

Algorithm: write the value, then write its length, then write the length-of-length, ..., until you hit 1. Then read in reverse: the first byte tells you how many bits the next field has, which tells you how many bits the field after that has, etc.

Implementation: present the iterative form (use a stack to encode in reverse, then write).

Length: $O(\log^* n)$ asymptotically (iterated logarithm). For practical $n$ (up to $2^{64}$), length is at most a small constant beyond delta.

#### D. The Crossover Points (~250 words, no code)

Show a table of code lengths for the same $n$ across unary, gamma, delta, omega:

| $n$ | Unary | Gamma | Delta | Omega |
|---|---|---|---|---|
| 1 | 1 | 1 | 1 | 1 |
| 4 | 4 | 5 | 5 | 5 |
| 16 | 16 | 9 | 8 | 8 |
| 256 | 256 | 17 | 13 | 12 |
| $2^{16}$ | 65536 | 33 | 22 | 19 |
| $2^{32}$ | 4 billion | 65 | 39 | 33 |

The lesson: each successive code is shorter for large $n$ at the cost of being slightly longer for small $n$. The crossover where delta beats gamma is around $n = 16$; omega beats delta around $n = 1000$.

#### E. The Implied Prior Ladder (~250 words, no code)

Each recursion step shifts the implied prior to a heavier tail: gamma's $1/n^2$, delta's $1/(n \log^2 n)$, omega's slightly heavier still. Observe that as the prior becomes heavier-tailed, the optimal code becomes more "patient" with large values.

Discuss the design trade: heavier-tailed priors mean the code tolerates large outliers better but pays a small constant overhead for small values.

#### F. The Limit (~200 words, no code)

Asymptotically, omega achieves $O(\log^* n)$ length, which is essentially constant for practical $n$. This is the fundamental lower bound for self-delimiting integer codes (the codeword must somehow describe its own length, which requires at least $\log^* n$ bits).

Note: omega is rarely used in practice (the constant overhead dominates), but it's the theoretical endpoint of the recursion. Practical implementations stop at delta.

#### G. Cross-references and footnote (~120 words)

- Forward: [Fibonacci Coding](/post/2023-04-fibonacci-wire-formats/) (post 6) takes a different design philosophy.
- Back: [Unary and Elias Gamma](/post/2022-06-elias-gamma-wire-formats/) (post 4); [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3).
- Cross-series: both Stepanov bridge posts.
- Footnote: PFC's `include/pfc/codecs.hpp` has `EliasDelta` and `EliasOmega`.

---

## Post 6: "Fibonacci Coding"

**Slot weight:** 6
**Date:** 2023-04-23
**Code budget:** ~180 lines (`fibonacci.hpp` containing `Fibonacci` codec plus the Zeckendorf representation helpers)
**Prose budget:** ~2000 words
**Opening H2:** "Fibonacci Coding"

### Sections

#### A. A Different Design Goal (~200 words, no code)

Elias codes (gamma, delta, omega) optimize for length under power-law-like priors. Fibonacci coding optimizes for something else: error resilience. A single bit flip in a gamma codeword can desynchronize the entire stream that follows. A single bit flip in a Fibonacci codeword affects at most that codeword and its immediate neighbor.

The key property: every Fibonacci codeword ends in "11" (two consecutive ones), and no Fibonacci codeword contains "11" elsewhere. The "11" is a self-synchronizing marker.

#### B. Zeckendorf's Theorem (~250 words + ~40 lines code)

State Zeckendorf's theorem: every positive integer has a unique representation as a sum of non-consecutive Fibonacci numbers (using $F_2 = 1, F_3 = 2, F_4 = 3, F_5 = 5, F_6 = 8, ...$).

Examples: $1 = F_2$. $4 = F_4 + F_2 = 3 + 1$. $10 = F_6 + F_3 = 8 + 2$. $11 = F_6 + F_4 = 8 + 3$.

Implement `to_zeckendorf(n)`: greedy algorithm subtracting the largest Fibonacci number $\leq n$, repeating.

```cpp
inline std::vector<bool> to_zeckendorf(std::uint64_t n) {
    // Build Fibonacci numbers up to n.
    std::vector<std::uint64_t> fibs{1, 2};
    while (fibs.back() <= n) fibs.push_back(fibs[fibs.size()-1] + fibs[fibs.size()-2]);
    fibs.pop_back();
    // Greedy decompose.
    std::vector<bool> bits(fibs.size(), false);
    for (std::size_t i = fibs.size(); i-- > 0;) {
        if (n >= fibs[i]) {
            bits[i] = true;
            n -= fibs[i];
        }
    }
    return bits;  // bits[i] = 1 if fibs[i] is in the decomposition
}
```

#### C. The Fibonacci Codeword (~300 words + ~50 lines code)

Codeword for $n$: write the Zeckendorf bits in order from $F_2$ outward, then append a final "1" as the terminator. This produces a string that ends in "11" (the last Zeckendorf bit plus the terminator) and contains no other "11" (because Zeckendorf requires non-consecutive Fibonacci numbers).

Examples: $1 \to "11"$. $2 \to "011"$. $3 \to "0011"$. $4 \to "1011"$. $5 \to "00011"$. $6 \to "10011"$. $7 \to "01011"$. $8 \to "000011"$.

Implementation: `Fibonacci::encode` writes the Zeckendorf bits then a `1`; `Fibonacci::decode` reads bits until two consecutive `1`s, then consumes the second `1` as the terminator.

#### D. The Implied Prior (~250 words, no code)

Length analysis: codeword for $n$ has length equal to the number of Zeckendorf bits plus one (the terminator). The number of Zeckendorf bits is roughly $\log_\phi n$ where $\phi = (1 + \sqrt{5})/2 \approx 1.618$.

So: Fibonacci length is $\log_\phi n + 1 \approx 1.44 \log_2 n + 1$ bits. About 44% longer than the entropy lower bound.

Implied prior: $p_n \propto \phi^{-n}$. This is the geometric distribution with golden-ratio base; heavier than $1/n^2$ but lighter than $1/n$.

Test: redundancy of Fibonacci on its implied prior is small.

#### E. Self-Synchronization (~250 words + ~30 lines code)

The "11" marker means a single bit error can corrupt at most two codewords. Demonstrate this with a small test: encode a sequence of integers, flip a random bit in the encoded stream, decode, and observe that the corruption stays local.

```cpp
TEST(FibonacciTest, BitFlipStaysLocal) {
    auto encoded = encode_sequence(Fibonacci{}, {3, 5, 7, 9, 11, 13});
    auto corrupted = flip_bit(encoded, /*bit_index=*/7);
    auto decoded = decode_sequence(Fibonacci{}, corrupted);
    // Most of the original sequence is recovered; at most 2 codewords are wrong.
    int matches = 0;
    for (std::size_t i = 0; i < std::min(decoded.size(), std::size_t{6}); ++i) {
        if (decoded[i] == std::vector<int>{3,5,7,9,11,13}[i]) ++matches;
    }
    EXPECT_GE(matches, 4);  // At least 4 of 6 unaffected.
}
```

This contrasts with gamma: a single bit flip in gamma can desynchronize indefinitely.

#### F. When to Use Fibonacci (~200 words, no code)

Use cases: streams over noisy channels (radio, storage with rare bit errors); long-running data streams where occasional corruption shouldn't lose the entire tail; any context where local error containment is worth a 44% length overhead over entropy.

Note that Fibonacci is rarely the right choice when the channel is reliable; the overhead is significant. It's a niche code with a specific virtue.

#### G. Cross-references and footnote (~120 words)

- Forward: [Rice / Golomb](/post/2023-09-rice-golomb-wire-formats/) (post 7) takes yet another design angle: parametric optimization for geometric sources.
- Back: [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3); [Unary and Elias Gamma](/post/2022-06-elias-gamma-wire-formats/) (post 4) for comparison.
- Cross-series: not directly relevant (Fibonacci is its own animal; the type-algebra side does not particularly need self-synchronization).
- Footnote: PFC's `include/pfc/codecs.hpp` has `Fibonacci`.

---

## Post 7: "Rice / Golomb"

**Slot weight:** 7
**Date:** 2023-09-17
**Code budget:** ~200 lines (`rice_golomb.hpp` containing `RiceCodec<k>` and `GolombCodec<m>` plus parameter-selection helpers)
**Prose budget:** ~2000 words
**Opening H2:** "Parametric Codes for Geometric Sources"

### Sections

#### A. The First Parametric Code (~200 words, no code)

All codes seen so far have been monolithic: unary is unary, gamma is gamma. Rice and Golomb introduce a parameter $k$ (or $m$) that lets you tune the code to a specific source's expected value. This is the first time we get to choose.

Frame it: Rice($k$) is a family of codes, one per value of $k$. Each member is optimal for a specific geometric distribution. Choosing $k$ is choosing your prior precisely.

#### B. Rice Coding (~300 words + ~50 lines code)

The Rice($k$) code splits an integer $n \geq 0$ into $q = n / 2^k$ (quotient) and $r = n \mod 2^k$ (remainder, $k$ bits). Encode: unary($q+1$) followed by the $k$-bit binary representation of $r$.

(Note: we use $n \geq 0$ here; many codes start at 1, but Rice is more naturally defined for non-negative integers.)

Examples ($k = 2$, so $r$ takes 2 bits):

| $n$ | $q$ | $r$ | Codeword |
|---|---|---|---|
| 0 | 0 | 0 | "1 00" |
| 1 | 0 | 1 | "1 01" |
| 2 | 0 | 2 | "1 10" |
| 3 | 0 | 3 | "1 11" |
| 4 | 1 | 0 | "01 00" |
| 5 | 1 | 1 | "01 01" |
| ... | | | |

Implementation:

```cpp
template<std::size_t K>
struct Rice {
    using value_type = std::uint64_t;
    static_assert(K > 0 && K < 64);

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        std::uint64_t q = n >> K;
        std::uint64_t r = n & ((std::uint64_t{1} << K) - 1);
        for (std::uint64_t i = 0; i < q; ++i) sink.write(false);
        sink.write(true);
        for (std::size_t i = 0; i < K; ++i) {
            sink.write(((r >> (K - 1 - i)) & 1) != 0);
        }
    }

    template<BitSource S>
    static value_type decode(S& source) {
        std::uint64_t q = 0;
        while (!source.read()) ++q;
        std::uint64_t r = 0;
        for (std::size_t i = 0; i < K; ++i) {
            r = (r << 1) | (source.read() ? 1 : 0);
        }
        return (q << K) | r;
    }
};
```

#### C. Golomb Coding (~250 words + ~40 lines code)

Golomb($m$) generalizes Rice to non-power-of-2 dividers. Encode: unary of quotient $q = n / m$ followed by truncated binary representation of $r = n \mod m$.

The "truncated binary" part: for $r < 2^{\lceil \log_2 m \rceil} - m$, use $\lfloor \log_2 m \rfloor$ bits; otherwise use $\lceil \log_2 m \rceil$ bits with a specific shift to preserve prefix-freeness.

Implement Golomb($m$) with the truncated-binary helper. Show the codewords for $m = 5$ as a small table.

#### D. The Parameter Selection (~250 words + ~30 lines code)

Given a geometric distribution with mean $\mu$, the optimal Golomb parameter is approximately $m^* = -1/\log_2((\mu - 1)/\mu)$ (Gallager and van Voorhis 1975). For Rice (power-of-2 $m$), round to the nearest power of 2.

Implement `optimal_rice_k(mean)` and `optimal_golomb_m(mean)`. Test that the resulting codes' redundancy is small for the given mean.

The takeaway: knowing your data's mean lets you pick an optimal code. This is the first time we see "optimal" become a tunable choice rather than a fixed property.

#### E. Use Cases (~250 words, no code)

Rice coding is the standard for run-length encoding (e.g., in lossless audio codecs like FLAC). The runs of equal values follow a geometric distribution; Rice($k$) with $k$ tuned to the mean run length is optimal.

Golomb is more general but slightly slower. Used in image compression (JPEG-LS) and various bitmap formats.

The pattern: when your data is genuinely geometric and you can estimate the mean, Rice/Golomb beats every other universal code.

#### F. The Connection to Huffman (~250 words, no code)

Rice/Golomb is a *parametric* code; Huffman is a *constructed* code. Rice with optimal $k$ is asymptotically as good as Huffman on a geometric source, but Rice doesn't require building a tree at run time. The trade is: Rice needs you to know the mean in advance; Huffman builds the optimal code from observed frequencies.

Forward to post 9: Huffman as the construction that achieves optimality for any finite distribution.

#### G. Cross-references and footnote (~120 words)

- Forward: [VByte / Varint](/post/2024-02-vbyte-wire-formats/) (post 8) takes a different practical angle: byte-alignment.
- Back: [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3); previous code posts 4, 5, 6.
- Cross-series: both Stepanov bridge posts (Rice's parameter is conceptually the "tag bit width" choice from the Either combinator).
- Footnote: PFC's `include/pfc/codecs.hpp` has `Rice<K>` and `Golomb<M>`.

---

## Post 8: "VByte / Varint"

**Slot weight:** 8
**Date:** 2024-02-25
**Code budget:** ~150 lines (`vbyte.hpp` containing `VByte` codec)
**Prose budget:** ~2000 words
**Opening H2:** "Byte-Aligned Variable-Length Encoding"

### Sections

#### A. The Practical Question (~200 words, no code)

All universal codes seen so far operate at bit granularity. Bit packing is theoretically optimal but computationally expensive: every byte boundary requires shift-and-mask logic. For high-throughput encoding (databases, network protocols, log compression), the overhead of bit packing can exceed the savings from compression.

VByte (also called Varint) trades a small amount of length efficiency for byte-alignment. It's the encoding used by Protocol Buffers, Google's columnar databases, and most production columnar file formats.

#### B. The Encoding (~250 words + ~40 lines code)

VByte splits an integer into 7-bit groups (least significant first). Each group is stored in a byte where:
- Bit 7 (MSB) is a continuation flag: 1 = more bytes follow, 0 = this is the last byte.
- Bits 0-6 hold 7 bits of the integer.

So small integers (0-127) take 1 byte; integers up to $2^{14} - 1$ take 2 bytes; up to $2^{21} - 1$ take 3 bytes; etc.

Implementation:

```cpp
struct VByte {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        while (n >= 128) {
            std::uint8_t byte = static_cast<std::uint8_t>((n & 0x7F) | 0x80);
            for (int i = 0; i < 8; ++i) sink.write(((byte >> i) & 1) != 0);
            n >>= 7;
        }
        std::uint8_t byte = static_cast<std::uint8_t>(n & 0x7F);
        for (int i = 0; i < 8; ++i) sink.write(((byte >> i) & 1) != 0);
    }

    template<BitSource S>
    static value_type decode(S& source) {
        value_type result = 0;
        std::size_t shift = 0;
        while (true) {
            std::uint8_t byte = 0;
            for (int i = 0; i < 8; ++i) {
                if (source.read()) byte |= static_cast<std::uint8_t>(1 << i);
            }
            result |= static_cast<value_type>(byte & 0x7F) << shift;
            if ((byte & 0x80) == 0) break;
            shift += 7;
        }
        return result;
    }
};
```

(Aside: real VByte implementations operate on bytes directly, not bits. This bit-level implementation is for pedagogical consistency with the rest of the series.)

#### C. The Implied Prior (~250 words, no code)

Length analysis: integer $n$ takes $\lceil \log_2(n+1) / 7 \rceil$ bytes, or $8 \lceil \log_2(n+1) / 7 \rceil$ bits.

Implied prior: step-uniform over byte boundaries. Within each byte boundary, all values have the same length. So VByte assigns:
- Each value in $[0, 128)$: $p = 2^{-8}/128 = 2^{-15}$
- Each value in $[128, 16384)$: $p = 2^{-16}/(16384 - 128)$
- ...

Conclusion: VByte is optimal for sources where the *byte length* is geometrically distributed. Real-world sources rarely match this exactly, but VByte is competitive across a wide range of distributions (it's a true "universal" code in the sense that it doesn't fail badly on any realistic source).

#### D. Length Comparison (~250 words, no code)

Comparison table for several values:

| $n$ | VByte length | Gamma length | Delta length |
|---|---|---|---|
| 1 | 8 | 1 | 1 |
| 100 | 8 | 13 | 13 |
| 1000 | 16 | 19 | 16 |
| $2^{20}$ | 24 | 41 | 26 |
| $2^{32}$ | 40 | 65 | 39 |

For large values, VByte is competitive with delta and beats gamma. For small values (1-127), VByte is much worse: a fixed 8-bit cost per value vs gamma's 1-3 bits. This is the price of byte-alignment.

#### E. Why It Wins in Practice (~250 words, no code)

Decoding speed: VByte decodes one byte at a time with simple shift-and-OR logic. Modern CPUs decode VByte at rates of multiple GB/s. Bit-level codes (gamma, delta) decode at maybe 100 MB/s due to bit-shifting overhead.

Memory layout: VByte naturally aligns to byte boundaries, which means random access into encoded streams is cheaper (you can skip whole bytes).

Hardware support: SIMD VByte decoders exist that decode 8-16 values per cycle on AVX2.

The aggregate effect: in production systems where decode throughput dominates, VByte is the default. In archival systems where storage cost dominates, bit-level codes win.

#### F. The Engineering Trade (~250 words, no code)

Frame VByte as the engineer's compromise: theoretically suboptimal, practically dominant. The information-theoretic analysis says gamma is more efficient, but the engineering analysis says the constant factor of bit-vs-byte operations swamps the small length savings.

Note that this pattern recurs throughout systems work: theoretically optimal solutions often lose to implementation-friendly approximations. The bridge from theory to practice is engineering judgment.

#### G. Cross-references and footnote (~120 words)

- Forward: [Huffman](/post/2024-08-huffman-wire-formats/) (post 9) returns to the entropy-optimal regime.
- Back: previous code posts (4, 5, 6, 7).
- Cross-series: not directly relevant; byte-alignment is orthogonal to the type-algebra story.
- Footnote: PFC's `include/pfc/codecs.hpp` has `VByte`. Production Protocol Buffers source (e.g., Google's protobuf-cpp) has the SIMD-optimized version this post does not implement.

---

## Post 9: "Huffman"

**Slot weight:** 9
**Date:** 2024-08-04
**Code budget:** ~280 lines (`huffman.hpp` containing tree building, encoding, decoding, and the optimality proof helpers)
**Prose budget:** ~2200 words
**Opening H2:** "Optimality from Frequency"

**This is one of the larger posts in the series. The implementation alone (priority queue, tree construction, recursive code-table emission, decoding from a tree) is ~250 lines. The optimality proof is non-trivial.**

### Sections

#### A. From Universal to Optimal (~250 words, no code)

All codes seen so far were universal: they work tolerably across many distributions without needing one as input. Huffman is different: given a finite distribution as input, it produces the unique-up-to-symbol-permutation prefix-free code with minimum expected length.

Frame the move: universal codes are defensive (no prior knowledge required, works across many sources). Huffman is offensive (knows the prior, exploits it fully).

#### B. The Algorithm (~350 words + ~120 lines code)

The Huffman algorithm:
1. Place each symbol with its frequency in a min-priority-queue.
2. Repeatedly: extract the two least-frequent items, combine them into a new node whose frequency is their sum, push the combined node back.
3. After $n - 1$ iterations, one node remains: this is the root of the Huffman tree.
4. Each leaf corresponds to a symbol; the path from root to leaf (left = 0, right = 1) is its codeword.

Implement:

```cpp
struct Node {
    double freq;
    int symbol = -1;  // -1 for internal nodes
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
};

inline std::unique_ptr<Node> build_huffman_tree(const std::vector<double>& freqs) {
    auto cmp = [](const std::unique_ptr<Node>& a, const std::unique_ptr<Node>& b) {
        return a->freq > b->freq;
    };
    std::priority_queue<std::unique_ptr<Node>, std::vector<std::unique_ptr<Node>>, decltype(cmp)> pq(cmp);
    for (std::size_t i = 0; i < freqs.size(); ++i) {
        auto node = std::make_unique<Node>();
        node->freq = freqs[i];
        node->symbol = static_cast<int>(i);
        pq.push(std::move(node));
    }
    while (pq.size() > 1) {
        auto a = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();
        auto b = std::move(const_cast<std::unique_ptr<Node>&>(pq.top())); pq.pop();
        auto parent = std::make_unique<Node>();
        parent->freq = a->freq + b->freq;
        parent->left = std::move(a);
        parent->right = std::move(b);
        pq.push(std::move(parent));
    }
    return std::move(const_cast<std::unique_ptr<Node>&>(pq.top()));
}
```

Plus the recursive code-table extraction (walk the tree, accumulate "0"/"1" path string, emit symbol-to-codeword map at each leaf) and the encoder/decoder built from this table.

#### C. Optimality (~350 words, no code)

Sketch the proof that Huffman is optimal: among all prefix-free codes for a finite distribution, no code has smaller expected length than Huffman's.

The proof is a beautiful inductive argument. Lemma 1: in an optimal code, the two least-frequent symbols have the longest (and equal) codewords. Lemma 2: the two least-frequent symbols are siblings (last bit differs by 1). Lemma 3: replacing the two least-frequent symbols with a combined symbol whose frequency is their sum produces a smaller problem whose optimal solution extends to the original problem's optimal solution. Induction completes the proof.

State the proof outcome: Huffman achieves expected length within 1 bit of entropy (since codeword lengths are integers and entropy may be fractional).

#### D. McMillan's Construction Returns (~250 words, no code)

Huffman is a specialized McMillan construction. Recall from post 2: McMillan's construction produces a prefix-free code from any Kraft-satisfying length vector. Huffman selects the *optimal* length vector (the one that minimizes expected length under the given distribution), then runs McMillan's construction.

This connects Huffman back to the foundational results: post 1 (Kraft), post 2 (McMillan), post 3 (length-as-prior). Huffman is "compute the right lengths, then apply McMillan's construction."

#### E. Tests and Demonstrations (~250 words + ~80 lines code)

Tests:
- Round-trip for various distributions (uniform, geometric, Zipf).
- Compare Huffman's expected length to entropy: should be within 1 bit.
- Compare Huffman to gamma on a $1/n^2$ source: Huffman wins by a small constant (because Huffman knows the prior exactly).
- Compare Huffman to optimal-Rice on a geometric source: Huffman matches when the source is dyadic-geometric, slightly better otherwise.
- A worst-case test: Huffman on a uniform distribution of $n$ symbols with $n$ a power of 2 produces the trivial fixed-width code.

#### F. Limitations (~250 words, no code)

Huffman requires the distribution as input. If you don't know it, you must either estimate it from the data (one-pass adaptive Huffman, or two-pass: first pass counts, second encodes) or use a universal code.

Per-symbol cost: Huffman codes one symbol at a time, with integer-bit codewords. The "1 bit above entropy" overhead can be significant for distributions where the entropy is fractional. Arithmetic coding (next post) addresses this by allowing fractional bits.

The block-Huffman trick: encoding tuples of $k$ symbols at a time reduces the redundancy from 1 bit per symbol to 1 bit per block. But the alphabet size grows exponentially in $k$, which limits the practical block size.

Forward to post 10: arithmetic coding solves both problems (no integer-length constraint, and effectively codes blocks of symbols continuously).

#### G. Cross-references and footnote (~120 words)

- Forward: [Arithmetic Coding](/post/2025-01-arithmetic-coding-wire-formats/) (post 10) achieves entropy in the limit.
- Back: [McMillan's Converse](/post/2020-09-mcmillan-wire-formats/) (post 2) is the construction Huffman specializes; [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3) is the framing.
- Cross-series: [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) -- Huffman is the entropy-optimal alternative to the fixed `log2(N)` tag-bit cost the Either combinator pays.
- Footnote: PFC's `include/pfc/huffman.hpp` is the production version with iterative tree construction and the optimal Tunstall-coding-friendly variant.

---

## Post 10: "Arithmetic Coding"

**Slot weight:** 10
**Date:** 2025-01-12
**Code budget:** ~320 lines (`arithmetic_coding.hpp` containing the integer range coder, encoder, decoder, and continuous-length analysis helpers)
**Prose budget:** ~2200 words
**Opening H2:** "From Integer to Continuous Lengths"

**This is the largest post in the implementation budget. The integer range coder requires careful attention to numerical precision, underflow handling, and bit-level details.**

### Sections

#### A. The Last Bit of Redundancy (~250 words, no code)

Huffman achieves "within 1 bit of entropy." Where does that 1 bit go? Huffman codewords are integers; entropy is real-valued. When you have a symbol with probability $0.7$, its optimal codeword length is $-\log_2(0.7) \approx 0.515$ bits. Huffman cannot produce a 0.515-bit codeword; it rounds to either 1 bit (exactly $-\log_2(0.5)$) or 0 bits (impossible; codes need at least 1 bit).

Arithmetic coding produces fractional-bit codewords by codifying a *block* of symbols as a single number in the unit interval. Each symbol shrinks the interval by a factor of $p_i$; after a long block, the interval is so small that specifying any number inside it requires only the entropy times the block length.

#### B. The Continuous View (~250 words, no code)

Imagine the unit interval $[0, 1)$. Subdivide it according to the symbol probabilities: symbol 1 gets $[0, p_1)$, symbol 2 gets $[p_1, p_1 + p_2)$, and so on. Encoding a sequence of symbols means: pick the sub-interval for the first symbol, then within that sub-interval pick the sub-interval for the second symbol, and so on. After encoding $L$ symbols, the cumulative interval has width $\prod p_i$, which has length $-\log_2(\prod p_i) = \sum -\log_2 p_i$ bits.

This is exactly entropy times the sequence length. As $L \to \infty$, the per-symbol cost approaches entropy exactly.

#### C. The Integer Implementation (~400 words + ~200 lines code)

Real arithmetic coders use 32-bit integer arithmetic, not floating-point reals. The high and low bounds of the current interval are tracked as 32-bit integers; when the high bits agree, they're emitted as output and the bounds are renormalized (shifted left, replacing the lost high bits with 0/1 in low/high respectively).

Implement an integer range coder:

```cpp
class ArithmeticEncoder {
    // ... ~120 lines ...
};

class ArithmeticDecoder {
    // ... ~80 lines ...
};
```

The key operations:
- `encode_symbol(low_cum_freq, high_cum_freq, total_freq)`: shrink the interval based on the symbol's cumulative frequencies.
- `renormalize()`: extract bits from the encoder's high bits when they agree.
- `decode_symbol(get_freq_callback)`: inverse: read bits, find the symbol whose cumulative-frequency interval contains the current decoder state.

Underflow handling: when `low < quarter && high > 3*quarter`, neither high bits agree but the interval is shrinking around the midpoint. Track an "underflow count" and emit corrected bits when high bits finally agree.

#### D. Tests (~200 words + ~80 lines code)

Tests:
- Round-trip for various distributions and sequence lengths.
- Verify length per symbol approaches entropy as the sequence grows: encode 1000, 10000, 100000 symbols and measure bits-per-symbol; should approach $H(p)$ asymptotically.
- Compare to Huffman on the same distribution: arithmetic should be no worse, and substantially better when entropy is fractional (e.g., a binary source with $p = 0.9$).

The compelling test: a binary source with $p_0 = 0.99$. Entropy is $H = -0.99 \log_2(0.99) - 0.01 \log_2(0.01) \approx 0.081$ bits per symbol. Huffman cannot compress this below 1 bit per symbol (one of the two symbols must have codeword "0" and the other "1"). Arithmetic coding achieves $\approx 0.082$ bits per symbol on a 1000-symbol stream. A factor of 12 improvement.

#### E. The Adaptive Variant (~250 words, no code)

Arithmetic coding generalizes naturally to adaptive distributions: update the cumulative frequencies after each symbol based on what's been seen. The encoder and decoder maintain identical state, so the adaptation is transparent.

Mention the practical impact: real arithmetic coders (e.g., in modern image compression like JPEG XL, video like AV1) are adaptive; they adjust to the source's empirical distribution as it's processed.

Note the trade with Huffman: adaptive Huffman exists but is more expensive (rebalancing the tree after each symbol). Adaptive arithmetic only updates a frequency table.

#### F. The Theoretical Endpoint (~300 words, no code)

Arithmetic coding achieves the entropy bound in the limit. This is the strongest possible compression theorem for memoryless sources: no prefix-free code can do better, and arithmetic comes asymptotically arbitrarily close.

Discuss what's left: arithmetic coding handles symbol-by-symbol probabilities. Sources with memory (Markov chains, or long-range correlations) require richer models. The standard answer is to use a context-mixing predictor (e.g., the PAQ family of compressors) feeding probabilities into an arithmetic coder. This is how the best lossless compressors as of 2024 are built.

Frame: arithmetic coding is the end of the road for entropy coding. Beyond it, the remaining gains come from better source modeling, not better coding.

#### G. Cross-references and footnote (~120 words)

- Forward: [Succinct Bit Vectors and Rank/Select](/post/2025-06-succinct-wire-formats/) (post 11) shifts from entropy coding to space-efficient data structures.
- Back: [Huffman](/post/2024-08-huffman-wire-formats/) (post 9), [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3).
- Cross-series: [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) -- arithmetic is the entropy-optimal version of the Either combinator's tag bit.
- Footnote: PFC's `include/pfc/arithmetic_coding.hpp` has both the integer range coder and a higher-level adaptive variant.

---

## Post 11: "Succinct Bit Vectors and Rank/Select"

**Slot weight:** 11
**Date:** 2025-06-22
**Code budget:** ~250 lines (`succinct_bitvector.hpp` containing `SuccinctBitVector` with rank, select, and the supporting block/superblock structure)
**Prose budget:** ~2000 words
**Opening H2:** "Constant-Time Queries on Bit Vectors"

### Sections

#### A. The Shift (~200 words, no code)

The first 10 posts focused on encoding: how to represent integers in bits with minimum length. Posts 11 and 12 shift focus to data structures: how to support fast queries on the encoded form.

The motivating queries: given a bit vector $B[0..n-1]$:
- $\text{rank}_1(i)$: how many 1-bits are in $B[0..i-1]$?
- $\text{select}_1(j)$: where is the $j$-th 1-bit in $B$?

Naive: store the bit vector and scan linearly. Space: $n$ bits. Query time: $O(n)$.
Storing all answers explicitly: $O(n)$ values of $O(\log n)$ bits each, so $O(n \log n)$ bits. Query time: $O(1)$.

Succinct: $O(1)$ query time with $n + o(n)$ bits total. The "plus a little bit" is the auxiliary index, asymptotically smaller than the bit vector itself.

#### B. The Structure (~300 words + ~80 lines code)

Standard succinct rank/select implementation:
- Store $B$ as a packed bit array.
- Add a *superblock index*: every $s = \log^2 n$ bits, store the cumulative rank up to that point ($n / s$ entries of $O(\log n)$ bits each, total $O(n / \log n)$ bits).
- Add a *block index*: every $b = (\log n) / 2$ bits within each superblock, store the cumulative rank within the superblock ($n / b$ entries of $O(\log \log n)$ bits each, total $O(n \log \log n / \log n)$ bits).
- For final lookup within a block: use bit-manipulation (popcount on a single word).

Total auxiliary space: $O(n \log \log n / \log n) = o(n)$.

Implement (with reasonable constants for practical use, not asymptotically tight):

```cpp
class SuccinctBitVector {
    std::vector<std::uint64_t> bits_;
    std::vector<std::uint64_t> superblock_ranks_;  // one per 4096 bits
    std::vector<std::uint16_t> block_ranks_;        // one per 64 bits, relative to superblock
    std::size_t n_;

public:
    SuccinctBitVector(const std::vector<bool>& bits) {
        // ... build the index ...
    }

    std::size_t rank1(std::size_t i) const {
        // ... O(1) using superblock + block + popcount ...
    }

    std::size_t select1(std::size_t j) const {
        // ... O(log n) via binary search on superblock + block, or O(1) with extra index ...
    }
};
```

#### C. Why Constant Time (~250 words, no code)

Walk through a rank query: split $i$ into superblock_idx $\cdot$ superblock_size $+$ block_idx $\cdot$ block_size $+$ within_block.

```
rank1(i) = superblock_ranks_[superblock_idx]
         + block_ranks_[block_idx]
         + popcount(bits_[word_idx] & ((1 << within_word) - 1))
```

Three array lookups and a popcount. All constant-time operations. The whole query is $O(1)$.

The trick: precomputing partial answers at logarithmic scales (superblock and block) lets the per-query work be bounded.

#### D. Select via Binary Search (~250 words, no code)

Select is harder than rank. The simple approach: binary search over the superblock array (find the superblock where the $j$-th 1-bit lies), then linear scan within. Total cost: $O(\log n)$ for the binary search.

Constant-time select requires an additional index (the "select samples"), with a sample at every $\log^2 n$-th 1-bit. This is more involved; describe the structure and refer to the survey literature for full details.

For the post's pedagogical implementation: $O(\log n)$ select is sufficient and much simpler. The constant-time version is a trade-off: more space and code complexity for a constant-factor improvement.

#### E. Where Succinct Bit Vectors Show Up (~250 words, no code)

Use cases:
- Inverted indexes: storing document-id sets as compressed bit vectors with rank/select for fast random access.
- Suffix arrays and FM-indexes: rank/select on a transformed string is the core operation.
- Compressed graphs: edge sets stored as bit vectors with rank/select for adjacency-list emulation.
- Wavelet trees (a generalization): storing strings over arbitrary alphabets with constant-time rank/select per character.

The pattern: any time you need to navigate a long bit vector with random-access queries, succinct bit vectors are the answer.

#### F. The Space-Time Trade (~300 words, no code)

Compare three options for representing a bit vector with rank queries:

| Representation | Space | Rank time | Select time |
|---|---|---|---|
| Plain bit array, scan | $n$ bits | $O(n)$ | $O(n)$ |
| Plain + cumulative rank table | $O(n \log n)$ | $O(1)$ | $O(\log n)$ binary search |
| Succinct (this post) | $n + o(n)$ | $O(1)$ | $O(\log n)$ or $O(1)$ |
| Sparse (RLE, gaps) | $O(m \log(n/m))$ where $m$ = popcount | $O(\log m)$ | $O(\log m)$ |

The choice depends on density. Succinct is best when the bit vector is dense (close to half 0s and half 1s). When it's sparse (mostly 0s or mostly 1s), encoding the gaps between 1s with a universal code (gamma, delta) and supporting rank/select via binary search is more space-efficient. This is the bridge to RoaringBitmap (next post): a hybrid that picks between strategies based on density.

#### G. Cross-references and footnote (~120 words)

- Forward: [RoaringBitmap](/post/2025-12-roaring-bitmap-wire-formats/) (post 12) is the hybrid that combines succinct bit vectors with sparse representations.
- Back: [Kraft's Inequality](/post/2020-03-kraft-wire-formats/) (post 1) is the relevant lower bound (succinct bit vectors achieve the information-theoretic minimum space); [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3) frames sparse-representation choice as a prior over densities.
- Cross-series: not directly relevant.
- Footnote: PFC's `include/pfc/succinct.hpp` is the production version with $O(1)$ select via the additional index.

---

## Post 12: "RoaringBitmap"

**Slot weight:** 12
**Date:** 2025-12-07
**Code budget:** ~280 lines (`roaring_bitmap.hpp` containing the three container types and the dispatching layer)
**Prose budget:** ~2000 words
**Opening H2:** "Hybrid Representation as Polyalgorithm"

### Sections

#### A. The Density Question (~200 words, no code)

A bit vector representing a set of integers can be sparse (mostly 0s) or dense (mostly 1s). The optimal representation depends on density:
- Very sparse (< 1% density): list of indices is best.
- Moderately sparse (1-50%): packed bit vector, possibly with run-length encoding.
- Dense (> 50%): packed bit vector, no compression beats it.
- Very dense (> 99%): list of zero-indices is best (representing the negation).

A single representation cannot be optimal across all densities. RoaringBitmap (Lemire et al., 2014) is a hybrid that picks the optimal sub-representation per chunk and stitches them together.

#### B. The Three Container Types (~300 words + ~120 lines code)

RoaringBitmap divides the 32-bit integer space into 64K-integer chunks (high 16 bits select chunk; low 16 bits index within). Each chunk uses one of three representations based on its density:

1. **Array container** (sparse): a sorted list of 16-bit integers. Used when the chunk has $\leq 4096$ elements.
2. **Bitmap container** (dense): a 4096-byte (32768-bit) bit vector. Used when the chunk has $> 4096$ but $< 60000$ elements.
3. **Run container** (clustered): a sorted list of $(start, length)$ pairs encoding runs of consecutive integers. Used when the chunk has many long runs.

Implement each container type with `add`, `contains`, `cardinality`, and a dispatching `RoaringBitmap` class that owns a map from chunk-id to container.

#### C. The Conversion Logic (~250 words + ~50 lines code)

When a chunk's density crosses a threshold, the container converts to the optimal representation:

- Array → Bitmap: when array exceeds 4096 elements (bitmap is now smaller in bits).
- Bitmap → Array: when bitmap drops below 4096 elements (after deletion).
- Either → Run: after an explicit "optimize" pass that detects long runs.

Implement the conversion functions. The conversion cost is amortized over many operations.

#### D. The Operations (~250 words + ~50 lines code)

Implement set-theoretic operations:
- Union: chunk-by-chunk; for each chunk-id present in either, combine the containers (with appropriate type-aware merging).
- Intersection: chunk-by-chunk; only chunks present in both contribute.
- Difference: chunk-by-chunk.

The trick: each chunk's operation can use the optimal algorithm for its container type. Two arrays merge linearly. Two bitmaps merge with bitwise OR. An array and a bitmap merge by adding each array element to the bitmap. The dispatch is per-chunk, per-pair.

#### E. Why It Wins (~250 words, no code)

RoaringBitmap is the dominant compressed bitmap representation in databases (Lucene, Druid, BigCache, ClickHouse, others). Reasons:

1. **Adaptive**: matches the optimal representation per chunk.
2. **Fast**: chunk-based operations parallelize naturally; container operations are SIMD-friendly.
3. **Composable**: union/intersection/difference are fast across mixed densities.
4. **Predictable**: worst case is bounded by the bitmap container (4 KB per 64K integers); best case is much better.

Comparative space: on the canonical "Wikipedia article ID set" benchmark, RoaringBitmap is 50-90% smaller than uncompressed bit vectors and 2-5x faster than alternative compressed bitmap formats (WAH, EWAH, CONCISE).

#### F. The Polyalgorithm Pattern (~250 words, no code)

RoaringBitmap is an instance of the *polyalgorithm* pattern: a single interface dispatches to one of several internal algorithms based on input characteristics. The pattern recurs:
- `std::sort` dispatches to insertion sort (small arrays), quicksort (medium), heapsort (worst-case fallback).
- Hash tables dispatch to open addressing or chaining based on load factor.
- Compilers dispatch register allocators, instruction selectors, and so on.

The lesson for compression and data structures: a single representation rarely dominates across all inputs. The right answer is often an adaptive dispatch.

Connect back to the series's framing: each container type in RoaringBitmap is optimal for a specific *prior over chunk density*. RoaringBitmap is the hybrid that doesn't commit to a prior: it reads density from the data and picks the optimal representation.

#### G. Cross-references and footnote (~120 words)

- Forward: [Synthesis: Codecs as Structure](/post/2026-05-synthesis-wire-formats/) (post 13) closes the arc.
- Back: [Succinct Bit Vectors](/post/2025-06-succinct-wire-formats/) (post 11) is one of the container types Roaring uses; [Universal Codes as Priors](/post/2022-01-priors-wire-formats/) (post 3) frames Roaring as a polyalgorithm over density priors.
- Cross-series: not directly relevant.
- Footnote: PFC's `include/pfc/succinct.hpp` includes a `RoaringBitmap` implementation with the three container types.

---

## Post 13: "Synthesis: Codecs as Structure"

**Slot weight:** 13
**Date:** 2026-05-15
**Code budget:** ~80 lines (small synthesis library: a registry of codecs with their priors, a "best code for distribution" selector)
**Prose budget:** ~2200 words
**Opening H2:** "Codecs as Structure"

**This is the closing meta-post. Less code, more synthesis. Connects back to the Stepanov-side codecs-as-functors framing.**

### Sections

#### A. The Twelve Codes Together (~250 words, no code)

The series presented 12 codes (counting Rice and Golomb separately): Unary, Elias gamma, Elias delta, Elias omega, Fibonacci, Rice, Golomb, VByte, Huffman, Arithmetic, Succinct rank/select, RoaringBitmap.

Recap: each one is the right answer for a specific prior. Each one is wrong (or non-optimal) for the others. The series as a whole is the catalogue of "what code do I use when ..." questions.

#### B. The Unifying Frame Restated (~300 words, no code)

Restate the codes-as-priors thesis from post 3, now with all 12 instances behind us:

1. A code is a hypothesis about the source.
2. Each code corresponds to a specific implicit distribution.
3. Choosing a code is choosing a prior.
4. The "best" code is the one whose implicit prior matches the actual source.
5. When you don't know the source, universal codes give you bounded redundancy across many sources.
6. When you know the source exactly, Huffman or arithmetic coding gives you within-1-bit (or asymptotically zero) redundancy.

Add: when you have a polyalgorithm over data characteristics (RoaringBitmap), you can adapt at the chunk level rather than committing to a global prior.

#### C. The Composition with Type Algebra (~300 words, no code)

Connect back to the Stepanov bridge posts. The codecs-as-functors claim from [Bits Follow Types](/post/2026-05-codecs-functors-stepanov/) said: codecs compose along the algebraic structure of types. The claim from [When Lists Become Bits](/post/2026-05-prefix-free-stepanov/) said: prefix-freeness is the property that lifts the free-monoid construction into bit space.

This series adds the dual: each LEAF codec in the type-algebra composition has its own choice of prior. The Either combinator from the Stepanov bridge had a fixed `log2(N)` tag-bit cost; Huffman or arithmetic on the variant index can do better when branches have unequal probability. The Vec combinator's length prefix uses a universal code (gamma, in the bridge's implementation); a different universal code might be better for a different distribution of vector lengths.

So: the composition of codecs is determined by the algebraic structure of types (Stepanov side); the choice of leaf codec is determined by the prior over the leaf data (wire-formats side). These are two orthogonal axes of design freedom.

#### D. The Codec-Selection Library (~300 words + ~80 lines code)

Implement a small synthesis library: given a sample of integer data, recommend the best universal code. The function fits each candidate code's implied prior to the data, computes redundancy, and picks the minimum.

```cpp
inline std::string recommend_code(const std::vector<std::uint64_t>& sample) {
    // Estimate the empirical distribution.
    auto empirical = empirical_distribution(sample);
    double best_redundancy = std::numeric_limits<double>::infinity();
    std::string best_code;
    for (const auto& candidate : {"Unary", "Gamma", "Delta", "Omega", "Fibonacci", "VByte"}) {
        double r = redundancy_for(candidate, empirical);
        if (r < best_redundancy) {
            best_redundancy = r;
            best_code = candidate;
        }
    }
    return best_code;
}
```

Test cases: geometric data picks Unary or Rice; power-law data picks Gamma or Delta; uniform-byte data picks VByte; data with rare large outliers picks Delta or Omega.

The library makes the selection concrete: there's no "best code in general," but there is a best code given a sample.

#### E. The Six Principles (~350 words, no code)

State six principles distilled from the series, mirroring the "Six Principles" section in Stepanov's `docs/index.md`:

1. **A code is a prior.** Every codeword length implies a probability. Choosing a code is choosing what you believe.
2. **Universality is robustness.** A universal code is one that performs well across many priors, not just one. Use universal codes when you don't know the prior; use Huffman or arithmetic when you do.
3. **Optimality is measurable.** Shannon's source-coding theorem gives the lower bound (entropy); every code's redundancy is measurable as expected-length minus entropy. Pick codes by minimizing redundancy on the actual source.
4. **Engineering trades dominate at scale.** Theoretical optima (gamma, delta, arithmetic) lose to byte-aligned approximations (VByte) when decode throughput matters. Recognize where the binding constraint lives.
5. **Polyalgorithms beat single algorithms.** When source characteristics vary (densities, distributions), adapt per-chunk rather than committing globally. RoaringBitmap is the canonical example.
6. **The algebra of composition is orthogonal to the choice of leaf code.** Type structure dictates how codecs compose; leaf-level priors dictate which codec each leaf gets. These are independent design dimensions.

#### F. What's Next (~250 words, no code)

The series stops at post 13, but the catalogue is not exhaustive. Briefly mention what's beyond:

- Context-mixing predictors (PAQ, ZPAQ) feed adaptive probabilities into arithmetic coders. State-of-the-art for general-purpose lossless compression.
- Asymmetric Numeral Systems (ANS), Duda 2014, achieves arithmetic-coding-quality compression at 5-10x the speed. Underlies LZ4, zstd, and most modern fast compressors.
- Lossy compression (audio, video, images) introduces a different class of trade: rate-distortion theory, where you bound the representation error rather than insisting on bit-exact recovery.
- Quantum-inspired codes (e.g., polar codes, used in 5G) extend the framing in directions that aren't yet textbook.

Frame: this series covered the foundation. The frontier moves on, but the core ideas (Kraft, McMillan, code-as-prior, redundancy bounds) remain the basis.

#### G. Cross-references and footnote (~120 words)

- Back: every post in this series (this is the closing meta-post; link liberally).
- Cross-series: both Stepanov bridge posts. The closing claim ties the two series together: the algebraic structure of types determines the composition of codecs; the prior over leaf data determines the choice of leaf codec.
- Forward: nothing in this series; the series ends here. The Stepanov series may extend; future posts there can reference this series's framework.
- Footnote: PFC ([github.com/queelius/wire-formats/tree/master/lib/pfc](https://github.com/queelius/wire-formats/tree/master/lib/pfc)) is the production library that puts all 12 codes (and several more) into a unified, composable system. This series develops the theory; PFC is the practice.

---

## Per-sub-sub-project deliverables

Each sub-sub-project's plan covers:

### Sub-project 3a: posts 3 + 4

- Scaffold and TDD for `priors.hpp` (post 3) and `unary_gamma.hpp` (post 4).
- Prose drafts for both posts.
- Update `docs/about.md` to mark posts 3 and 4 as Published.
- Update `mkdocs.yml` to add posts 3 and 4 to nav.
- Hugo sync to metafunctor.
- Push.

Estimated: ~22 tasks (similar shape to sub-project 2).

### Sub-project 3b: posts 5 + 6

- Scaffold and TDD for `elias_delta_omega.hpp` and `fibonacci.hpp`.
- Prose drafts for both.
- Update `docs/about.md` and `mkdocs.yml`.
- Hugo sync.
- Push.

Estimated: ~22 tasks.

### Sub-project 3c: posts 7 + 8

- Scaffold and TDD for `rice_golomb.hpp` and `vbyte.hpp`.
- Prose drafts.
- Updates.
- Sync, push.

Estimated: ~22 tasks.

### Sub-project 3d: post 9 (Huffman)

- Scaffold and TDD for `huffman.hpp` (priority queue, tree, encode/decode, optimality tests).
- Prose draft.
- Updates.
- Sync, push.

Estimated: ~16 tasks (one bigger post takes about as much as two smaller ones).

### Sub-project 3e: post 10 (Arithmetic Coding)

- Scaffold and TDD for `arithmetic_coding.hpp` (integer range coder; underflow handling; tests including the binary-source compression demo).
- Prose draft.
- Updates.
- Sync, push.

Estimated: ~18 tasks (slightly bigger due to numerical-precision care).

### Sub-project 3f: posts 11 + 12

- Scaffold and TDD for `succinct_bitvector.hpp` (block + superblock structure) and `roaring_bitmap.hpp` (three container types + dispatcher).
- Prose drafts.
- Updates.
- Sync, push.

Estimated: ~24 tasks (RoaringBitmap is bigger than typical due to three container types).

### Sub-project 3g: post 13 (Synthesis)

- Scaffold and TDD for `synthesis.hpp` (small recommend-code library).
- Prose draft.
- Updates.
- Sync, push.

Estimated: ~10 tasks.

**Total estimate across all 7 sub-sub-projects: ~134 implementation tasks.**

## Acceptance criteria (per sub-sub-project)

For each sub-sub-project, completion means:

1. Each post's directory exists with `index.md`, `<topic>.hpp`, `test_<topic>.cpp`.
2. `make build` succeeds; `make test` passes all tests for the new test executables (no regressions in existing tests).
3. Each `index.md` passes the soul check (no em-dashes).
4. `make docs` (or `mkdocs serve`) renders the new posts cleanly.
5. `docs/about.md` is updated (Forthcoming → Published for the new posts).
6. `mkdocs.yml` includes the new posts in nav.
7. `post/CMakeLists.txt` includes the new test executables.
8. Hugo sync via the FIXED Makefile target propagates the new posts to metafunctor.
9. `mf series scan` shows the wire-formats series with the correct post count.
10. wire-formats and metafunctor are pushed.

## What is NOT in scope

- Posts beyond 13 (the series ends with synthesis; future series, if any, are separate projects).
- Reworking the implementation of universal codes already in PFC (the wire-formats versions are intentionally simpler; PFC remains the production reference).
- Integrating wire-formats and PFC at the source-code level (no shared headers, no submodules; PFC and wire-formats are independent repos).
- Updating the Stepanov series further (the bridge updates are already done in sub-project 2).

## Open questions and deferred decisions

These get resolved during the per-sub-sub-project plans or during implementation:

- **Post 9 (Huffman)**: priority queue choice (`std::priority_queue` with `std::unique_ptr`-friendly comparator vs custom heap). I'd default to `std::priority_queue`; defer the constant-factor tuning.
- **Post 10 (Arithmetic)**: 32-bit vs 64-bit range coder. I'd default to 32-bit (the canonical CACM 1987 implementation); 64-bit has slightly better numerical stability but is non-canonical.
- **Post 11 (Succinct)**: $O(\log n)$ select vs $O(1)$ select. I'd default to $O(\log n)$ for pedagogical simplicity; mention $O(1)$ in prose only.
- **Post 12 (RoaringBitmap)**: 32-bit vs 64-bit integer range. I'd default to 32-bit (matches the canonical Roaring format); 64-bit Roaring exists but is more complex.
- **Post 13 (Synthesis)**: how aggressive should the recommend-code library be? Default to a small registry of universal codes; defer Huffman and arithmetic (which need the distribution as input, not a sample).
- **Per-post date overrides**: the planned dates assume backdating is consistent through the series. If any specific date conflicts with metafunctor content (verify per post before drafting), shift by a few days.

## Implementation sequence (handoff to the 7 plans)

Each sub-sub-project's plan handles its own task decomposition. The general pattern (mirrors sub-project 2):

1. Reconnaissance (verify date non-collision; verify no post-directory conflicts).
2. Scaffold post directories.
3. TDD: implement each component incrementally, one commit per logical step.
4. Composed worked example (where applicable).
5. Prose draft.
6. Update `docs/about.md` and `mkdocs.yml`.
7. Verify clean rebuild + soul check.
8. Hugo sync.
9. User-confirmed push.

The 7 plans go to `wire-formats/docs/superpowers/plans/`:

- `2026-04-24-3a-framing-and-first-codes.md`
- `2026-04-24-3b-elias-extended-and-fibonacci.md`
- `2026-04-24-3c-rice-golomb-and-vbyte.md`
- `2026-04-24-3d-huffman.md`
- `2026-04-24-3e-arithmetic-coding.md`
- `2026-04-24-3f-succinct-and-roaring.md`
- `2026-04-24-3g-synthesis.md`
