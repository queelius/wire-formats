// kraft.hpp
// Pedagogical implementation for the post "Kraft's Inequality" in the
// "Algebra over Wire Formats" series. For the production version, see PFC:
// https://github.com/queelius/pfc

#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace kraft {

// ---- BinaryTree -- the trie view of a prefix-free code ----------------------
//
// A code is a set of codewords (each a string over {0, 1}). We represent the
// code as a binary tree where each codeword traces a root-to-leaf path:
// '0' goes left, '1' goes right. Codewords end at terminal nodes (is_codeword
// = true). A code is prefix-free iff no terminal node lies on the path to
// another terminal node.

class BinaryTree {
public:
    BinaryTree() : root_(std::make_unique<Node>()) {}

    // Insert a codeword (a string of '0' and '1' characters).
    // Idempotent: inserting the same codeword twice is a no-op.
    void insert(const std::string& codeword) {
        Node* cur = root_.get();
        for (char c : codeword) {
            assert((c == '0' || c == '1') && "codeword must be over {0,1}");
            std::unique_ptr<Node>& child = (c == '0') ? cur->left : cur->right;
            if (!child) child = std::make_unique<Node>();
            cur = child.get();
        }
        cur->is_codeword = true;
    }

    // Returns true iff the codeword is in the tree.
    bool contains(const std::string& codeword) const {
        const Node* cur = root_.get();
        for (char c : codeword) {
            const std::unique_ptr<Node>& child = (c == '0') ? cur->left : cur->right;
            if (!child) return false;
            cur = child.get();
        }
        return cur->is_codeword;
    }

    // Returns true iff no codeword in the tree is a prefix of another.
    // Equivalently: no terminal node has any descendants that are terminal.
    [[nodiscard]] bool is_prefix_free() const {
        return is_prefix_free_recursive(root_.get(), false);
    }

private:
    struct Node {
        std::unique_ptr<Node> left;   // '0' branch
        std::unique_ptr<Node> right;  // '1' branch
        bool is_codeword = false;
    };

    std::unique_ptr<Node> root_;

    static bool is_prefix_free_recursive(const Node* node, bool ancestor_is_codeword) {
        if (!node) return true;
        // If this node is a codeword AND we passed through a codeword on the
        // way down, the ancestor codeword is a prefix of this one. Violation.
        // Equivalently: if this node is a codeword AND has any children that
        // are also codewords, this codeword is a prefix of those.
        if (node->is_codeword && ancestor_is_codeword) return false;
        bool below = node->is_codeword;
        if (!is_prefix_free_recursive(node->left.get(), ancestor_is_codeword || below)) return false;
        if (!is_prefix_free_recursive(node->right.get(), ancestor_is_codeword || below)) return false;
        return true;
    }
};

}  // namespace kraft
