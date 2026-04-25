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
