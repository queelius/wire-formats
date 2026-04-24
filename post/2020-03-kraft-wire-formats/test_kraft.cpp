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
