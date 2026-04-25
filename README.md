# Algebra over Wire Formats

Pedagogical blog posts exploring information theory by construction in C++23.

**Companion to** the [Stepanov series](https://github.com/queelius/stepanov), which develops algorithms from algebraic structure on type. This series develops algorithms from algebraic structure on bit strings: how prefix-free codes work, why each universal code is optimal under a different prior, and how succinct data structures achieve their bounds.

## The Core Thesis

**A code is a hypothesis about the source.** Each universal code corresponds to a different prior over the integers; each entropy-optimal code (Huffman, arithmetic) is best under different assumptions; each succinct data structure (rank/select bit vectors, RoaringBitmap) is the right answer under different access patterns. The series develops these as one coherent algebraic story.

The production reference for the code in this series is [PFC](https://github.com/queelius/wire-formats/tree/master/lib/pfc), a header-only C++20 prefix-free codecs library.

## Posts

Each post in `post/` is self-contained:
- `index.md` (the article, Hugo-compatible with YAML frontmatter)
- `*.hpp` (minimal pedagogical implementation, ~100-400 lines)
- `test_*.cpp` (GoogleTest suite verifying the implementation)

| Post | Date | Topic |
|------|------|-------|
| `2020-03-kraft-wire-formats/` | 2020-03-22 | Kraft's inequality: which length vectors are achievable for prefix-free codes |
| `2020-09-mcmillan-wire-formats/` | 2020-09-13 | McMillan's converse: any Kraft-satisfying length vector has a prefix-free code |

(More posts forthcoming; the series arc spans 13 posts through 2026.)

## Building

```bash
make build   # Configure and build all post code
make test    # Run all GoogleTest suites
make clean   # Remove build artifacts
make docs    # Build the mkdocs site locally
```

## Hugo Sync (for the canonical metafunctor.com site)

```bash
BLOG_POST_DIR=~/github/repos/metafunctor/content/post make sync
```

The `sync` target rsyncs each post directory into the BLOG_POST_DIR, preserving any unrelated content there. The `--delete` flag is scoped per-post, NOT global.

## License

GPL v3 (matching the companion Stepanov series).

## Author

**Alexander Towell**, [metafunctor.com](https://metafunctor.com), [lex@metafunctor.com](mailto:lex@metafunctor.com)
