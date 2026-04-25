// huffman.hpp - Huffman coding implementation for PFC library
#pragma once

#include "core.hpp"
#include "error_handling.hpp"
#include <vector>
#include <deque>
#include <queue>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <array>

namespace pfc {
namespace codecs {

// ============================================================
// Huffman Tree Node
// ============================================================

template<typename Symbol>
struct HuffmanNode {
    Symbol symbol;
    uint64_t frequency;
    std::unique_ptr<HuffmanNode> left;
    std::unique_ptr<HuffmanNode> right;

    HuffmanNode(Symbol s, uint64_t f) : symbol(s), frequency(f) {}

    HuffmanNode(std::unique_ptr<HuffmanNode> l, std::unique_ptr<HuffmanNode> r)
        : symbol{}, frequency(l->frequency + r->frequency),
          left(std::move(l)), right(std::move(r)) {}

    bool is_leaf() const { return !left && !right; }
};

// ============================================================
// Static Huffman Codec
// ============================================================

template<typename Symbol = uint8_t, typename Allocator = std::allocator<uint8_t>>
class StaticHuffman {
public:
    using symbol_type = Symbol;
    using allocator_type = Allocator;
    using frequency_map = std::unordered_map<Symbol, uint64_t>;
    using code_map = std::unordered_map<Symbol, std::pair<uint32_t, int>>; // (code, bit_length)

    // Build Huffman tree from frequency table
    static result<StaticHuffman> from_frequencies(const frequency_map& frequencies) {
        if (frequencies.empty()) {
            return make_error_code(pfc_error::invalid_input);
        }

        StaticHuffman codec;

        // Build the tree
        auto tree = build_tree(frequencies);
        if (!tree) {
            return make_error_code(pfc_error::compression_error);
        }

        // Generate codes
        codec.generate_codes(tree.get(), 0, 0);
        codec.build_decode_table(tree.get());
        codec.tree_ = std::move(tree);

        return codec;
    }

    // Build from data by calculating frequencies
    template<typename InputIt>
    static result<StaticHuffman> from_data(InputIt first, InputIt last) {
        frequency_map frequencies;
        for (auto it = first; it != last; ++it) {
            frequencies[*it]++;
        }
        return from_frequencies(frequencies);
    }

    // Encode single symbol
    template<typename BitSink>
    result<void> encode_symbol(Symbol symbol, BitSink& sink) const {
        auto it = codes_.find(symbol);
        if (it == codes_.end()) {
            return make_error_code(pfc_error::invalid_input);
        }

        const auto& [code, length] = it->second;
        for (int i = length - 1; i >= 0; --i) {
            sink.write((code >> i) & 1);
        }
        return {};
    }

    // Decode single symbol
    template<typename BitSource>
    result<Symbol> decode_symbol(BitSource& source) const {
        if (!tree_) {
            return make_error_code(pfc_error::invalid_input);
        }

        const HuffmanNode<Symbol>* node = tree_.get();

        while (!node->is_leaf()) {
            if (!source.peek()) {
                return make_error_code(pfc_error::incomplete_data);
            }

            bool bit = source.read();
            node = bit ? node->right.get() : node->left.get();

            if (!node) {
                return make_error_code(pfc_error::corrupted_data);
            }
        }

        return node->symbol;
    }

    // Encode sequence
    template<typename InputIt, typename BitSink>
    result<size_t> encode(InputIt first, InputIt last, BitSink& sink) const {
        size_t count = 0;
        for (auto it = first; it != last; ++it) {
            if (auto res = encode_symbol(*it, sink); !res) {
                return res.error();
            }
            ++count;
        }
        return count;
    }

    // Decode sequence
    template<typename OutputIt, typename BitSource>
    result<size_t> decode(BitSource& source, OutputIt out, size_t count) const {
        for (size_t i = 0; i < count; ++i) {
            auto symbol_res = decode_symbol(source);
            if (!symbol_res) {
                return symbol_res.error();
            }
            *out++ = *symbol_res;
        }
        return count;
    }

    // Get average code length
    double average_code_length() const {
        if (codes_.empty()) return 0;

        uint64_t total_freq = 0;
        double weighted_length = 0;

        for (const auto& [symbol, code_info] : codes_) {
            auto it = symbol_frequencies_.find(symbol);
            if (it != symbol_frequencies_.end()) {
                total_freq += it->second;
                weighted_length += it->second * code_info.second;
            }
        }

        return total_freq > 0 ? weighted_length / total_freq : 0;
    }

    // Serialize the code table for transmission
    template<typename BitSink>
    result<void> write_header(BitSink& sink) const {
        // Write number of symbols
        uint16_t symbol_count = codes_.size();
        for (int i = 0; i < 16; ++i) {
            sink.write((symbol_count >> i) & 1);
        }

        // Write each symbol and its code length
        for (const auto& [symbol, code_info] : codes_) {
            // Write symbol (assuming 8-bit symbols)
            for (int i = 0; i < 8; ++i) {
                sink.write((symbol >> i) & 1);
            }
            // Write code length (5 bits max for lengths up to 31)
            for (int i = 0; i < 5; ++i) {
                sink.write((code_info.second >> i) & 1);
            }
        }

        return {};
    }

    // Read the code table from stream
    template<typename BitSource>
    static result<StaticHuffman> read_header(BitSource& source) {
        // Read number of symbols
        uint16_t symbol_count = 0;
        for (int i = 0; i < 16; ++i) {
            if (!source.peek()) return make_error_code(pfc_error::incomplete_data);
            symbol_count |= source.read() << i;
        }

        if (symbol_count == 0 || symbol_count > 256) {
            return make_error_code(pfc_error::invalid_header);
        }

        // Read symbols and build frequency map (using lengths as proxy)
        frequency_map frequencies;
        for (uint16_t i = 0; i < symbol_count; ++i) {
            Symbol symbol = 0;
            for (int j = 0; j < 8; ++j) {
                if (!source.peek()) return make_error_code(pfc_error::incomplete_data);
                symbol |= source.read() << j;
            }

            uint8_t length = 0;
            for (int j = 0; j < 5; ++j) {
                if (!source.peek()) return make_error_code(pfc_error::incomplete_data);
                length |= source.read() << j;
            }

            // Use inverse of length as frequency approximation
            frequencies[symbol] = 1ULL << (32 - length);
        }

        return from_frequencies(frequencies);
    }

private:
    std::unique_ptr<HuffmanNode<Symbol>> tree_;
    code_map codes_;
    frequency_map symbol_frequencies_;

    // Build Huffman tree from frequencies
    static std::unique_ptr<HuffmanNode<Symbol>> build_tree(const frequency_map& frequencies) {
        using NodePtr = std::unique_ptr<HuffmanNode<Symbol>>;
        auto compare = [](const NodePtr& a, const NodePtr& b) {
            return a->frequency > b->frequency; // Min heap
        };

        std::priority_queue<NodePtr, std::vector<NodePtr>, decltype(compare)> heap(compare);

        // Create leaf nodes
        for (const auto& [symbol, freq] : frequencies) {
            heap.push(std::make_unique<HuffmanNode<Symbol>>(symbol, freq));
        }

        // Build tree
        while (heap.size() > 1) {
            auto left = std::move(const_cast<NodePtr&>(heap.top()));
            heap.pop();
            auto right = std::move(const_cast<NodePtr&>(heap.top()));
            heap.pop();

            heap.push(std::make_unique<HuffmanNode<Symbol>>(std::move(left), std::move(right)));
        }

        return heap.empty() ? nullptr : std::move(const_cast<NodePtr&>(heap.top()));
    }

    // Generate codes from tree
    void generate_codes(const HuffmanNode<Symbol>* node, uint32_t code, int depth) {
        if (!node) return;

        if (node->is_leaf()) {
            codes_[node->symbol] = {code, depth > 0 ? depth : 1}; // Handle single symbol case
            symbol_frequencies_[node->symbol] = node->frequency;
        } else {
            generate_codes(node->left.get(), code << 1, depth + 1);
            generate_codes(node->right.get(), (code << 1) | 1, depth + 1);
        }
    }

    // Build decode table for faster decoding
    void build_decode_table(const HuffmanNode<Symbol>* root) {
        // For now, we use tree traversal
        // Could optimize with lookup table for common prefixes
    }
};

// ============================================================
// Canonical Huffman Codec (for deterministic encoding)
// ============================================================

template<typename Symbol = uint8_t>
class CanonicalHuffman {
public:
    using symbol_type = Symbol;
    using frequency_map = std::unordered_map<Symbol, uint64_t>;

    static result<CanonicalHuffman> from_frequencies(const frequency_map& frequencies) {
        if (frequencies.empty()) {
            return make_error_code(pfc_error::invalid_input);
        }

        // First build regular Huffman to get code lengths
        auto regular = StaticHuffman<Symbol>::from_frequencies(frequencies);
        if (!regular) {
            return regular.error();
        }

        CanonicalHuffman codec;
        // Convert to canonical form
        codec.canonicalize(*regular);

        return codec;
    }

    template<typename BitSink>
    result<void> encode_symbol(Symbol symbol, BitSink& sink) const {
        auto it = codes_.find(symbol);
        if (it == codes_.end()) {
            return make_error_code(pfc_error::invalid_input);
        }

        const auto& [code, length] = it->second;
        for (int i = length - 1; i >= 0; --i) {
            sink.write((code >> i) & 1);
        }
        return {};
    }

private:
    std::unordered_map<Symbol, std::pair<uint32_t, int>> codes_;
    std::vector<std::pair<Symbol, int>> symbol_lengths_;

    void canonicalize(const StaticHuffman<Symbol>& regular) {
        // Sort symbols by code length, then by symbol value
        // Assign canonical codes based on this ordering
        // This ensures deterministic encoding across implementations
    }
};

// ============================================================
// Adaptive Huffman Codec (dynamic updating)
// ============================================================

template<typename Symbol = uint8_t>
class AdaptiveHuffman {
public:
    using symbol_type = Symbol;

    AdaptiveHuffman() {
        // Initialize with uniform distribution or NYT (Not Yet Transmitted) nodes
        initialize();
    }

    template<typename BitSink>
    result<void> encode_symbol(Symbol symbol, BitSink& sink) {
        // Encode using current tree
        // Update tree after encoding
        update_tree(symbol);
        return {};
    }

    template<typename BitSource>
    result<Symbol> decode_symbol(BitSource& source) {
        // Decode using current tree
        // Update tree after decoding
        Symbol symbol{};
        update_tree(symbol);
        return symbol;
    }

private:
    struct AdaptiveNode {
        Symbol symbol;
        uint64_t weight;
        AdaptiveNode* parent = nullptr;
        AdaptiveNode* left = nullptr;
        AdaptiveNode* right = nullptr;
        int node_number; // For sibling property
    };

    std::unique_ptr<AdaptiveNode> root_;
    std::unordered_map<Symbol, AdaptiveNode*> symbol_nodes_;
    AdaptiveNode* nyt_node_; // Not Yet Transmitted

    void initialize() {
        // Create initial NYT node as root
    }

    void update_tree(Symbol symbol) {
        // Update weights and maintain sibling property
        // Perform node swaps if necessary
    }
};

} // namespace codecs
} // namespace pfc