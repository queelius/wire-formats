---
title: "About"
description: "About the Algebra over Wire Formats series"
---

# About

## The Series Arc

The series is planned for 13 posts spanning 2020 through 2026, roughly 2 posts per year. The arc moves from foundational results (Kraft, McMillan) through the universal codes (Elias, Fibonacci, Rice/Golomb, VByte) to the entropy-optimal codes (Huffman, arithmetic) and finally to succinct data structures (rank/select, RoaringBitmap), closing with a synthesis post that ties everything back to the codecs-as-functors framing introduced in the Stepanov bridge posts.

| # | Title | Date | Status |
|---|-------|------|--------|
| 1 | Kraft's Inequality | 2020-03-22 | Published |
| 2 | McMillan's Converse | 2020-09-13 | Published |
| 3 | Universal Codes as Priors | 2022-01-15 | Published |
| 4 | Unary and Elias Gamma | 2022-06-19 | Published |
| 5 | Elias Delta and Omega | 2022-11-13 | Published |
| 6 | Fibonacci Coding | 2023-04-23 | Published |
| 7 | Rice / Golomb | 2023-09-17 | Published |
| 8 | VByte / Varint | 2024-02-25 | Published |
| 9 | Huffman Coding | 2024-08-04 | Published |
| 10 | Arithmetic Coding | 2025-01-12 | Published |
| 11 | Succinct Bit Vectors and Rank/Select | 2025-06-22 | Published |
| 12 | RoaringBitmap                        | 2025-12-07 | Published |
| 13 | Synthesis: Codecs as Structure | 2026-05-15 | Forthcoming |

## Voice and Style

- Each post is self-contained and stands alone (start anywhere)
- Code is C++23, headers-only, with a minimal pedagogical implementation per post (100 to 400 lines)
- Tests use GoogleTest v1.14.0
- Math is rendered with MathJax via mkdocs-arithmatex

## Further Reading

- Cover and Thomas, *Elements of Information Theory* (2006)
- MacKay, *Information Theory, Inference, and Learning Algorithms* (2003)
- Knuth, *The Art of Computer Programming, Volume 4A* (2011)
- Sayood, *Introduction to Data Compression* (2017)
- McMillan, "Two Inequalities Implied by Unique Decipherability," 1956
