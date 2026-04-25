---
title: "A Code Is a Hypothesis About the Source"
description: "A pedagogical series exploring information theory by construction in C++23"
---

# A Code Is a Hypothesis About the Source

This series develops information theory the same way the [Stepanov series](https://github.com/queelius/stepanov) develops algorithms: by construction, with minimal pedagogical code, and with the algebraic structure made explicit.

The Stepanov series argued that the algebraic structure of a type determines its algorithms. This companion series argues the analogous claim for bit-level encodings: **the algebraic structure of a code determines its compression efficiency, its decoding cost, and its compositional behavior.** Each universal code corresponds to a different prior over the integers; each entropy-optimal code (Huffman, arithmetic) is best under different assumptions; each succinct data structure is the right answer under different access patterns.

## The Principle

A prefix-free code assigns codeword lengths `(l_1, ..., l_n)` to symbols. Kraft's inequality characterizes which length vectors are achievable: $$\sum_i 2^{-l_i} \leq 1.$$ Within this budget, every code is a different way of spending the budget. Allocating more bits to symbol `i` (longer `l_i`) means less budget for the others. Information theory's optimum (assigning `-\log_2 p_i` bits to a symbol with probability `p_i`) saturates Kraft when the probabilities are dyadic.

This series walks through the codes that have shown up in practice, treating each one as a different hypothesis about the integer distribution it expects to encode. The choices are not arbitrary: each code is optimal somewhere, and recognizing where reframes "compression algorithm" as "model selection."

## Posts

### Foundations

- [Kraft's Inequality](post/2020-03-kraft-wire-formats/index.md): the budget constraint on prefix-free codes
- [McMillan's Converse](post/2020-09-mcmillan-wire-formats/index.md): every Kraft-satisfying length vector has a prefix-free code

(More posts forthcoming; see the [arc plan](about.md) for the full schedule.)

## The Production Reference

The code in this series is pedagogical. The production version (with full STL integration, 31k+ test assertions, and the rich combinator library these posts only sketch) lives in [PFC](https://github.com/queelius/wire-formats/tree/master/lib/pfc).

## Companion Series

- [Stepanov: Generic Programming in C++](https://queelius.github.io/stepanov/): algorithms from algebra on type. The Stepanov bridge posts ([Bits Follow Types](https://queelius.github.io/stepanov/post/2026-05-codecs-functors-stepanov/), [When Lists Become Bits](https://queelius.github.io/stepanov/post/2026-05-prefix-free-stepanov/)) connect that series to this one.

## Author

**Alexander Towell**, [metafunctor.com](https://metafunctor.com), [lex@metafunctor.com](mailto:lex@metafunctor.com)
