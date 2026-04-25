# Algebra over Wire Formats: Sub-sub-project 3a (Posts 3 and 4) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship posts 3 ("Universal Codes as Priors", 2022-01-15) and 4 ("Unary and Elias Gamma", 2022-06-19) of the Algebra over Wire Formats series, including TDD implementations of `priors.hpp` and `unary_gamma.hpp`, full GoogleTest suites, prose drafts, and sync to metafunctor.com.

**Architecture:** Two new post directories under `post/` (`2022-01-priors-wire-formats/` and `2022-06-elias-gamma-wire-formats/`), each with a header, a test file, and `index.md`. Post 3's `priors.hpp` defines the `implied_prior`, `entropy`, `expected_length`, and `redundancy` functions that all subsequent posts (4 through 13) can include as a shared library. Post 4's `unary_gamma.hpp` defines `Unary` and `Gamma` codecs in `namespace unary_gamma`; the implementations match the spec verbatim and are consistent with the PFC production versions in `include/pfc/codecs.hpp`. Both posts wire into the existing `post/CMakeLists.txt`, update `docs/about.md` and `mkdocs.yml`, and sync to metafunctor via the Makefile `sync` target.

**Tech Stack:** C++23, GoogleTest v1.14.0 (already wired in `post/CMakeLists.txt`), mkdocs, the soul plugin's banned-phrase hook.

---

## Spec reference

See `docs/superpowers/specs/2026-04-24-arc-posts-3-through-13.md`, sections "Post 3: Universal Codes as Priors" and "Post 4: Unary and Elias Gamma" for per-section content guides A through G, code budgets, and prose budgets.

## Cross-references note

The Stepanov bridge posts were updated in sub-project 2 (Task 19 of `docs/superpowers/plans/2026-04-24-algebra-over-wire-formats.md`). No further Stepanov changes are needed in this sub-sub-project.

Forward-references to posts 5+ in the prose of posts 3 and 4 remain plain text (no link) since those posts do not exist yet.

Post 4's `unary_gamma.hpp` can draw its `Unary` and `Gamma` implementations directly from the mcmillan post's `prefix_free.hpp` context (the spec notes re-stating the same code is fine; the framing is new). The PFC production reference is `include/pfc/codecs.hpp` structs `Unary` and `EliasGamma`.

---

## Task 1: Reconnaissance (date collision check)

**Files:** read-only.

- [ ] **Step 1: Verify dates 2022-01-15 and 2022-06-19 do not collide with existing metafunctor posts**

```bash
grep -h "^date:" /home/spinoza/github/repos/metafunctor/content/post/*/index.md 2>/dev/null \
  | grep -E "^date: 2022-01-15|^date: 2022-06-19" | sort -u
```

Expected: empty output. If any dates collide, pick adjacent unused days and note them before proceeding.

- [ ] **Step 2: Confirm post 3 and post 4 directories do not yet exist**

```bash
ls /home/spinoza/github/metafunctor-series/wire-formats/post/ | grep -E "2021-03|2021-08"
```

Expected: no output (neither directory exists yet).

- [ ] **Step 3: Confirm the existing CMakeLists ends with McMillan block (so append is safe)**

```bash
tail -8 /home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt
```

Expected: the last `add_test` line belongs to McMillan. No commit for this task.

---

## Task 2: Scaffold post 3 directory and wire CMakeLists

**Files:**
- Create: `post/2022-01-priors-wire-formats/index.md`
- Create: `post/2022-01-priors-wire-formats/priors.hpp`
- Create: `post/2022-01-priors-wire-formats/test_priors.cpp`
- Modify: `post/CMakeLists.txt`

- [ ] **Step 1: Create the post 3 directory and skeleton files**

```bash
mkdir -p /home/spinoza/github/metafunctor-series/wire-formats/post/2022-01-priors-wire-formats
```

Create `post/2022-01-priors-wire-formats/index.md` with placeholder frontmatter:

```markdown
---
title: "Universal Codes as Priors"
date: 2022-01-15
draft: true
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

(Draft in progress. See plan Task 7 for full prose.)
```

Create `post/2022-01-priors-wire-formats/priors.hpp` with header guards only:

```cpp
// priors.hpp
// Pedagogical implementation for the post "Universal Codes as Priors" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc

#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <vector>

namespace priors {

// Implementation arrives in Tasks 3 and 4.

}  // namespace priors
```

Create `post/2022-01-priors-wire-formats/test_priors.cpp` with a placeholder test:

```cpp
#include <gtest/gtest.h>
#include "priors.hpp"

TEST(PriorsTest, Placeholder) {
    EXPECT_TRUE(true);
}
```

- [ ] **Step 2: Append the post 3 test executable to post/CMakeLists.txt**

Append to `/home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt`:

```cmake

# =============================================================================
# Universal Codes as Priors (post 3, 2022-01-15)
# =============================================================================
add_executable(test_priors 2022-01-priors-wire-formats/test_priors.cpp)
target_link_libraries(test_priors GTest::gtest_main)
target_include_directories(test_priors PRIVATE 2022-01-priors-wire-formats)
add_test(NAME test_priors COMMAND test_priors)
```

- [ ] **Step 3: Build and verify placeholder test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -10
```

Expected: all previous tests pass plus the new `test_priors.Placeholder` test. Output should include `[  PASSED  ] 1 test.` for test_priors.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-01-priors-wire-formats post/CMakeLists.txt
git commit -m "scaffold(priors): add post 3 directory and CMake wiring"
```

---

## Task 3: TDD -- implement `implied_prior`

**Files:**
- Modify: `post/2022-01-priors-wire-formats/priors.hpp`
- Modify: `post/2022-01-priors-wire-formats/test_priors.cpp`

- [ ] **Step 1: Write failing tests for `implied_prior`**

Replace `test_priors.cpp` with:

```cpp
#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "priors.hpp"

using namespace priors;

// Helper: check that two double vectors are element-wise close.
static void expect_close(const std::vector<double>& a,
                         const std::vector<double>& b,
                         double tol = 1e-9) {
    ASSERT_EQ(a.size(), b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        EXPECT_NEAR(a[i], b[i], tol) << "index " << i;
    }
}

// Unary lengths for symbols 1..K: l_i = i.
static std::vector<std::size_t> unary_lengths(std::size_t K) {
    std::vector<std::size_t> v(K);
    for (std::size_t i = 0; i < K; ++i) v[i] = i + 1;
    return v;
}

// Gamma lengths for symbols 1..N: l_n = 2*floor(log2(n)) + 1.
static std::vector<std::size_t> gamma_lengths(std::size_t N) {
    std::vector<std::size_t> v(N);
    for (std::size_t i = 0; i < N; ++i) {
        std::size_t n = i + 1;
        std::size_t k = 0;
        std::size_t tmp = n;
        while (tmp > 1) { tmp >>= 1; ++k; }
        v[i] = 2 * k + 1;
    }
    return v;
}

// Unary implied prior should be geometric(1/2): p_i = 2^{-i} for i = 1..K,
// normalized because the truncated sum < 1.
TEST(PriorsTest, ImpliedPriorUnaryIsGeometricHalf) {
    // For unary lengths {1, 2, 3, 4}, Kraft sum = 1/2 + 1/4 + 1/8 + 1/16 = 15/16.
    // Normalization: p_i = (2^{-i}) / (15/16) = 16/(15 * 2^i).
    auto lengths = unary_lengths(4);
    auto probs = implied_prior(lengths);
    ASSERT_EQ(probs.size(), 4u);
    double total = 0.0;
    for (double p : probs) total += p;
    EXPECT_NEAR(total, 1.0, 1e-12);
    // Each successive probability should be half the previous (geometric ratio).
    for (std::size_t i = 1; i < probs.size(); ++i) {
        EXPECT_NEAR(probs[i], probs[i - 1] / 2.0, 1e-12) << "index " << i;
    }
}

// For a Kraft-saturating code (e.g., fixed-width 2-bit: lengths {2,2,2,2}),
// implied prior should be uniform: each prob = 0.25.
TEST(PriorsTest, ImpliedPriorSaturatingCodeIsUniform) {
    std::vector<std::size_t> lengths = {2, 2, 2, 2};
    auto probs = implied_prior(lengths);
    ASSERT_EQ(probs.size(), 4u);
    for (double p : probs) {
        EXPECT_NEAR(p, 0.25, 1e-12);
    }
}

// Gamma implied prior: for symbols 1..N, should be approximately p_n ~ 1/n^2.
// We verify the ratio p_1 / p_n is approximately n^2 (with a factor-of-2
// deviation for non-power-of-2 n values, which is expected from gamma's
// block structure).
TEST(PriorsTest, ImpliedPriorGammaApproxPowerLaw2) {
    auto lengths = gamma_lengths(16);
    auto probs = implied_prior(lengths);
    ASSERT_EQ(probs.size(), 16u);
    // p_1 is the reference. Each p_n should be within a factor of 4 of p_1/n^2.
    double p1 = probs[0];
    for (std::size_t i = 1; i < probs.size(); ++i) {
        double n = static_cast<double>(i + 1);
        double expected = p1 / (n * n);
        // Allow factor-of-4 deviation (gamma assigns same length to whole block).
        EXPECT_GT(probs[i], expected / 4.0) << "n=" << n;
        EXPECT_LT(probs[i], expected * 4.0) << "n=" << n;
    }
}

// Probabilities must sum to 1.
TEST(PriorsTest, ImpliedPriorSumsToOne) {
    auto lengths = gamma_lengths(32);
    auto probs = implied_prior(lengths);
    double total = 0.0;
    for (double p : probs) total += p;
    EXPECT_NEAR(total, 1.0, 1e-9);
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `implied_prior` not being declared.

- [ ] **Step 3: Implement `implied_prior` in `priors.hpp`**

Replace the `// Implementation arrives in Tasks 3 and 4.` comment in `priors.hpp` with:

```cpp
// ---- implied_prior -- the core correspondence: lengths -> probabilities ------
//
// For a prefix-free code with codeword lengths (l_1, ..., l_n), the implicit
// probability of symbol i is 2^{-l_i}. If the lengths do not saturate Kraft
// (sum < 1), normalize so the probabilities sum to 1.
//
// This is the inverse of Shannon's prescription: given p_i, the optimal length
// is -log2(p_i). Going backward: given a length l_i, the implied probability is
// 2^{-l_i}.

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

- [ ] **Step 4: Build and run to verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_priors"
```

Expected: all `PriorsTest` cases pass (ImpliedPriorUnaryIsGeometricHalf, ImpliedPriorSaturatingCodeIsUniform, ImpliedPriorGammaApproxPowerLaw2, ImpliedPriorSumsToOne).

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-01-priors-wire-formats/priors.hpp \
        post/2022-01-priors-wire-formats/test_priors.cpp
git commit -m "feat(priors): implement implied_prior with Kraft normalization (TDD)"
```

---

## Task 4: TDD -- implement `entropy`, `expected_length`, `redundancy`

**Files:**
- Modify: `post/2022-01-priors-wire-formats/priors.hpp`
- Modify: `post/2022-01-priors-wire-formats/test_priors.cpp`

- [ ] **Step 1: Append failing tests for Shannon functions**

Append to `test_priors.cpp` (after the existing tests):

```cpp
// entropy of a uniform distribution over K symbols = log2(K).
TEST(PriorsTest, EntropyUniform) {
    std::vector<double> probs = {0.25, 0.25, 0.25, 0.25};
    EXPECT_NEAR(entropy(probs), 2.0, 1e-12);
}

// entropy of a degenerate distribution (one symbol certain) = 0.
TEST(PriorsTest, EntropyDegenerate) {
    std::vector<double> probs = {1.0};
    EXPECT_NEAR(entropy(probs), 0.0, 1e-12);
}

// entropy of geometric(1/2) truncated to K terms.
// H = sum_{n=1}^{K} p_n * n (where p_n = 2^{-n} / Z, Z = 1 - 2^{-K}).
// For K=8 this is close to 2 bits.
TEST(PriorsTest, EntropyGeometricHalf) {
    auto lengths = unary_lengths(8);
    auto probs = implied_prior(lengths);
    double h = entropy(probs);
    EXPECT_GT(h, 1.5);
    EXPECT_LT(h, 3.0);
}

// expected_length: for uniform 2-bit code, expected length = 2.
TEST(PriorsTest, ExpectedLengthUniform2Bit) {
    std::vector<double> probs = {0.25, 0.25, 0.25, 0.25};
    std::vector<std::size_t> lengths = {2, 2, 2, 2};
    EXPECT_NEAR(expected_length(probs, lengths), 2.0, 1e-12);
}

// expected_length >= entropy (Shannon's theorem).
TEST(PriorsTest, ExpectedLengthAtLeastEntropy) {
    auto lengths = gamma_lengths(32);
    auto probs = implied_prior(lengths);
    double h = entropy(probs);
    double L = expected_length(probs, lengths);
    EXPECT_GE(L, h - 1e-9);
}

// redundancy = expected_length - entropy >= 0 always.
TEST(PriorsTest, RedundancyNonNegative) {
    auto lengths = gamma_lengths(64);
    auto probs = implied_prior(lengths);
    EXPECT_GE(redundancy(probs, lengths), -1e-9);
}

// redundancy = 0 when the code is exactly optimal (dyadic distribution).
// lengths {1, 2, 2} with probs {0.5, 0.25, 0.25}: entropy = 1.5, expected = 1.5.
TEST(PriorsTest, RedundancyZeroForDyadicOptimal) {
    std::vector<std::size_t> lengths = {1, 2, 2};
    std::vector<double> probs = {0.5, 0.25, 0.25};
    EXPECT_NEAR(redundancy(probs, lengths), 0.0, 1e-12);
}

// For unary code on its own geometric(1/2) prior, redundancy should be
// essentially 0 (unary is exactly optimal for this prior).
TEST(PriorsTest, RedundancyUnaryOnGeometricPrior) {
    auto lengths = unary_lengths(20);
    auto probs = implied_prior(lengths);
    EXPECT_NEAR(redundancy(probs, lengths), 0.0, 1e-6);
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `entropy`, `expected_length`, `redundancy` not declared.

- [ ] **Step 3: Implement Shannon functions in `priors.hpp`**

Append to `priors.hpp` (after the `implied_prior` function, still inside `namespace priors`):

```cpp
// ---- entropy -- Shannon entropy of a distribution --------------------------
//
// H(p) = -sum_i p_i * log2(p_i)
// Zero-probability symbols contribute 0 (by convention: 0 * log 0 = 0).

inline double entropy(const std::vector<double>& probs) {
    double h = 0.0;
    for (double p : probs) {
        if (p > 0.0) h -= p * std::log2(p);
    }
    return h;
}

// ---- expected_length -- average codeword length under a distribution --------
//
// L(p, l) = sum_i p_i * l_i
// This is the expected bits-per-symbol when encoding from distribution p
// using a code with lengths l.

inline double expected_length(const std::vector<double>& probs,
                              const std::vector<std::size_t>& lengths) {
    assert(probs.size() == lengths.size());
    double L = 0.0;
    for (std::size_t i = 0; i < probs.size(); ++i) {
        L += probs[i] * static_cast<double>(lengths[i]);
    }
    return L;
}

// ---- redundancy -- excess bits beyond Shannon optimum ----------------------
//
// R(p, l) = L(p, l) - H(p) >= 0  (Shannon's source-coding theorem).
// Equality holds iff l_i = -log2(p_i) for all i and the Kraft sum equals 1.

inline double redundancy(const std::vector<double>& probs,
                         const std::vector<std::size_t>& lengths) {
    return expected_length(probs, lengths) - entropy(probs);
}
```

- [ ] **Step 4: Build and verify all tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_priors"
```

Expected: all PriorsTest cases pass (11 tests total after this task).

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-01-priors-wire-formats/priors.hpp \
        post/2022-01-priors-wire-formats/test_priors.cpp
git commit -m "feat(priors): implement entropy, expected_length, redundancy (TDD)"
```

---

## Task 5: Composed integration tests for post 3's library

**Files:**
- Modify: `post/2022-01-priors-wire-formats/test_priors.cpp`

These tests exercise the full library (implied_prior, entropy, expected_length, redundancy) together on specific named distributions to verify the spec's "two concrete examples" (section D) and "table of priors" (section E) claims hold numerically.

- [ ] **Step 1: Append integration tests**

Append to `test_priors.cpp`:

```cpp
// Integration: geometric(1/2) source + unary code achieves entropy exactly.
// This verifies section D claim: "unary achieves entropy for geometric(1/2)."
TEST(PriorsTest, UnaryAchievesEntropyForGeometricPrior) {
    // Use K=30 terms; the truncation tail is < 2^{-30}.
    const std::size_t K = 30;
    auto lengths = unary_lengths(K);
    // Build geometric(1/2) truncated to K terms.
    std::vector<double> geo(K);
    double z = 0.0;
    for (std::size_t i = 0; i < K; ++i) {
        geo[i] = std::ldexp(1.0, -static_cast<int>(i + 1));
        z += geo[i];
    }
    for (double& p : geo) p /= z;
    double h = entropy(geo);
    double L = expected_length(geo, lengths);
    // The code is exactly optimal for its own implied prior (which is this geo).
    // After normalization the two should match very closely.
    EXPECT_NEAR(L, h, 0.01);
}

// Integration: power-law source + gamma code has small redundancy.
// This verifies section D claim: "gamma is approximately optimal for 1/n^2."
TEST(PriorsTest, GammaSmallRedundancyForPowerLaw2) {
    const std::size_t N = 64;
    auto lengths = gamma_lengths(N);
    // Build power-law(2): p_n = C/n^2, normalized.
    std::vector<double> pl(N);
    double z = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        double n = static_cast<double>(i + 1);
        pl[i] = 1.0 / (n * n);
        z += pl[i];
    }
    for (double& p : pl) p /= z;
    double r = redundancy(pl, lengths);
    // Universal codes have bounded redundancy (constant additive overhead);
    // for gamma on a power-law(2) source, redundancy should be < 3 bits.
    EXPECT_GE(r, 0.0);
    EXPECT_LT(r, 3.0);
}

// Sanity: a sub-optimal code (unary applied to power-law source) has higher
// redundancy than the right code (gamma on the same source).
TEST(PriorsTest, GammaBeatsUnaryOnPowerLawSource) {
    const std::size_t N = 64;
    auto gamma_lens = gamma_lengths(N);
    auto unary_lens = unary_lengths(N);
    // Build power-law(2) source.
    std::vector<double> pl(N);
    double z = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        double n = static_cast<double>(i + 1);
        pl[i] = 1.0 / (n * n);
        z += pl[i];
    }
    for (double& p : pl) p /= z;
    double r_gamma = redundancy(pl, gamma_lens);
    double r_unary = redundancy(pl, unary_lens);
    EXPECT_LT(r_gamma, r_unary);
}
```

- [ ] **Step 2: Build and verify**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_priors"
```

Expected: all PriorsTest cases pass (14 tests after this task).

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-01-priors-wire-formats/test_priors.cpp
git commit -m "test(priors): add composed integration tests (geometric/gamma/power-law)"
```

---

## Task 6: Verify post 3 full-suite pass (clean rebuild + warning check)

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make build 2>&1 | tail -5
```

Expected: successful clean build with no errors.

- [ ] **Step 2: Warning check for priors**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
cmake -B post/build -S post -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic" 2>&1 | grep -E "warning:|error:" | grep "priors" | head -20
cmake --build post/build --target test_priors 2>&1 | grep -E "warning:|error:" | head -20
```

Expected: zero warnings and zero errors from priors.hpp and test_priors.cpp.

- [ ] **Step 3: Full ctest pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -8
```

Expected: all tests pass (test_kraft, test_mcmillan, test_priors).

No commit for this task (verification only).

---

## Task 7: Draft post 3 prose

**Files:**
- Modify: `post/2022-01-priors-wire-formats/index.md`

Draft from the spec's sections A through G for Post 3. The target is approximately 2000 words. Key requirement: no em-dashes anywhere in the file (the soul hook will block writes that contain them).

- [ ] **Step 1: Draft the prose**

Replace the placeholder `(Draft in progress...)` line in `post/2022-01-priors-wire-formats/index.md` with the full article body. Use the spec's section-by-section outline:

Section A ("The Question", ~200 words): introduce the source-model selection framing. Any integer-compressing code is a bet about which integers show up most. Foreshadow the thesis.

Section B ("The Correspondence: Lengths to Priors", ~300 words + `implied_prior` code block): present the length-to-probability map, show `implied_prior`, explain Kraft normalization.

Section C ("Optimality: Shannon's Theorem and Expected Length", ~300 words + `entropy`, `expected_length`, `redundancy` code blocks): state Shannon's theorem, show the three functions, illustrate redundancy >= 0.

Section D ("Two Concrete Examples: Unary and Gamma", ~300 words + length-vector helper code): walk through both examples with actual numbers. Show that unary achieves entropy exactly on its own prior; gamma has bounded redundancy on power-law sources.

Section E ("The Table of Priors", ~250 words): include the table from the spec mapping each code to its implied prior.

Section F ("Universality", ~250 words): minimax framing, class-of-distributions interpretation, forward pointer to posts 4-8 (universal codes) and 9-10 (entropy-optimal).

Section G ("Cross-references and footnote", ~120 words): include the exact cross-reference set from the spec. Forward link to post 4 (plain link since it will exist after Task 14). Backward links to posts 1 and 2 (live links). Cross-series links to both Stepanov bridge posts. PFC footnote pointing to `include/pfc/codecs.hpp`.

Set `draft: false` in the frontmatter when satisfied with the draft.

- [ ] **Step 2: Soul check (banned-phrase hook)**

```bash
check-banned-phrases.sh /home/spinoza/github/metafunctor-series/wire-formats/post/2022-01-priors-wire-formats/index.md 2>&1
```

Expected: no banned phrases found. If the soul hook is not available as a standalone command, run:

```bash
grep -n $'\xe2\x80\x94' /home/spinoza/github/metafunctor-series/wire-formats/post/2022-01-priors-wire-formats/index.md | head -5
```

Expected: no output (no em-dashes in the file).

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-01-priors-wire-formats/index.md
git commit -m "docs(priors): draft post 3 prose (Universal Codes as Priors)"
```

---

## Task 8: Scaffold post 4 directory and wire CMakeLists

**Files:**
- Create: `post/2022-06-elias-gamma-wire-formats/index.md`
- Create: `post/2022-06-elias-gamma-wire-formats/unary_gamma.hpp`
- Create: `post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp`
- Modify: `post/CMakeLists.txt`

- [ ] **Step 1: Create the post 4 directory and skeleton files**

```bash
mkdir -p /home/spinoza/github/metafunctor-series/wire-formats/post/2022-06-elias-gamma-wire-formats
```

Create `post/2022-06-elias-gamma-wire-formats/index.md` with placeholder frontmatter:

```markdown
---
title: "Unary and Elias Gamma"
date: 2022-06-19
draft: true
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

(Draft in progress. See plan Task 14 for full prose.)
```

Create `post/2022-06-elias-gamma-wire-formats/unary_gamma.hpp` with header guards only:

```cpp
// unary_gamma.hpp
// Pedagogical implementation for the post "Unary and Elias Gamma" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc (codecs.hpp: Unary, EliasGamma)

#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace unary_gamma {

// BitSink and BitSource concepts (minimal local definitions for self-contained
// pedagogical use; production code uses pfc/core.hpp).

template<typename S>
concept BitSink = requires(S& s, bool b) {
    { s.write(b) } -> std::same_as<void>;
};

template<typename S>
concept BitSource = requires(S& s) {
    { s.read() } -> std::same_as<bool>;
};

// Implementation arrives in Tasks 9 and 10.

}  // namespace unary_gamma
```

Create `post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp` with a placeholder:

```cpp
#include <gtest/gtest.h>
#include "unary_gamma.hpp"

TEST(UnaryGammaTest, Placeholder) {
    EXPECT_TRUE(true);
}
```

- [ ] **Step 2: Append the post 4 test executable to post/CMakeLists.txt**

Append to `/home/spinoza/github/metafunctor-series/wire-formats/post/CMakeLists.txt`:

```cmake

# =============================================================================
# Unary and Elias Gamma (post 4, 2022-06-19)
# =============================================================================
add_executable(test_unary_gamma 2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp)
target_link_libraries(test_unary_gamma GTest::gtest_main)
target_include_directories(test_unary_gamma PRIVATE 2022-06-elias-gamma-wire-formats)
add_test(NAME test_unary_gamma COMMAND test_unary_gamma)
```

- [ ] **Step 3: Build and verify placeholder test passes**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -10
```

Expected: all existing tests pass plus the new `test_unary_gamma.Placeholder` test.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-06-elias-gamma-wire-formats post/CMakeLists.txt
git commit -m "scaffold(unary-gamma): add post 4 directory and CMake wiring"
```

---

## Task 9: TDD -- implement `Unary` codec

**Files:**
- Modify: `post/2022-06-elias-gamma-wire-formats/unary_gamma.hpp`
- Modify: `post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp`

- [ ] **Step 1: Write failing tests for `Unary`**

Replace `test_unary_gamma.cpp` with:

```cpp
#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include <vector>
#include "unary_gamma.hpp"

using namespace unary_gamma;

// Minimal in-memory BitSink for tests.
struct BitBuffer {
    std::vector<bool> bits;
    void write(bool b) { bits.push_back(b); }
    bool read() {
        bool b = bits[pos_]; ++pos_; return b;
    }
    std::size_t pos_ = 0;
};

// Helper: encode then decode n, check round-trip.
static uint64_t unary_round_trip(uint64_t n) {
    BitBuffer buf;
    Unary::encode(n, buf);
    buf.pos_ = 0;
    return Unary::decode(buf);
}

// Helper: return the bit count emitted for n.
static std::size_t unary_bit_count(uint64_t n) {
    BitBuffer buf;
    Unary::encode(n, buf);
    return buf.bits.size();
}

TEST(UnaryGammaTest, UnaryRoundTrip1) {
    EXPECT_EQ(unary_round_trip(1), 1u);
}

TEST(UnaryGammaTest, UnaryRoundTrip2) {
    EXPECT_EQ(unary_round_trip(2), 2u);
}

TEST(UnaryGammaTest, UnaryRoundTripLarge) {
    for (uint64_t n = 1; n <= 20; ++n) {
        EXPECT_EQ(unary_round_trip(n), n) << "n=" << n;
    }
}

// Codeword for n has exactly n bits.
TEST(UnaryGammaTest, UnaryBitCount) {
    for (uint64_t n = 1; n <= 20; ++n) {
        EXPECT_EQ(unary_bit_count(n), n) << "n=" << n;
    }
}

// Codeword for n=1 is a single '1' bit.
TEST(UnaryGammaTest, UnaryEncoding1IsSingleOne) {
    BitBuffer buf;
    Unary::encode(uint64_t{1}, buf);
    ASSERT_EQ(buf.bits.size(), 1u);
    EXPECT_EQ(buf.bits[0], true);
}

// Codeword for n=3 is "001": two zeros then a one.
TEST(UnaryGammaTest, UnaryEncoding3IsTwoZerosOneOne) {
    BitBuffer buf;
    Unary::encode(uint64_t{3}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], false);
    EXPECT_EQ(buf.bits[2], true);
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `Unary` not declared.

- [ ] **Step 3: Implement `Unary` in `unary_gamma.hpp`**

Replace the `// Implementation arrives in Tasks 9 and 10.` comment with:

```cpp
// ---- Unary -- the simplest universal code -----------------------------------
//
// Encodes positive integer n >= 1 as (n-1) zero bits followed by one '1' bit.
// Examples: 1 -> "1", 2 -> "01", 3 -> "001", 4 -> "0001".
//
// Length of codeword for n: n bits.
// Kraft sum: sum_{n=1}^{inf} 2^{-n} = 1 (saturates).
// Implied prior: p_n = 2^{-n} (geometric distribution with parameter 1/2).
// Optimal source: geometric(1/2), i.e., each value is half as likely as the
//                 previous. Unary achieves entropy exactly on this prior.

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

- [ ] **Step 4: Build and verify tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_unary_gamma"
```

Expected: all UnaryGammaTest cases up to this point pass.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-06-elias-gamma-wire-formats/unary_gamma.hpp \
        post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp
git commit -m "feat(unary-gamma): implement Unary codec (TDD)"
```

---

## Task 10: TDD -- implement `Gamma` codec

**Files:**
- Modify: `post/2022-06-elias-gamma-wire-formats/unary_gamma.hpp`
- Modify: `post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp`

- [ ] **Step 1: Append failing tests for `Gamma`**

Append to `test_unary_gamma.cpp`:

```cpp
// Helper: encode then decode n via Gamma, check round-trip.
static uint64_t gamma_round_trip(uint64_t n) {
    BitBuffer buf;
    Gamma::encode(n, buf);
    buf.pos_ = 0;
    return Gamma::decode(buf);
}

// Helper: return bit count for Gamma(n).
static std::size_t gamma_bit_count(uint64_t n) {
    BitBuffer buf;
    Gamma::encode(n, buf);
    return buf.bits.size();
}

TEST(UnaryGammaTest, GammaRoundTrip) {
    for (uint64_t n = 1; n <= 256; ++n) {
        EXPECT_EQ(gamma_round_trip(n), n) << "n=" << n;
    }
}

// Length of Gamma(n) = 2*floor(log2(n)) + 1.
TEST(UnaryGammaTest, GammaBitCount) {
    for (uint64_t n = 1; n <= 128; ++n) {
        std::size_t k = 0;
        uint64_t tmp = n;
        while (tmp > 1) { tmp >>= 1; ++k; }
        std::size_t expected = 2 * k + 1;
        EXPECT_EQ(gamma_bit_count(n), expected) << "n=" << n;
    }
}

// Spot-check specific encodings from the spec.
// 1 -> "1", 2 -> "010", 3 -> "011", 4 -> "00100", 8 -> "0001000"
TEST(UnaryGammaTest, GammaEncoding1IsSingleOne) {
    BitBuffer buf;
    Gamma::encode(uint64_t{1}, buf);
    ASSERT_EQ(buf.bits.size(), 1u);
    EXPECT_EQ(buf.bits[0], true);
}

TEST(UnaryGammaTest, GammaEncoding2Is010) {
    BitBuffer buf;
    Gamma::encode(uint64_t{2}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], false);
}

TEST(UnaryGammaTest, GammaEncoding3Is011) {
    BitBuffer buf;
    Gamma::encode(uint64_t{3}, buf);
    ASSERT_EQ(buf.bits.size(), 3u);
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], true);
    EXPECT_EQ(buf.bits[2], true);
}

TEST(UnaryGammaTest, GammaEncoding4Is00100) {
    BitBuffer buf;
    Gamma::encode(uint64_t{4}, buf);
    ASSERT_EQ(buf.bits.size(), 5u);
    // "00100": leading zeros count = 2, then the 3-bit binary for 4
    EXPECT_EQ(buf.bits[0], false);
    EXPECT_EQ(buf.bits[1], false);
    EXPECT_EQ(buf.bits[2], true);
    EXPECT_EQ(buf.bits[3], false);
    EXPECT_EQ(buf.bits[4], false);
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: compile errors about `Gamma` not declared.

- [ ] **Step 3: Implement `Gamma` in `unary_gamma.hpp`**

Append to `unary_gamma.hpp` (after the `Unary` struct, still inside `namespace unary_gamma`):

```cpp
// ---- Gamma -- Elias gamma code (Peter Elias, 1975) -------------------------
//
// Encodes positive integer n >= 1:
//   1. Write floor(log2(n)) zero bits.
//   2. Write a '1' bit.
//   3. Write the binary representation of n minus its leading 1 bit (MSB first),
//      using floor(log2(n)) bits.
//
// Examples: 1->"1", 2->"010", 3->"011", 4->"00100", 5->"00101",
//           6->"00110", 7->"00111", 8->"0001000".
//
// Length: 2*floor(log2(n)) + 1 bits.
// Kraft sum: sum_{k=0}^{inf} 2^k * 2^{-(2k+1)} = sum_{k=0}^{inf} 2^{-(k+1)} = 1.
// Implied prior: p_n = 2^{-(2*floor(log2(n))+1)}, approximately 1/(2n^2).
// Optimal source: power-law with exponent ~2 (e.g., word frequencies).

struct Gamma {
    using value_type = std::uint64_t;

    template<BitSink S>
    static void encode(value_type n, S& sink) {
        assert(n >= 1 && "Gamma is undefined for n = 0");
        // k = floor(log2(n)): number of leading zeros and number of trailing bits.
        std::size_t k = std::bit_width(n) - 1;
        // Write k zeros.
        for (std::size_t i = 0; i < k; ++i) sink.write(false);
        // Write the one separator bit.
        sink.write(true);
        // Write the k trailing bits of n (after the implicit leading 1), MSB first.
        for (std::size_t i = k; i > 0; --i) {
            sink.write(((n >> (i - 1)) & 1u) != 0u);
        }
    }

    template<BitSource S>
    static value_type decode(S& source) {
        // Count leading zeros to get k.
        std::size_t k = 0;
        while (!source.read()) ++k;
        // Read k more bits to reconstruct n (starting from the implicit leading 1).
        value_type n = 1;
        for (std::size_t i = 0; i < k; ++i) {
            n = (n << 1) | (source.read() ? value_type{1} : value_type{0});
        }
        return n;
    }
};
```

- [ ] **Step 4: Build and verify all tests pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_unary_gamma"
```

Expected: all UnaryGammaTest cases pass.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-06-elias-gamma-wire-formats/unary_gamma.hpp \
        post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp
git commit -m "feat(unary-gamma): implement Gamma (Elias gamma) codec (TDD)"
```

---

## Task 11: TDD -- length-vector helpers `unary_lengths` and `gamma_lengths`

**Files:**
- Modify: `post/2022-06-elias-gamma-wire-formats/unary_gamma.hpp`
- Modify: `post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp`

These helpers are used by the integration tests (Task 12) and also serve as explicit documentation of the length functions embedded in the post prose.

- [ ] **Step 1: Append failing tests for helper functions**

Append to `test_unary_gamma.cpp`:

```cpp
// unary_lengths(K) returns {1, 2, 3, ..., K}.
TEST(UnaryGammaTest, UnaryLengthsVector) {
    auto v = unary_lengths(5);
    ASSERT_EQ(v.size(), 5u);
    for (std::size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(v[i], i + 1) << "i=" << i;
    }
}

// gamma_lengths(N) returns the Gamma codeword lengths for 1..N.
TEST(UnaryGammaTest, GammaLengthsVector) {
    // Spot-check: gamma(1)=1, gamma(2)=3, gamma(3)=3, gamma(4)=5, gamma(8)=7.
    auto v = gamma_lengths(8);
    ASSERT_EQ(v.size(), 8u);
    EXPECT_EQ(v[0], 1u);  // n=1
    EXPECT_EQ(v[1], 3u);  // n=2
    EXPECT_EQ(v[2], 3u);  // n=3
    EXPECT_EQ(v[3], 5u);  // n=4
    EXPECT_EQ(v[4], 5u);  // n=5
    EXPECT_EQ(v[5], 5u);  // n=6
    EXPECT_EQ(v[6], 5u);  // n=7
    EXPECT_EQ(v[7], 7u);  // n=8
}

// gamma_lengths agrees with the bit-count measured by Gamma::encode.
TEST(UnaryGammaTest, GammaLengthsMatchEncode) {
    auto v = gamma_lengths(32);
    for (std::size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(v[i], gamma_bit_count(static_cast<uint64_t>(i + 1))) << "n=" << (i+1);
    }
}
```

- [ ] **Step 2: Run to verify compile failure**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make build 2>&1 | grep -E "error:|undefined" | head -10
```

Expected: errors about `unary_lengths` and `gamma_lengths` not declared in `unary_gamma` namespace.

- [ ] **Step 3: Add helper functions to `unary_gamma.hpp`**

Append to `unary_gamma.hpp` (after `Gamma`, still inside `namespace unary_gamma`):

```cpp
// ---- Length-vector generators (pedagogical helpers) ------------------------
//
// These generate the codeword-length vectors for Unary and Gamma for use
// with the priors library (implied_prior, entropy, expected_length, redundancy).

// unary_lengths(K): lengths for symbols 1..K under Unary coding.
// l_n = n for n = 1..K.
inline std::vector<std::size_t> unary_lengths(std::size_t K) {
    std::vector<std::size_t> v(K);
    for (std::size_t i = 0; i < K; ++i) v[i] = i + 1;
    return v;
}

// gamma_lengths(N): lengths for symbols 1..N under Gamma coding.
// l_n = 2*floor(log2(n)) + 1.
inline std::vector<std::size_t> gamma_lengths(std::size_t N) {
    std::vector<std::size_t> v(N);
    for (std::size_t i = 0; i < N; ++i) {
        std::size_t n = i + 1;
        std::size_t k = std::bit_width(n) - 1;  // floor(log2(n))
        v[i] = 2 * k + 1;
    }
    return v;
}
```

- [ ] **Step 4: Build and verify**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_unary_gamma"
```

Expected: all UnaryGammaTest cases pass including the three new helper tests.

- [ ] **Step 5: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-06-elias-gamma-wire-formats/unary_gamma.hpp \
        post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp
git commit -m "feat(unary-gamma): add unary_lengths and gamma_lengths helpers (TDD)"
```

---

## Task 12: Integration tests using post 3's priors library

**Files:**
- Modify: `post/2022-06-elias-gamma-wire-formats/unary_gamma.hpp` (add include path note)
- Modify: `post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp`
- Modify: `post/CMakeLists.txt` (add include path for priors)

These tests verify the spec's claims about optimality: unary achieves entropy on a geometric source; gamma has bounded redundancy on a power-law source.

- [ ] **Step 1: Add priors include path to the test_unary_gamma target in CMakeLists.txt**

In `post/CMakeLists.txt`, change the `target_include_directories` line for `test_unary_gamma` to include both directories:

```cmake
target_include_directories(test_unary_gamma PRIVATE
    2022-06-elias-gamma-wire-formats
    2022-01-priors-wire-formats)
```

- [ ] **Step 2: Append integration tests to test_unary_gamma.cpp**

Append to `test_unary_gamma.cpp`:

```cpp
// Integration tests using the priors library from post 3.
#include "../2022-01-priors-wire-formats/priors.hpp"

// Unary is exactly optimal for geometric(1/2): redundancy ~ 0.
// (The code achieves entropy exactly because the prior is dyadic and saturates Kraft.)
TEST(UnaryGammaTest, UnaryAchievesEntropyOnGeometricPrior) {
    const std::size_t K = 30;
    auto lens = unary_lengths(K);
    auto probs = priors::implied_prior(lens);
    double r = priors::redundancy(probs, lens);
    // The implied prior of unary IS geometric(1/2), so redundancy should be
    // essentially zero (only floating-point and truncation error).
    EXPECT_NEAR(r, 0.0, 1e-6);
}

// Gamma has bounded redundancy on a power-law(2) source.
// This is the "approximately optimal" claim from the spec.
TEST(UnaryGammaTest, GammaSmallRedundancyOnPowerLaw2) {
    const std::size_t N = 128;
    auto lens = gamma_lengths(N);
    // Build power-law(2) source: p_n = C/n^2.
    std::vector<double> pl(N);
    double z = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        double n = static_cast<double>(i + 1);
        pl[i] = 1.0 / (n * n);
        z += pl[i];
    }
    for (double& p : pl) p /= z;
    double r = priors::redundancy(pl, lens);
    EXPECT_GE(r, 0.0);
    EXPECT_LT(r, 3.0);  // Universal-code bounded redundancy.
}

// Gamma beats unary on a power-law source.
TEST(UnaryGammaTest, GammaBeatsUnaryOnPowerLaw2) {
    const std::size_t N = 64;
    auto gamma_lens = gamma_lengths(N);
    auto unary_lens = unary_lengths(N);
    std::vector<double> pl(N);
    double z = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        double n = static_cast<double>(i + 1);
        pl[i] = 1.0 / (n * n);
        z += pl[i];
    }
    for (double& p : pl) p /= z;
    double r_gamma = priors::redundancy(pl, gamma_lens);
    double r_unary = priors::redundancy(pl, unary_lens);
    EXPECT_LT(r_gamma, r_unary);
}

// Unary beats gamma on a geometric(1/2) source.
TEST(UnaryGammaTest, UnaryBeatsGammaOnGeometricHalf) {
    const std::size_t K = 30;
    auto unary_lens = unary_lengths(K);
    auto gamma_lens = gamma_lengths(K);
    auto probs = priors::implied_prior(unary_lens);  // geometric(1/2)
    double r_unary = priors::redundancy(probs, unary_lens);
    double r_gamma = priors::redundancy(probs, gamma_lens);
    EXPECT_LE(r_unary, r_gamma);
}
```

- [ ] **Step 3: Build and verify**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | grep -E "PASSED|FAILED|test_unary_gamma"
```

Expected: all UnaryGammaTest cases pass including the four integration tests.

- [ ] **Step 4: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-06-elias-gamma-wire-formats/test_unary_gamma.cpp \
        post/CMakeLists.txt
git commit -m "test(unary-gamma): integration tests verifying optimality against priors library"
```

---

## Task 13: Verify post 4 full-suite pass (clean rebuild + warning check)

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make build 2>&1 | tail -5
```

Expected: successful clean build.

- [ ] **Step 2: Warning check for unary_gamma**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
cmake -B post/build -S post -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic" 2>&1 | grep -E "warning:|error:" | grep "unary_gamma" | head -20
cmake --build post/build --target test_unary_gamma 2>&1 | grep -E "warning:|error:" | head -20
```

Expected: zero warnings and zero errors from unary_gamma.hpp and test_unary_gamma.cpp.

- [ ] **Step 3: Full ctest pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make test 2>&1 | tail -8
```

Expected: all tests pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma).

No commit for this task.

---

## Task 14: Draft post 4 prose

**Files:**
- Modify: `post/2022-06-elias-gamma-wire-formats/index.md`

Draft from the spec's sections A through G for Post 4. Target approximately 2000 words. No em-dashes.

- [ ] **Step 1: Draft the prose**

Replace the placeholder in `post/2022-06-elias-gamma-wire-formats/index.md` with the full article body:

Section A ("The Two Simplest Universal Codes", ~200 words): unary predates information theory (tally marks); gamma is Elias 1975. Both are instances of post 3's "code as prior" claim.

Section B ("Unary: Geometric Prior", ~300 words + `Unary` code block): present the encoding rule, the struct implementation, the length analysis (n bits, Kraft saturates, implied prior is geometric(1/2)), and the test showing expected_length = entropy on this prior.

Section C ("Elias Gamma: Power-Law Prior", ~300 words + `Gamma` code block): present the encoding rule (k zeros, one, k trailing bits), the struct implementation, the length analysis (2k+1 bits, Kraft saturates, implied prior approximately 1/n^2), and the test showing bounded redundancy on power-law(2).

Section D ("Length Characteristics: O(n) vs O(log n)", ~250 words): include the comparison table from the spec showing unary and gamma lengths for n in {1, 2, 4, 8, 16, 100, 1024}. Discuss the crossover and which regime each code wins.

Section E ("When to Use Which", ~250 words): practical guidance. Use unary when 90%+ of values are 1 or 2. Use gamma for heavy left-skew with a long tail (word frequencies, file sizes). Both decode in time linear in codeword length.

Section F ("The Recursive Idea", ~200 words): note that gamma's length prefix is encoded in unary. What if we used gamma for the length prefix? That gives Elias delta (forthcoming in post 5). Forward pointer (plain text, no link yet): "The recursive idea is developed in the next post."

Section G ("Cross-references and footnote", ~120 words): per the spec. Forward to post 5 (plain text, no link). Backward to post 3 (live link). Cross-series to "When Lists Become Bits" (live link). PFC footnote pointing to `include/pfc/codecs.hpp` (`Unary` and `EliasGamma` structs).

Set `draft: false` when satisfied.

- [ ] **Step 2: Soul check**

```bash
grep -n $'\xe2\x80\x94' /home/spinoza/github/metafunctor-series/wire-formats/post/2022-06-elias-gamma-wire-formats/index.md | head -5
```

Expected: no output (no em-dashes).

- [ ] **Step 3: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add post/2022-06-elias-gamma-wire-formats/index.md
git commit -m "docs(unary-gamma): draft post 4 prose (Unary and Elias Gamma)"
```

---

## Task 15: Update `docs/about.md` (mark posts 3 and 4 as Published)

**Files:**
- Modify: `docs/about.md`

- [ ] **Step 1: Update the status rows for posts 3 and 4**

In `docs/about.md`, change the rows for posts 3 and 4 from `Forthcoming` to `Published`:

Old lines:
```
| 3 | Universal Codes as Priors | 2022-01-15 | Forthcoming |
| 4 | Unary and Elias Gamma | 2022-06-19 | Forthcoming |
```

New lines:
```
| 3 | Universal Codes as Priors | 2022-01-15 | Published |
| 4 | Unary and Elias Gamma | 2022-06-19 | Published |
```

- [ ] **Step 2: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add docs/about.md
git commit -m "docs(about): mark posts 3 and 4 as Published"
```

---

## Task 16: Update `mkdocs.yml` (add posts 3 and 4 to nav)

**Files:**
- Modify: `mkdocs.yml`

- [ ] **Step 1: Add a "Universal Codes" nav section and both posts**

In `mkdocs.yml`, after the `"Foundations"` nav section, add:

```yaml
  - "Universal Codes":
      - "Universal Codes as Priors": "post/2022-01-priors-wire-formats/index.md"
      - "Unary and Elias Gamma": "post/2022-06-elias-gamma-wire-formats/index.md"
```

The full nav section should look like:

```yaml
nav:
  - Home: index.md
  - About: about.md
  - "Foundations":
      - "Kraft's Inequality": "post/2020-03-kraft-wire-formats/index.md"
      - "McMillan's Converse": "post/2020-09-mcmillan-wire-formats/index.md"
  - "Universal Codes":
      - "Universal Codes as Priors": "post/2022-01-priors-wire-formats/index.md"
      - "Unary and Elias Gamma": "post/2022-06-elias-gamma-wire-formats/index.md"
```

- [ ] **Step 2: Commit**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats
git add mkdocs.yml
git commit -m "docs(mkdocs): add Universal Codes nav section with posts 3 and 4"
```

---

## Task 17: Final verification (clean rebuild + soul check + mkdocs build)

**Files:** no changes; verification only.

- [ ] **Step 1: Clean rebuild and full test pass**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make clean && make test 2>&1 | tail -10
```

Expected: all four test suites pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma).

- [ ] **Step 2: Soul check on both prose files**

```bash
grep -n $'\xe2\x80\x94' \
    /home/spinoza/github/metafunctor-series/wire-formats/post/2022-01-priors-wire-formats/index.md \
    /home/spinoza/github/metafunctor-series/wire-formats/post/2022-06-elias-gamma-wire-formats/index.md \
    | head -10
```

Expected: no output.

- [ ] **Step 3: mkdocs build**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && make docs 2>&1 | tail -8
```

Expected: successful build. Warnings about forthcoming posts in the nav are acceptable but posts 3 and 4 should resolve cleanly.

No commit for this task.

---

## Task 18: Hugo sync

**Files:** changes land in `~/github/repos/metafunctor/` (synced, not committed here).

- [ ] **Step 1: Sync posts 3 and 4 to metafunctor**

```bash
BLOG_POST_DIR=/home/spinoza/github/repos/metafunctor/content/post \
    make -C /home/spinoza/github/metafunctor-series/wire-formats sync 2>&1
```

Expected: rsync output showing two directories synced:
- `-> 2022-01-priors-wire-formats`
- `-> 2022-06-elias-gamma-wire-formats`

- [ ] **Step 2: Verify metafunctor received both post directories**

```bash
ls /home/spinoza/github/repos/metafunctor/content/post/ | grep -E "2022-01-priors|2022-06-elias-gamma"
```

Expected: both directories present.

No commit for this task (sync only; metafunctor commit is a separate step).

---

## Task 19: Verify metafunctor state

**Files:** read-only inspection of metafunctor repo.

- [ ] **Step 1: Check git status in metafunctor**

```bash
cd /home/spinoza/github/repos/metafunctor && git status --short | grep -E "2022-01-priors|2021-08-elias"
```

Expected: two new untracked directories (or staged adds if already added).

- [ ] **Step 2: Check mf series scan**

```bash
cd /home/spinoza/github/repos/metafunctor && mf series scan 2>&1 | grep wire-formats | head -5
```

Expected: wire-formats series shows count including the two new posts (total 4 posts: Kraft, McMillan, priors, unary-gamma).

No commit for this task.

---

## Task 20: Final pre-push verification across both repos

**Files:** read-only.

- [ ] **Step 1: Wire-formats git log (confirm all commits present)**

```bash
git -C /home/spinoza/github/metafunctor-series/wire-formats log --oneline | head -20
```

Expected: all commits from this plan appear in sequence: scaffold(priors), feat(priors), feat(priors), test(priors), scaffold(unary-gamma), feat(unary-gamma), feat(unary-gamma), feat(unary-gamma), test(unary-gamma), docs, docs(about), docs(mkdocs).

- [ ] **Step 2: Verify no uncommitted changes in wire-formats**

```bash
cd /home/spinoza/github/metafunctor-series/wire-formats && git status --short
```

Expected: clean working tree (the build/ and site/ directories are gitignored).

- [ ] **Step 3: Confirm metafunctor has the two new post directories ready to commit**

```bash
cd /home/spinoza/github/repos/metafunctor && git status --short | grep "2021-"
```

Expected: two new directories shown as untracked or staged.

No commit for this task.

---

## Task 21: User-confirmed push

The user must confirm before pushing. Do not push automatically.

- [ ] **Step 1: Present the push plan and wait for confirmation**

Report the following to the user before proceeding:

- Wire-formats repo: N new commits to push to origin/main.
- Metafunctor repo: 2 new post directories to commit and push.
- No changes to the Stepanov repo in this sub-sub-project.

Wait for explicit user approval.

- [ ] **Step 2: Push wire-formats (after user confirms)**

```bash
git -C /home/spinoza/github/metafunctor-series/wire-formats push origin main
```

- [ ] **Step 3: Commit and push metafunctor (after user confirms)**

```bash
cd /home/spinoza/github/repos/metafunctor
git add content/post/2022-01-priors-wire-formats \
        content/post/2022-06-elias-gamma-wire-formats
git commit -m "content(wire-formats): sync posts 3 and 4 (priors, unary-gamma)"
git push origin main
```

---

## Task 22: Final summary report

- [ ] **Step 1: Produce a summary for the user**

Report:
- Posts shipped: post 3 ("Universal Codes as Priors", 2022-01-15) and post 4 ("Unary and Elias Gamma", 2022-06-19).
- Files created: `priors.hpp` (4 functions: implied_prior, entropy, expected_length, redundancy), `test_priors.cpp` (14 tests), `unary_gamma.hpp` (2 codecs + 2 helpers: Unary, Gamma, unary_lengths, gamma_lengths), `test_unary_gamma.cpp` (integration tests against priors library), `index.md` for each post.
- Test counts: all suites pass (test_kraft, test_mcmillan, test_priors, test_unary_gamma).
- Navigation: `mkdocs.yml` updated with "Universal Codes" section; `docs/about.md` updated with Published status for both posts.
- Sync: both posts available in metafunctor under `content/post/`.
- Next: sub-sub-project 3b covers posts 5 ("Elias Delta and Omega") and 6 ("Fibonacci Coding"). Post 5's `Delta` codec can call `Gamma::encode` from post 4's `unary_gamma.hpp` directly.
