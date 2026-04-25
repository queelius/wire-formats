#pragma once
// algorithms.hpp - Generic algorithms for packed data
// Stepanov would be proud

#include "core.hpp"
#include "packed.hpp"
#include <algorithm>
#include <numeric>
#include <functional>
#include <iterator>

namespace pfc::algorithms {

// ============================================================
//  Encoding/Decoding Algorithms
// ============================================================

// Encode a range of values into a buffer
template<std::ranges::input_range Range, typename Codec>
    requires std::convertible_to<std::ranges::range_value_t<Range>, 
                                 typename Codec::value_type>
void encode_range(const Range& range, auto& sink) requires BitSink<decltype(sink)> {
    for (const auto& value : range) {
        Codec::encode(value, sink);
    }
}

// Decode n values from a source
template<typename T, typename Codec, typename Source>
std::vector<T> decode_n(Source& source, size_t count) requires BitSource<Source> {
    std::vector<T> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        result.push_back(Codec::template decode<T>(source));
    }
    return result;
}

// Decode values until source is exhausted
template<typename T, typename Codec, typename Source>
std::vector<T> decode_all(Source& source) requires BitSource<Source> {
    std::vector<T> result;
    while (source.peek()) {
        result.push_back(Codec::template decode<T>(source));
    }
    return result;
}

// ============================================================
//  Transform Algorithms
// ============================================================

// Transform packed values using a function
template<PackedValue Input, PackedValue Output, typename UnaryOp>
Output transform_packed(const Input& input, UnaryOp op) {
    return Output{op(input.value())};
}

// Transform a range of packed values
template<std::ranges::input_range Range, PackedValue Output, typename UnaryOp>
    requires PackedValue<std::ranges::range_value_t<Range>>
auto transform_packed_range(const Range& range, UnaryOp op) {
    using Input = std::ranges::range_value_t<Range>;
    std::vector<Output> result;
    
    for (const auto& elem : range) {
        result.emplace_back(op(elem.value()));
    }
    
    return result;
}

// ============================================================
//  Compression Ratio Analysis
// ============================================================

struct CompressionStats {
    size_t original_bits;
    size_t compressed_bits;
    double ratio;
    double bits_per_value;
};

template<typename T, typename Codec>
CompressionStats analyze_compression(const std::vector<T>& values) {
    // Calculate original size (assuming fixed width)
    size_t original_bits = values.size() * sizeof(T) * 8;
    
    // Calculate compressed size
    std::vector<uint8_t> buffer;
    buffer.reserve(original_bits / 8 + 1024);  // Reasonable initial size
    
    BitWriter writer(buffer.data());
    for (const auto& value : values) {
        Codec::encode(value, writer);
    }
    writer.align();
    
    size_t compressed_bits = (writer.position() - buffer.data()) * 8;
    
    return CompressionStats{
        .original_bits = original_bits,
        .compressed_bits = compressed_bits,
        .ratio = static_cast<double>(original_bits) / compressed_bits,
        .bits_per_value = static_cast<double>(compressed_bits) / values.size()
    };
}

// ============================================================
//  Adaptive Codec Selection
// ============================================================

template<typename T>
struct CodecSelector {
    template<typename Codec>
    struct CodecInfo {
        using codec_type = Codec;
        CompressionStats stats;
    };
    
    template<typename... Codecs>
    static auto select_best(const std::vector<T>& sample) {
        std::tuple<CodecInfo<Codecs>...> infos{
            CodecInfo<Codecs>{analyze_compression<T, Codecs>(sample)}...
        };
        
        // Find codec with best compression ratio
        auto best_codec = std::apply([](const auto&... info) {
            const void* best = nullptr;
            double best_ratio = 0.0;
            
            ((info.stats.ratio > best_ratio ? 
              (best_ratio = info.stats.ratio, best = &info) : 0), ...);
            
            return best;
        }, infos);
        
        return best_codec;
    }
};

// ============================================================
//  Streaming Operations
// ============================================================

template<PackedValue P>
class PackedStream {
private:
    BitReader reader_;
    
public:
    explicit PackedStream(std::span<const uint8_t> data) 
        : reader_(data) {}
    
    // Read next value
    std::optional<P> next() {
        if (!reader_.peek()) {
            return std::nullopt;
        }
        return P::decode(reader_);
    }
    
    // Read n values
    std::vector<P> take(size_t n) {
        std::vector<P> result;
        result.reserve(n);
        
        for (size_t i = 0; i < n && reader_.peek(); ++i) {
            result.push_back(P::decode(reader_));
        }
        
        return result;
    }
    
    // Skip n values
    void skip(size_t n) {
        for (size_t i = 0; i < n && reader_.peek(); ++i) {
            P::decode(reader_);
        }
    }
    
    // Check if more values available
    [[nodiscard]] bool has_more() const noexcept {
        return reader_.peek();
    }
};

// ============================================================
//  Delta Encoding/Decoding
// ============================================================

template<typename T, typename Codec>
class DeltaCodec {
public:
    template<typename Sink>
    static void encode_deltas(const std::vector<T>& values, Sink& sink) requires BitSink<Sink> {
        if (values.empty()) return;
        
        // Encode first value directly
        Codec::encode(values[0], sink);
        
        // Encode deltas
        for (size_t i = 1; i < values.size(); ++i) {
            T delta = values[i] - values[i-1];
            if constexpr (std::is_signed_v<T>) {
                codecs::SignedGamma::encode(delta, sink);
            } else {
                Codec::encode(delta, sink);
            }
        }
    }
    
    template<typename Source>
    static std::vector<T> decode_deltas(Source& source, size_t count) requires BitSource<Source> {
        if (count == 0) return {};
        
        std::vector<T> result;
        result.reserve(count);
        
        // Decode first value
        result.push_back(Codec::template decode<T>(source));
        
        // Decode deltas and reconstruct values
        for (size_t i = 1; i < count; ++i) {
            T delta;
            if constexpr (std::is_signed_v<T>) {
                delta = codecs::SignedGamma::template decode<T>(source);
            } else {
                delta = Codec::template decode<T>(source);
            }
            result.push_back(result.back() + delta);
        }
        
        return result;
    }
};

// ============================================================
//  Run-Length Encoding
// ============================================================

template<typename T, typename ValueCodec, typename CountCodec = codecs::EliasGamma>
class RunLengthCodec {
public:
    template<typename Sink>
    static void encode(const std::vector<T>& values, Sink& sink) requires BitSink<Sink> {
        if (values.empty()) {
            CountCodec::encode(0u, sink);
            return;
        }
        
        // Count runs
        std::vector<std::pair<T, uint32_t>> runs;
        T current = values[0];
        uint32_t count = 1;
        
        for (size_t i = 1; i < values.size(); ++i) {
            if (values[i] == current) {
                ++count;
            } else {
                runs.emplace_back(current, count);
                current = values[i];
                count = 1;
            }
        }
        runs.emplace_back(current, count);
        
        // Encode number of runs
        CountCodec::encode(static_cast<uint32_t>(runs.size()), sink);
        
        // Encode runs
        for (const auto& [value, run_count] : runs) {
            ValueCodec::encode(value, sink);
            CountCodec::encode(run_count - 1, sink);  // -1 since count >= 1
        }
    }
    
    template<typename Source>
    static std::vector<T> decode(Source& source) requires BitSource<Source> {
        uint32_t num_runs = CountCodec::template decode<uint32_t>(source);
        
        std::vector<T> result;
        for (uint32_t i = 0; i < num_runs; ++i) {
            T value = ValueCodec::template decode<T>(source);
            uint32_t count = CountCodec::template decode<uint32_t>(source) + 1;
            
            for (uint32_t j = 0; j < count; ++j) {
                result.push_back(value);
            }
        }
        
        return result;
    }
};

// ============================================================
//  Parallel Encoding (for large datasets)
// ============================================================

template<typename T, typename Codec>
struct ParallelEncoder {
    static std::vector<uint8_t> encode(const std::vector<T>& values, 
                                       size_t chunk_size = 1024) {
        if (values.empty()) return {};
        
        // Split into chunks
        std::vector<std::vector<uint8_t>> chunks;
        chunks.reserve((values.size() + chunk_size - 1) / chunk_size);
        
        for (size_t i = 0; i < values.size(); i += chunk_size) {
            size_t end = std::min(i + chunk_size, values.size());
            
            // Encode chunk
            std::vector<uint8_t> chunk_buffer;
            chunk_buffer.resize(chunk_size * sizeof(T));  // Worst case
            
            BitWriter writer(chunk_buffer.data());
            for (size_t j = i; j < end; ++j) {
                Codec::encode(values[j], writer);
            }
            writer.align();
            
            chunk_buffer.resize(writer.position() - chunk_buffer.data());
            chunks.push_back(std::move(chunk_buffer));
        }
        
        // Combine chunks
        std::vector<uint8_t> result;
        size_t total_size = 0;
        for (const auto& chunk : chunks) {
            total_size += chunk.size();
        }
        
        result.reserve(total_size + chunks.size() * 4);  // Space for sizes
        
        // Write chunk count
        BitWriter header_writer(result.data());
        codecs::EliasGamma::encode(static_cast<uint32_t>(chunks.size()), 
                                  header_writer);
        
        // Write chunks with sizes
        for (const auto& chunk : chunks) {
            codecs::EliasGamma::encode(static_cast<uint32_t>(chunk.size()), 
                                      header_writer);
            result.insert(result.end(), chunk.begin(), chunk.end());
        }
        
        return result;
    }
};

} // namespace pfc::algorithms