# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## Project Overview

Algebra over Wire Formats is a pedagogical blog series exploring information theory by construction in C++23. Companion to the Stepanov series (`~/github/metafunctor-series/stepanov/`).

**Series thesis**: a code is a hypothesis about the source. Each universal code corresponds to a different prior; each compression scheme is optimal under different assumptions.

The production reference is PFC (`~/github/released/pfc/`).

## Repo conventions

- Each post lives in `post/YYYY-MM-name-wire-formats/` (flat directory, no subdirs)
- Each post has exactly three files: `index.md`, `*.hpp`, `test_*.cpp`
- Test executables are registered in `post/CMakeLists.txt` (no per-post CMakeLists)
- C++23, GoogleTest v1.14.0, fetched via FetchContent
- Voice: Alex Towell's, soul-plugin checked. NO em-dashes (use commas, periods, colons, parens)
- Math: LaTeX (`$$...$$` for display, `\(...\)` for inline)

## Build

```bash
make build         # Configure and build via post/CMakeLists.txt
make test          # Run all tests via ctest
make clean         # Remove post/build/
make docs          # Build mkdocs site
make docs-serve    # Serve mkdocs locally
```

## Hugo sync

```bash
BLOG_POST_DIR=~/github/repos/metafunctor/content/post make sync
```

The `sync` target uses per-directory rsync with `--delete` scoped to each
destination subdirectory. Unrelated content in BLOG_POST_DIR is preserved.

## Series ordering

The series's planned 13-post arc is documented in
`docs/superpowers/specs/2026-04-24-bootstrap-and-posts-1-2-design.md`. Posts
are backdated 2020-2026 (~2/year) to reflect the long developmental story.

## Cross-references

- Stepanov bridge posts (slot 20 "Bits Follow Types", slot 21 "When Lists
  Become Bits") forward-reference this series. Once posts are shipped,
  update those references to live links pointing here.
