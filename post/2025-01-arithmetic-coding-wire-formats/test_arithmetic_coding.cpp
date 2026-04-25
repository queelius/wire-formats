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
