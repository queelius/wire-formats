#include <gtest/gtest.h>
#include <algorithm>
#include <set>
#include <string>
#include <vector>
#include "mcmillan.hpp"

using namespace mcmillan;

TEST(McMillanTest, BuildPrefixFreeCodeForBalancedLengths) {
    auto code = build_prefix_free_code({2, 2, 2, 2});
    ASSERT_EQ(code.size(), 4u);
    for (const auto& cw : code) {
        EXPECT_EQ(cw.size(), 2u);
    }
    std::set<std::string> distinct(code.begin(), code.end());
    EXPECT_EQ(distinct.size(), 4u);
}

TEST(McMillanTest, BuildPrefixFreeCodeForExampleLengths) {
    auto code = build_prefix_free_code({1, 2, 3, 3});
    ASSERT_EQ(code.size(), 4u);
    EXPECT_EQ(code[0].size(), 1u);
    EXPECT_EQ(code[1].size(), 2u);
    EXPECT_EQ(code[2].size(), 3u);
    EXPECT_EQ(code[3].size(), 3u);
}

TEST(McMillanTest, BuildPrefixFreeCodeProducesPrefixFreeCode) {
    auto code = build_prefix_free_code({1, 2, 3, 3});
    for (std::size_t i = 0; i < code.size(); ++i) {
        for (std::size_t j = 0; j < code.size(); ++j) {
            if (i == j) continue;
            const std::string& a = code[i];
            const std::string& b = code[j];
            if (a.size() > b.size()) continue;
            EXPECT_NE(b.substr(0, a.size()), a)
                << "Codeword " << a << " (idx " << i
                << ") is a prefix of " << b << " (idx " << j << ")";
        }
    }
}

TEST(McMillanTest, BuildPrefixFreeCodeForEmptyLengthsReturnsEmpty) {
    auto code = build_prefix_free_code({});
    EXPECT_TRUE(code.empty());
}
