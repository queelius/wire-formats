# PFC: Prefix-Free Codecs (production reference)

Header-only C++20 library implementing zero-copy, prefix-free data
representations with full algebraic type support and STL integration.

## What this is

This is the production reference for the [Algebra over Wire Formats](https://github.com/queelius/wire-formats) blog series. The series develops the theoretical and pedagogical content; this library is the hardened, fully-tested implementation that the series points to.

If you read a wire-formats post and want to see the production version of a codec, look in `include/pfc/` here.

## What changed

This library used to live as a standalone GitHub repo at `queelius/pfc`. It is now part of the wire-formats series repo, in `lib/pfc/`. The standalone repo is archived; this is the canonical location.

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_CXX_STANDARD=20
cmake --build . -j4
ctest
```

## License

MIT (matches PFC's original license; the wire-formats series is GPL v3 but the library remains MIT for compatibility with downstream library consumers).
