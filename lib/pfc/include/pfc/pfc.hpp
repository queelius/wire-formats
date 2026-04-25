#pragma once
// pfc.hpp - Prefix-Free Codec Library
// A unified, elegant library for data compression
// "Perfection is achieved not when there is nothing more to add,
//  but when there is nothing left to take away." - Antoine de Saint-Exupéry

// Core components
#include "core.hpp"
#include "codecs.hpp"
#include "packed.hpp"
#include "algorithms.hpp"

// Production features
#include "error_handling.hpp"
#include "allocator_support.hpp"
#include "huffman.hpp"
#include "lz77.hpp"
#include "crc.hpp"

// Advanced types
#include "algebraic.hpp"
#include "numeric_codecs.hpp"
#include "stl_integration.hpp"
#include "coordinates.hpp"

// I/O and advanced coding
#include "stream_io.hpp"
#include "arithmetic_coding.hpp"

#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace pfc {

// Version information
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

// Bring commonly used types into pfc namespace
using codecs::EliasGamma;
using codecs::EliasDelta;
using codecs::Fibonacci;
using codecs::Unary;
using codecs::Boolean;
using codecs::SignedGamma;
using codecs::SignedDelta;

template<size_t K>
using Rice = codecs::Rice<K>;

template<size_t Width>
using Fixed = codecs::Fixed<Width>;

// Common packed types
using U32 = PackedU32<EliasGamma>;
using U64 = PackedU64<EliasGamma>;
using I32 = PackedI32<SignedGamma>;
using I64 = PackedI64<SignedGamma>;
using Bool = PackedBool;

// ============================================================
//  High-Level API - Simple and powerful
// ============================================================

// Compress data to bytes
template<typename T, typename Codec = EliasGamma>
std::vector<uint8_t> compress(const std::vector<T>& data) {
    if (data.empty()) return {};
    
    // Estimate size (conservative)
    size_t estimated_bytes = data.size() * sizeof(T) / 2 + 128;
    std::vector<uint8_t> buffer(estimated_bytes);
    
    BitWriter writer{std::span<uint8_t>{buffer}};
    
    // Write count
    EliasGamma::encode(static_cast<uint32_t>(data.size()), writer);
    
    // Write values
    for (const auto& value : data) {
        Codec::encode(value, writer);
    }
    
    writer.align();
    
    // Resize to actual size
    buffer.resize(writer.bytes_written());
    return buffer;
}

// Decompress bytes to data
template<typename T, typename Codec = EliasGamma>
std::vector<T> decompress(const std::vector<uint8_t>& compressed) {
    if (compressed.empty()) return {};
    
    BitReader reader(compressed);
    
    // Read count
    uint32_t count = EliasGamma::template decode<uint32_t>(reader);
    
    // Read values
    std::vector<T> result;
    result.reserve(count);
    
    for (uint32_t i = 0; i < count; ++i) {
        result.push_back(Codec::template decode<T>(reader));
    }
    
    return result;
}

// ============================================================
//  Codec Selection Helper
// ============================================================

enum class DataDistribution {
    Uniform,       // All values equally likely -> Fixed width
    Geometric,     // P(n) ∝ p^n -> Rice or Elias Gamma  
    PowerLaw,      // P(n) ∝ n^(-α) -> Elias Delta
    Sparse,        // Few unique values -> Run-length
    Unknown        // Test multiple codecs
};

template<typename T>
struct OptimalCodec {
    template<DataDistribution Dist>
    using type = std::conditional_t<
        Dist == DataDistribution::Uniform,
        Fixed<sizeof(T) * 8>,
        std::conditional_t<
            Dist == DataDistribution::Geometric,
            EliasGamma,
            std::conditional_t<
                Dist == DataDistribution::PowerLaw,
                EliasDelta,
                EliasGamma  // Default
            >
        >
    >;
};

// ============================================================
//  Utility Functions
// ============================================================

// Calculate entropy of data
template<typename T>
double calculate_entropy(const std::vector<T>& data) {
    if (data.empty()) return 0.0;
    
    std::unordered_map<T, size_t> freq;
    for (const auto& val : data) {
        ++freq[val];
    }
    
    double entropy = 0.0;
    double n = static_cast<double>(data.size());
    
    for (const auto& [_, count] : freq) {
        double p = count / n;
        if (p > 0) {
            entropy -= p * std::log2(p);
        }
    }
    
    return entropy;
}

// Estimate best codec based on data sample
template<typename T>
const char* suggest_codec(const std::vector<T>& sample) {
    if (sample.empty()) return "Unknown";
    
    // Calculate statistics
    auto [min_it, max_it] = std::minmax_element(sample.begin(), sample.end());
    T min_val = *min_it;
    T max_val = *max_it;
    T range = max_val - min_val;
    
    // Count unique values
    std::unordered_set<T> unique(sample.begin(), sample.end());
    double unique_ratio = static_cast<double>(unique.size()) / sample.size();
    
    // Calculate average value
    double avg = std::accumulate(sample.begin(), sample.end(), 0.0) / sample.size();
    
    // Simple heuristics
    if (unique_ratio < 0.1) {
        return "RunLength";
    } else if (range < 256) {
        return "Fixed<8>";
    } else if (avg < 100 && max_val < 1000) {
        return "EliasGamma";
    } else if (max_val > 1'000'000) {
        return "EliasDelta";
    } else {
        return "EliasGamma";  // Good default
    }
}

} // namespace pfc