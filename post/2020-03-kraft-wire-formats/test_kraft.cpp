#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>
#include "kraft.hpp"

using namespace kraft;

TEST(KraftTest, BinaryTreeInsertCodewordAtRoot) {
    BinaryTree t;
    t.insert("0");
    EXPECT_TRUE(t.contains("0"));
    EXPECT_FALSE(t.contains("1"));
}

TEST(KraftTest, BinaryTreeInsertMultipleCodewords) {
    BinaryTree t;
    t.insert("0");
    t.insert("10");
    t.insert("110");
    t.insert("111");
    EXPECT_TRUE(t.contains("0"));
    EXPECT_TRUE(t.contains("10"));
    EXPECT_TRUE(t.contains("110"));
    EXPECT_TRUE(t.contains("111"));
}

TEST(KraftTest, BinaryTreeIsPrefixFreeReturnsTrueForValidCode) {
    BinaryTree t;
    t.insert("0");
    t.insert("10");
    t.insert("110");
    t.insert("111");
    EXPECT_TRUE(t.is_prefix_free());
}

TEST(KraftTest, BinaryTreeIsPrefixFreeReturnsFalseWhenCodewordIsPrefix) {
    BinaryTree t;
    t.insert("0");
    t.insert("01");
    EXPECT_FALSE(t.is_prefix_free());
}

TEST(KraftTest, BinaryTreeIsPrefixFreeReturnsFalseWhenLongerCodewordContainsAnother) {
    BinaryTree t;
    t.insert("010");
    t.insert("01");
    EXPECT_FALSE(t.is_prefix_free());
}

TEST(KraftTest, KraftSumOfEmptyVectorIsZero) {
    EXPECT_DOUBLE_EQ(kraft_sum({}), 0.0);
}

TEST(KraftTest, KraftSumOfSingletonLengthOne) {
    EXPECT_DOUBLE_EQ(kraft_sum({1}), 0.5);
}

TEST(KraftTest, KraftSumOfBalancedBinaryCode) {
    // Code with all four 2-bit codewords saturates Kraft.
    // Lengths 2, 2, 2, 2 give 4 * 2^-2 = 1.
    EXPECT_DOUBLE_EQ(kraft_sum({2, 2, 2, 2}), 1.0);
}

TEST(KraftTest, KraftSumOfExampleCode) {
    // Example from the post: A=0 (1 bit), B=10 (2 bits), C=110 (3 bits), D=111 (3 bits).
    // Sum: 1/2 + 1/4 + 1/8 + 1/8 = 1.
    EXPECT_DOUBLE_EQ(kraft_sum({1, 2, 3, 3}), 1.0);
}

TEST(KraftTest, KraftSumOfUnaryFirstFewIsLessThanOne) {
    // Unary lengths: 1, 2, 3, 4, 5. Sum: 1/2 + 1/4 + 1/8 + 1/16 + 1/32 = 31/32.
    EXPECT_NEAR(kraft_sum({1, 2, 3, 4, 5}), 31.0/32.0, 1e-12);
}

TEST(KraftTest, KraftSumExceedsOneForOverlongCode) {
    // 3 codewords of length 1 cannot fit prefix-freely (only 2 leaves at depth 1).
    // Sum: 3 * 1/2 = 1.5 > 1.
    EXPECT_GT(kraft_sum({1, 1, 1}), 1.0);
}
