#include <gtest/gtest.h>
#include "arithmetic_coding.hpp"
#include "priors.hpp"

using namespace arithmetic_coding;

// Round-trip: write N bits and read them back.
TEST(BitIOTest, RoundTripSingleBit) {
    BitWriter bw;
    bw.write(true);
    bw.flush();
    BitReader br(bw.bytes());
    EXPECT_EQ(br.read(), true);
}

TEST(BitIOTest, RoundTripMultipleBits) {
    BitWriter bw;
    std::vector<bool> bits = {1,0,1,1,0,0,1,0, 1,1,0,0,0,1,1,0};
    for (bool b : bits) bw.write(b);
    bw.flush();
    BitReader br(bw.bytes());
    for (std::size_t i = 0; i < bits.size(); ++i) {
        EXPECT_EQ(br.read(), bits[i]) << "bit index " << i;
    }
}

TEST(BitIOTest, EmptyStreamBytesEmpty) {
    BitWriter bw;
    bw.flush();
    // A flushed writer with no bits written should produce exactly 0 or 1
    // bytes (depending on implementation; we require the reader can be
    // constructed from whatever is returned).
    EXPECT_NO_THROW({ BitReader br(bw.bytes()); });
}

TEST(ArithmeticEncoderTest, ConstructorInitializesState) {
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    // After construction: low = 0, high = TOP_VALUE, underflow_count = 0.
    EXPECT_EQ(enc.low(),             0u);
    EXPECT_EQ(enc.high(),            TOP_VALUE);
    EXPECT_EQ(enc.underflow_count(), 0u);
}

// After encoding a single symbol over two equiprobable symbols (low_cum=0,
// high_cum=1, total=2), the interval should be the lower half [0, HALF-1].
TEST(ArithmeticEncoderTest, EncodeSymbolShrinksIntervalCorrectly) {
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    // Symbol 0 of 2 equiprobable symbols: low_cum=0, high_cum=1, total=2.
    enc.encode_symbol(0, 1, 2);
    // Expected: new_high = 0 + (TOTAL_RANGE * 1) / 2 - 1 = HALF - 1.
    //           new_low  = 0 + (TOTAL_RANGE * 0) / 2     = 0.
    // Then renormalize should emit one '0' bit and double the interval.
    // After renormalize: low=0, high=TOP_VALUE (full range again).
    EXPECT_EQ(enc.low(),  0u);
    EXPECT_EQ(enc.high(), TOP_VALUE);
}

// After encoding symbol 1 of 2 (the upper half), the interval should be
// the upper half [HALF, TOP_VALUE], then renormalize emits a '1' bit.
TEST(ArithmeticEncoderTest, EncodeSymbolUpperHalf) {
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    enc.encode_symbol(1, 2, 2);
    EXPECT_EQ(enc.low(),  0u);
    EXPECT_EQ(enc.high(), TOP_VALUE);
}

// Underflow test: encoding a symbol that straddles the midpoint
// (low < QUARTER, high >= THREE_QUARTER after shrink) should increment
// underflow_count_ rather than emitting a bit.
TEST(ArithmeticEncoderTest, UnderflowIncrements) {
    // Use a very skewed distribution: symbol with low_cum=1, high_cum=3,
    // total=4. The interval shrinks to [QUARTER, THREE_QUARTER-1], which
    // straddles the midpoint; underflow_count should become >= 1.
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    enc.encode_symbol(1, 3, 4);
    EXPECT_GE(enc.underflow_count(), 1u);
}

// After encoding one symbol and calling finish(), the bit stream should be
// non-empty and the encoder should be in a defined terminal state.
TEST(ArithmeticEncoderTest, FinishProducesNonEmptyStream) {
    BitWriter bw;
    ArithmeticEncoder enc(bw);
    enc.encode_symbol(0, 1, 2);  // symbol 0 of {0,1} equiprobable
    enc.finish();
    bw.flush();
    EXPECT_GT(bw.bytes().size(), 0u);
}

// Encoding the same sequence twice should produce identical bit streams.
TEST(ArithmeticEncoderTest, FinishIsDeterministic) {
    auto encode_once = [](std::uint32_t sym_low, std::uint32_t sym_high,
                          std::uint32_t total) {
        BitWriter bw;
        ArithmeticEncoder enc(bw);
        enc.encode_symbol(sym_low, sym_high, total);
        enc.finish();
        bw.flush();
        return bw.bytes();
    };
    EXPECT_EQ(encode_once(0, 1, 2), encode_once(0, 1, 2));
    EXPECT_EQ(encode_once(1, 2, 2), encode_once(1, 2, 2));
}

// A decoder constructed from a BitReader derived from a 4-byte stream
// should initialize with low=0, high=TOP_VALUE, and code set to the
// first 32 bits of the stream.
TEST(ArithmeticDecoderTest, ConstructorPrimesCode) {
    // Encode one symbol so we have a non-trivial byte stream.
    BitWriter bw;
    {
        ArithmeticEncoder enc(bw);
        enc.encode_symbol(0, 1, 2);
        enc.finish();
    }
    bw.flush();
    BitReader br(bw.bytes());
    ArithmeticDecoder dec(br);
    EXPECT_EQ(dec.low(),  0u);
    EXPECT_EQ(dec.high(), TOP_VALUE);
    // code_ should be some 32-bit value -- we just verify it is accessible.
    EXPECT_NO_THROW({ (void)dec.code(); });
}

// Encoding then decoding a single symbol should recover the original.
// Use two equiprobable symbols (total=2, cumulative freqs {0,1,2}).
TEST(ArithmeticDecoderTest, DecodeSymbolAfterEncodeRoundTrips) {
    // Cumulative frequency table: sym 0 -> [0,1), sym 1 -> [1,2). Total=2.
    // Symbol lookup callback: given scaled_value in [0,total), return symbol.
    auto get_freq = [](std::uint32_t scaled) -> std::size_t {
        return (scaled >= 1) ? 1u : 0u;
    };
    // Cumulative intervals for update: sym 0 -> [0,1), sym 1 -> [1,2).
    auto cum_range = [](std::size_t sym)
        -> std::pair<std::uint32_t, std::uint32_t> {
        if (sym == 0) return {0, 1};
        return {1, 2};
    };

    for (std::size_t expected_sym : {0u, 1u}) {
        BitWriter bw;
        {
            ArithmeticEncoder enc(bw);
            auto [lo, hi] = cum_range(expected_sym);
            enc.encode_symbol(lo, hi, 2);
            enc.finish();
        }
        bw.flush();
        BitReader br(bw.bytes());
        ArithmeticDecoder dec(br);
        std::size_t got = dec.decode_symbol(get_freq, cum_range, 2);
        EXPECT_EQ(got, expected_sym);
    }
}

// Helper: encode a single symbol from a two-symbol source, then decode and
// verify. The two-symbol table has sym0=[0,sep) and sym1=[sep,total).
// When encoding sym0 we call encode_symbol(0, sep, total); sep=hi_cum.
// When encoding sym1 we call encode_symbol(sep, total, total); sep=lo_cum.
static std::size_t encode_decode_single(std::uint32_t lo_cum,
                                        std::uint32_t hi_cum,
                                        std::uint32_t total,
                                        std::size_t   /*expected_sym*/) {
    BitWriter bw;
    {
        ArithmeticEncoder enc(bw);
        enc.encode_symbol(lo_cum, hi_cum, total);
        enc.finish();
    }
    bw.flush();
    BitReader br(bw.bytes());
    ArithmeticDecoder dec(br);

    // sep is the boundary between sym0 and sym1 in the 2-symbol table.
    // If lo_cum == 0 we are encoding sym0 whose upper bound is hi_cum.
    // Otherwise we are encoding sym1 whose lower bound is lo_cum.
    std::uint32_t sep = (lo_cum == 0) ? hi_cum : lo_cum;

    auto get_freq = [=](std::uint32_t scaled) -> std::size_t {
        return (scaled >= sep) ? 1u : 0u;
    };
    auto cum_range = [=](std::size_t sym)
        -> std::pair<std::uint32_t, std::uint32_t> {
        if (sym == 0) return {0, sep};
        return {sep, total};
    };
    return dec.decode_symbol(get_freq, cum_range, total);
}

// Equiprobable (50/50)
TEST(RoundTripTest, SingleSymbolEquiprobable) {
    EXPECT_EQ(encode_decode_single(0, 1, 2, 0u), 0u);
    EXPECT_EQ(encode_decode_single(1, 2, 2, 1u), 1u);
}

// Skewed 90/10
TEST(RoundTripTest, SingleSymbolSkewed90) {
    EXPECT_EQ(encode_decode_single(0, 9, 10, 0u), 0u);
    EXPECT_EQ(encode_decode_single(9, 10, 10, 1u), 1u);
}

// Skewed 99/1
TEST(RoundTripTest, SingleSymbolSkewed99) {
    EXPECT_EQ(encode_decode_single(0, 99, 100, 0u), 0u);
    EXPECT_EQ(encode_decode_single(99, 100, 100, 1u), 1u);
}

// Helper: encode a sequence of binary symbols under a Bernoulli(p) source
// (p = prob_high/total), then decode and verify.
// Returns true if the decoded sequence matches the original.
static bool sequence_round_trip(const std::vector<std::size_t>& symbols,
                                 std::uint32_t prob_high, std::uint32_t total) {
    // Cumulative freqs: sym 0 -> [0, prob_high), sym 1 -> [prob_high, total).
    BitWriter bw;
    {
        ArithmeticEncoder enc(bw);
        for (std::size_t sym : symbols) {
            if (sym == 0) {
                enc.encode_symbol(0, prob_high, total);
            } else {
                enc.encode_symbol(prob_high, total, total);
            }
        }
        enc.finish();
    }
    bw.flush();
    BitReader br(bw.bytes());
    ArithmeticDecoder dec(br);

    auto get_freq = [=](std::uint32_t scaled) -> std::size_t {
        return (scaled >= prob_high) ? 1u : 0u;
    };
    auto cum_range = [=](std::size_t sym)
        -> std::pair<std::uint32_t, std::uint32_t> {
        if (sym == 0) return {0, prob_high};
        return {prob_high, total};
    };

    for (std::size_t expected : symbols) {
        std::size_t got = dec.decode_symbol(get_freq, cum_range, total);
        if (got != expected) return false;
    }
    return true;
}

TEST(RoundTripTest, Sequence10SymbolsEquiprobable) {
    std::vector<std::size_t> syms = {0,1,0,0,1,1,0,1,0,0};
    EXPECT_TRUE(sequence_round_trip(syms, 1, 2));
}

TEST(RoundTripTest, Sequence50SymbolsSkewed90) {
    std::vector<std::size_t> syms(50);
    for (std::size_t i = 0; i < 50; ++i) syms[i] = (i % 10 == 0) ? 1u : 0u;
    EXPECT_TRUE(sequence_round_trip(syms, 9, 10));
}

TEST(RoundTripTest, Sequence100SymbolsSkewed99) {
    std::vector<std::size_t> syms(100);
    for (std::size_t i = 0; i < 100; ++i) syms[i] = (i % 100 == 0) ? 1u : 0u;
    EXPECT_TRUE(sequence_round_trip(syms, 99, 100));
}
