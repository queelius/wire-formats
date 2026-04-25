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

// ---- BitmapContainer tests --------------------------------------------------

TEST(BitmapContainerTest, EmptyOnConstruct) {
    BitmapContainer c;
    EXPECT_EQ(c.cardinality(), 0u);
    EXPECT_FALSE(c.contains(0));
    EXPECT_FALSE(c.contains(65535));
}

TEST(BitmapContainerTest, AddAndContains) {
    BitmapContainer c;
    c.add(0);
    c.add(65535);
    c.add(1000);
    EXPECT_EQ(c.cardinality(), 3u);
    EXPECT_TRUE(c.contains(0));
    EXPECT_TRUE(c.contains(65535));
    EXPECT_TRUE(c.contains(1000));
    EXPECT_FALSE(c.contains(1));
    EXPECT_FALSE(c.contains(999));
}

TEST(BitmapContainerTest, DuplicateAddNoChange) {
    BitmapContainer c;
    c.add(500);
    c.add(500);
    EXPECT_EQ(c.cardinality(), 1u);
}

TEST(BitmapContainerTest, CardinalityAfterManyAdds) {
    BitmapContainer c;
    for (uint16_t v = 0; v < 1000; ++v) c.add(v);
    EXPECT_EQ(c.cardinality(), 1000u);
}

TEST(BitmapContainerTest, WordBoundaryBits) {
    BitmapContainer c;
    c.add(63);   // Last bit of word 0.
    c.add(64);   // First bit of word 1.
    EXPECT_TRUE(c.contains(63));
    EXPECT_TRUE(c.contains(64));
    EXPECT_FALSE(c.contains(62));
    EXPECT_FALSE(c.contains(65));
}

// ---- RunContainer tests -----------------------------------------------------

TEST(RunContainerTest, EmptyOnConstruct) {
    RunContainer c;
    EXPECT_EQ(c.cardinality(), 0u);
    EXPECT_FALSE(c.contains(0));
}

TEST(RunContainerTest, SingleElement) {
    RunContainer c;
    c.add(42);
    EXPECT_EQ(c.cardinality(), 1u);
    EXPECT_TRUE(c.contains(42));
    EXPECT_FALSE(c.contains(41));
    EXPECT_FALSE(c.contains(43));
}

TEST(RunContainerTest, ConsecutiveAddsMergeRun) {
    RunContainer c;
    c.add(10);
    c.add(11);
    c.add(12);
    EXPECT_EQ(c.cardinality(), 3u);
    EXPECT_EQ(c.num_runs(), 1u);  // All merged into one run [10,12].
    EXPECT_TRUE(c.contains(10));
    EXPECT_TRUE(c.contains(11));
    EXPECT_TRUE(c.contains(12));
    EXPECT_FALSE(c.contains(9));
    EXPECT_FALSE(c.contains(13));
}

TEST(RunContainerTest, NonConsecutiveAddsTwoRuns) {
    RunContainer c;
    c.add(5);
    c.add(6);
    c.add(10);
    c.add(11);
    EXPECT_EQ(c.cardinality(), 4u);
    EXPECT_EQ(c.num_runs(), 2u);
}

TEST(RunContainerTest, AddAtBoundaryExtendsPriorRun) {
    RunContainer c;
    c.add(100);
    c.add(101);
    c.add(102);
    c.add(103);  // Should extend [100,102] to [100,103].
    EXPECT_EQ(c.num_runs(), 1u);
    EXPECT_EQ(c.cardinality(), 4u);
}

TEST(RunContainerTest, DuplicateAddNoChange) {
    RunContainer c;
    c.add(7);
    c.add(8);
    c.add(7);  // Duplicate.
    EXPECT_EQ(c.cardinality(), 2u);
    EXPECT_EQ(c.num_runs(), 1u);
}
