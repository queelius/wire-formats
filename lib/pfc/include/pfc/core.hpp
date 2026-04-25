#pragma once
// core.hpp - Core concepts and bit I/O for prefix-free codecs
// Beautiful, elegant, and inevitable.

#include <cstdint>
#include <concepts>
#include <bit>
#include <span>
#include <ranges>
#include <optional>

namespace pfc {

// ============================================================
//  Core Concepts - The essence of what we're building
// ============================================================

template<typename T>
concept BitSink = requires(T& sink, bool bit) {
    { sink.write(bit) } -> std::same_as<void>;
};

template<typename T>
concept BitSource = requires(T& source) {
    { source.read() } -> std::same_as<bool>;
    { source.peek() } -> std::convertible_to<bool>;  // Check if more bits available
};

template<typename C, typename T, typename Sink>
concept Encoder = BitSink<Sink> && requires(const T& value, Sink& sink) {
    { C::encode(value, sink) } -> std::same_as<void>;
};

template<typename C, typename T, typename Source>
concept Decoder = BitSource<Source> && requires(Source& source) {
    { C::decode(source) } -> std::same_as<T>;
};

// Forward declarations for Codec concept
class BitWriter;
class BitReader;

template<typename C, typename T>
concept Codec = requires {
    // A codec must work with any BitSink and BitSource
    requires Encoder<C, T, BitWriter>;
    requires Decoder<C, T, BitReader>;
};

// ============================================================
//  Bit I/O - Zero-copy, efficient, beautiful
// ============================================================

class BitWriter {
    uint8_t* ptr_;
    uint8_t* start_;  // Track start position
    uint8_t  byte_ = 0;
    uint8_t  bit_pos_ = 0;

public:
    explicit BitWriter(uint8_t* ptr) noexcept : ptr_(ptr), start_(ptr) {}
    explicit BitWriter(std::span<uint8_t> buffer) noexcept : ptr_(buffer.data()), start_(buffer.data()) {}

    void write(bool bit) noexcept {
        byte_ |= (bit ? 1u : 0u) << bit_pos_;
        if (++bit_pos_ == 8) {
            *ptr_++ = byte_;
            byte_ = 0;
            bit_pos_ = 0;
        }
    }

    // Write multiple bits at once (LSB first)
    void write_bits(uint64_t bits, size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            write((bits >> i) & 1);
        }
    }

    // Align to byte boundary, padding with zeros
    void align() noexcept {
        if (bit_pos_ > 0) {
            *ptr_++ = byte_;
            byte_ = 0;
            bit_pos_ = 0;
        }
    }

    // Get number of bytes written (including partial byte)
    [[nodiscard]] size_t bytes_written() const noexcept {
        return (ptr_ - start_) + (bit_pos_ > 0 ? 1 : 0);
    }

    // Get current position for later retrieval
    [[nodiscard]] uint8_t* position() const noexcept {
        return ptr_;
    }

    // Get start position
    [[nodiscard]] uint8_t* start() const noexcept {
        return start_;
    }
};

class BitReader {
    const uint8_t* ptr_;
    const uint8_t* end_;
    uint8_t byte_ = 0;
    uint8_t bit_pos_ = 8;  // Start at 8 to force initial read

public:
    BitReader(const uint8_t* ptr, size_t size) noexcept 
        : ptr_(ptr), end_(ptr + size) {}
    
    explicit BitReader(std::span<const uint8_t> buffer) noexcept
        : ptr_(buffer.data()), end_(buffer.data() + buffer.size()) {}

    [[nodiscard]] bool peek() const noexcept {
        return ptr_ < end_ || bit_pos_ < 8;
    }

    bool read() noexcept {
        if (bit_pos_ == 8) {
            if (ptr_ >= end_) return false;  // EOF
            byte_ = *ptr_++;
            bit_pos_ = 0;
        }
        return (byte_ >> bit_pos_++) & 1;
    }

    // Read multiple bits at once (LSB first)
    [[nodiscard]] uint64_t read_bits(size_t count) noexcept {
        uint64_t result = 0;
        for (size_t i = 0; i < count; ++i) {
            result |= static_cast<uint64_t>(read()) << i;
        }
        return result;
    }

    // Skip to next byte boundary
    void align() noexcept {
        bit_pos_ = 8;
    }

    // Get current position
    [[nodiscard]] const uint8_t* position() const noexcept { 
        return ptr_ - (bit_pos_ < 8 ? 1 : 0); 
    }
};

// ============================================================
//  Utility Functions - Building blocks for elegance
// ============================================================

// Count leading zeros (portable, efficient)
template<std::unsigned_integral T>
[[nodiscard]] constexpr int count_leading_zeros(T x) noexcept {
    if (x == 0) return std::numeric_limits<T>::digits;
    if constexpr (sizeof(T) <= sizeof(unsigned int)) {
        return __builtin_clz(x) - (32 - std::numeric_limits<T>::digits);
    } else if constexpr (sizeof(T) <= sizeof(unsigned long)) {
        return __builtin_clzl(x) - (64 - std::numeric_limits<T>::digits);
    } else {
        return __builtin_clzll(x) - (64 - std::numeric_limits<T>::digits);
    }
}

// Most significant bit position (0-based)
template<std::unsigned_integral T>
[[nodiscard]] constexpr int msb_position(T x) noexcept {
    return x == 0 ? -1 : std::numeric_limits<T>::digits - 1 - count_leading_zeros(x);
}

} // namespace pfc