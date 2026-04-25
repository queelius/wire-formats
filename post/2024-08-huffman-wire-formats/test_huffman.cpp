#include <gtest/gtest.h>
#include <cmath>
#include <cstddef>
#include <vector>
#include "huffman.hpp"

using namespace huffman;

// Helper: count the total number of leaves reachable from a node.
static int count_leaves(const Node* node) {
    if (!node) return 0;
    if (node->symbol >= 0) return 1;  // leaf
    return count_leaves(node->left.get()) + count_leaves(node->right.get());
}

// Helper: count the total number of nodes (leaves + internal).
static int count_nodes(const Node* node) {
    if (!node) return 0;
    return 1 + count_nodes(node->left.get()) + count_nodes(node->right.get());
}

// For a 4-symbol distribution, the Huffman tree must have exactly 4 leaves
// and (4 - 1) = 3 internal nodes, so 7 nodes total.
TEST(HuffmanTest, TreeHasCorrectLeafCount) {
    std::vector<double> freqs = {0.4, 0.3, 0.2, 0.1};
    auto root = build_huffman_tree(freqs);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(count_leaves(root.get()), 4);
    EXPECT_EQ(count_nodes(root.get()), 7);  // 4 leaves + 3 internal
}

// The root's frequency must equal the sum of all input frequencies.
TEST(HuffmanTest, RootFrequencyIsSum) {
    std::vector<double> freqs = {0.4, 0.3, 0.2, 0.1};
    auto root = build_huffman_tree(freqs);
    ASSERT_NE(root, nullptr);
    double total = 0.0;
    for (double f : freqs) total += f;
    EXPECT_NEAR(root->freq, total, 1e-12);
}

// Single-symbol distribution: the root is itself a leaf.
TEST(HuffmanTest, SingleSymbolTreeIsLeaf) {
    std::vector<double> freqs = {1.0};
    auto root = build_huffman_tree(freqs);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->symbol, 0);
    EXPECT_EQ(root->left, nullptr);
    EXPECT_EQ(root->right, nullptr);
}

// Two-symbol distribution: root has two leaf children.
TEST(HuffmanTest, TwoSymbolTree) {
    std::vector<double> freqs = {0.6, 0.4};
    auto root = build_huffman_tree(freqs);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->symbol, -1);  // internal node
    ASSERT_NE(root->left, nullptr);
    ASSERT_NE(root->right, nullptr);
    EXPECT_GE(root->left->symbol, 0);   // leaf
    EXPECT_GE(root->right->symbol, 0);  // leaf
}

// Leaf symbols: each symbol index 0..n-1 must appear exactly once.
TEST(HuffmanTest, AllSymbolsPresent) {
    std::vector<double> freqs = {0.25, 0.25, 0.25, 0.25};
    auto root = build_huffman_tree(freqs);
    ASSERT_NE(root, nullptr);
    // Collect leaf symbols.
    std::vector<int> found;
    std::function<void(const Node*)> collect = [&](const Node* n) {
        if (!n) return;
        if (n->symbol >= 0) found.push_back(n->symbol);
        collect(n->left.get());
        collect(n->right.get());
    };
    collect(root.get());
    std::sort(found.begin(), found.end());
    ASSERT_EQ(found.size(), 4u);
    for (int i = 0; i < 4; ++i) EXPECT_EQ(found[i], i);
}
