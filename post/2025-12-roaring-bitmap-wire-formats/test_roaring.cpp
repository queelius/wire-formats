#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "roaring_bitmap.hpp"

using namespace roaring;

// ---- ArrayContainer tests ---------------------------------------------------

TEST(ArrayContainerTest, EmptyOnConstruct) {
    ArrayContainer c;
    EXPECT_EQ(c.cardinality(), 0u);
    EXPECT_FALSE(c.contains(0));
    EXPECT_FALSE(c.contains(65535));
}

TEST(ArrayContainerTest, AddAndContains) {
    ArrayContainer c;
    c.add(100);
    c.add(200);
    c.add(50);
    EXPECT_EQ(c.cardinality(), 3u);
    EXPECT_TRUE(c.contains(50));
    EXPECT_TRUE(c.contains(100));
    EXPECT_TRUE(c.contains(200));
    EXPECT_FALSE(c.contains(99));
    EXPECT_FALSE(c.contains(101));
}

TEST(ArrayContainerTest, DuplicateAddNoChange) {
    ArrayContainer c;
    c.add(42);
    c.add(42);
    EXPECT_EQ(c.cardinality(), 1u);
}

TEST(ArrayContainerTest, AddMaintainsSortedOrder) {
    ArrayContainer c;
    c.add(300);
    c.add(10);
    c.add(100);
    // Internally sorted: contains should work via binary search.
    EXPECT_TRUE(c.contains(10));
    EXPECT_TRUE(c.contains(100));
    EXPECT_TRUE(c.contains(300));
    EXPECT_FALSE(c.contains(200));
}

TEST(ArrayContainerTest, CardinalityGrows) {
    ArrayContainer c;
    for (uint16_t v = 0; v < 100; ++v) c.add(v);
    EXPECT_EQ(c.cardinality(), 100u);
}
