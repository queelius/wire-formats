#pragma once
// codecs.hpp - Elegant prefix-free codec implementations
// Each codec is a work of art, simple yet powerful

#include "core.hpp"
#include <limits>
#include <array>
#include <vector>
#include <utility>

namespace pfc::codecs {

// ============================================================
//  Unary Codec - The simplest possible variable-length code
// ============================================================

struct Unary {
    template<std::unsigned_integral T, typename Sink>
    static void encode(T value, Sink& sink) requires BitSink<Sink> {
        // Write n zeros followed by a one
        for (T i = 0; i < value; ++i) {
            sink.write(false);
        }
        sink.write(true);
    }

    template<std::unsigned_integral T, typename Source>
    static T decode(Source& source) requires BitSource<Source> {
        T count = 0;
        while (!source.read()) {
            ++count;
        }
        return count;
    }
};

// ============================================================
//  Elias Gamma - The fundamental universal code
// ============================================================

struct EliasGamma {
    template<std::unsigned_integral T, typename Sink>
    static void encode(T value, Sink& sink) requires BitSink<Sink> {
        ++value; // Map 0→1, 1→2, etc. for gamma coding
        
        int len = msb_position(value) + 1;
        
        // Write len-1 zeros
        for (int i = 0; i < len - 1; ++i) {
            sink.write(false);
        }
        
        // Write value in binary (MSB to LSB)
        for (int i = len - 1; i >= 0; --i) {
            sink.write((value >> i) & 1);
        }
    }

    template<std::unsigned_integral T>
    static T decode(auto& source) requires BitSource<decltype(source)> {
        // Count leading zeros
        int zeros = 0;
        while (!source.read()) {
            ++zeros;
        }
        
        // Read zeros more bits
        T value = 1;
        for (int i = 0; i < zeros; ++i) {
            value = (value << 1) | source.read();
        }
        
        return value - 1; // Map back: 1→0, 2→1, etc.
    }
};

// ============================================================
//  Elias Delta - More efficient for larger numbers
// ============================================================

struct EliasDelta {
    template<std::unsigned_integral T>
    static void encode(T value, auto& sink) requires BitSink<decltype(sink)> {
        ++value; // Map 0→1
        
        int len = msb_position(value) + 1;
        
        // Encode len using gamma
        EliasGamma::encode(static_cast<T>(len - 1), sink);
        
        // Write remaining bits of value (skip MSB which is always 1)
        for (int i = len - 2; i >= 0; --i) {
            sink.write((value >> i) & 1);
        }
    }

    template<std::unsigned_integral T>
    static T decode(auto& source) requires BitSource<decltype(source)> {
        // Decode length
        uint32_t len = EliasGamma::decode<uint32_t>(source) + 1;
        
        // Read remaining bits
        T value = 1;
        for (uint32_t i = 0; i < len - 1; ++i) {
            value = (value << 1) | source.read();
        }
        
        return value - 1;
    }
};

// ============================================================
//  Fibonacci Codec - Based on Fibonacci sequence
// ============================================================

struct Fibonacci {
private:
    static constexpr size_t MAX_FIBS = 92; // Enough for 64-bit
    
    template<typename T>
    static constexpr std::array<T, MAX_FIBS> compute_fibs() {
        std::array<T, MAX_FIBS> fibs{};
        fibs[0] = 1;
        fibs[1] = 2;
        for (size_t i = 2; i < MAX_FIBS && fibs[i-1] < std::numeric_limits<T>::max() / 2; ++i) {
            fibs[i] = fibs[i-1] + fibs[i-2];
        }
        return fibs;
    }

public:
    template<std::unsigned_integral T>
    static void encode(T value, auto& sink) requires BitSink<decltype(sink)> {
        if (value == 0) {
            sink.write(true);
            sink.write(true);
            return;
        }
        
        constexpr auto fibs = compute_fibs<T>();
        ++value; // Map to positive integers
        
        // Find Fibonacci representation
        std::array<bool, MAX_FIBS> bits{};
        int highest = 0;
        
        for (int i = MAX_FIBS - 1; i >= 0; --i) {
            if (fibs[i] <= value && fibs[i] > 0) {
                bits[i] = true;
                value -= fibs[i];
                highest = std::max(highest, i);
            }
        }
        
        // Write bits from lowest to highest, then terminator
        for (int i = 0; i <= highest; ++i) {
            sink.write(bits[i]);
        }
        sink.write(true); // Terminator
    }

    template<std::unsigned_integral T>
    static T decode(auto& source) requires BitSource<decltype(source)> {
        constexpr auto fibs = compute_fibs<T>();
        
        T value = 0;
        bool prev_bit = false;
        
        for (size_t i = 0; i < MAX_FIBS; ++i) {
            bool bit = source.read();
            
            if (bit && prev_bit) {
                // Found terminator (two consecutive 1s)
                return value > 0 ? value - 1 : 0;
            }
            
            if (bit) {
                value += fibs[i];
            }
            
            prev_bit = bit;
        }
        
        return value > 0 ? value - 1 : 0;
    }
};

// ============================================================
//  Rice Codec - Optimal for geometric distributions
// ============================================================

template<size_t K>
struct Rice {
    static_assert(K < 64, "K must be less than 64");
    
    template<std::unsigned_integral T>
    static void encode(T value, auto& sink) requires BitSink<decltype(sink)> {
        T q = value >> K;  // Quotient
        T r = value & ((1u << K) - 1);  // Remainder
        
        // Encode quotient in unary
        Unary::encode(q, sink);
        
        // Encode remainder in binary
        for (size_t i = 0; i < K; ++i) {
            sink.write((r >> i) & 1);
        }
    }

    template<std::unsigned_integral T>
    static T decode(auto& source) requires BitSource<decltype(source)> {
        // Decode quotient
        T q = Unary::decode<T>(source);
        
        // Decode remainder
        T r = 0;
        for (size_t i = 0; i < K; ++i) {
            r |= static_cast<T>(source.read()) << i;
        }
        
        return (q << K) | r;
    }
};

// ============================================================
//  Fixed Width - Sometimes simplicity wins
// ============================================================

template<size_t Width>
struct Fixed {
    static_assert(Width > 0 && Width <= 64, "Width must be between 1 and 64");
    
    template<std::unsigned_integral T>
    static void encode(T value, auto& sink) requires BitSink<decltype(sink)> {
        for (size_t i = 0; i < Width; ++i) {
            sink.write((value >> i) & 1);
        }
    }

    template<std::unsigned_integral T>
    static T decode(auto& source) requires BitSource<decltype(source)> {
        T value = 0;
        for (size_t i = 0; i < Width; ++i) {
            value |= static_cast<T>(source.read()) << i;
        }
        return value;
    }
};

// ============================================================
//  Adaptive Codec - Choose codec based on value distribution
// ============================================================

template<typename SmallCodec, typename LargeCodec, std::unsigned_integral T = uint32_t>
struct Adaptive {
    static constexpr T threshold = 128;  // Tunable parameter
    
    static void encode(T value, auto& sink) requires BitSink<decltype(sink)> {
        if (value < threshold) {
            sink.write(false);  // Flag for small codec
            SmallCodec::encode(value, sink);
        } else {
            sink.write(true);   // Flag for large codec
            LargeCodec::encode(value - threshold, sink);
        }
    }

    static T decode(auto& source) requires BitSource<decltype(source)> {
        if (!source.read()) {
            return SmallCodec::template decode<T>(source);
        } else {
            return LargeCodec::template decode<T>(source) + threshold;
        }
    }
};

// ============================================================
//  Boolean Codec - The atom of information
// ============================================================

struct Boolean {
    static void encode(bool value, auto& sink) requires BitSink<decltype(sink)> {
        sink.write(value);
    }

    static bool decode(auto& source) requires BitSource<decltype(source)> {
        return source.read();
    }
};

// ============================================================
//  Signed Integer Codec - Zigzag encoding
// ============================================================

template<typename UnsignedCodec>
struct Signed {
    template<std::signed_integral T>
    static void encode(T value, auto& sink) requires BitSink<decltype(sink)> {
        using U = std::make_unsigned_t<T>;
        // Zigzag encoding: 0→0, -1→1, 1→2, -2→3, 2→4...
        U encoded = (value < 0) ? ((-value) * 2 - 1) : (value * 2);
        UnsignedCodec::encode(encoded, sink);
    }

    template<std::signed_integral T>
    static T decode(auto& source) requires BitSource<decltype(source)> {
        using U = std::make_unsigned_t<T>;
        U encoded = UnsignedCodec::template decode<U>(source);
        // Zigzag decoding
        return (encoded & 1) ? -static_cast<T>((encoded + 1) / 2) 
                              : static_cast<T>(encoded / 2);
    }
};

// ============================================================
//  VByte (Variable Byte / Varint) - Industry standard codec
//  Protocol Buffers compatible, cache-friendly, SIMD-ready
// ============================================================

struct VByte {
    template<std::unsigned_integral T, typename Sink>
    static void encode(T value, Sink& sink) requires BitSink<Sink> {
        // Each byte: 7 bits data (LSB first) + 1 continuation bit (MSB)
        // Continuation bit: 0 = more bytes follow, 1 = last byte
        do {
            uint8_t byte = value & 0x7F;
            value >>= 7;
            if (value != 0) {
                // More bytes to follow, continuation bit = 0
                for (int i = 0; i < 8; ++i) {
                    sink.write((byte >> i) & 1);
                }
            } else {
                // Last byte, continuation bit = 1
                byte |= 0x80;
                for (int i = 0; i < 8; ++i) {
                    sink.write((byte >> i) & 1);
                }
            }
        } while (value != 0);
    }

    template<std::unsigned_integral T>
    static T decode(auto& source) requires BitSource<decltype(source)> {
        T result = 0;
        int shift = 0;

        while (true) {
            // Read 8 bits
            uint8_t byte = 0;
            for (int i = 0; i < 8; ++i) {
                byte |= static_cast<uint8_t>(source.read()) << i;
            }

            // Extract data bits (lower 7 bits)
            T data = byte & 0x7F;
            result |= data << shift;

            // Check continuation bit (MSB)
            if (byte & 0x80) {
                // Last byte, we're done
                break;
            }

            shift += 7;
        }

        return result;
    }
};

// ============================================================
//  Exponential-Golomb - Parameterized family used in video coding
//  Order 0 = Elias Gamma, higher orders = flatter distribution
// ============================================================

template<size_t Order = 0>
struct ExpGolomb {
    static_assert(Order < 32, "Order must be less than 32");

    template<std::unsigned_integral T>
    static void encode(T value, auto& sink) requires BitSink<decltype(sink)> {
        // Map input: value → value + 2^Order - 1
        T mapped = value + (1u << Order) - 1;

        // Compute q and r
        T q = mapped >> Order;  // quotient: mapped / 2^Order
        T r = mapped & ((1u << Order) - 1);  // remainder: mapped % 2^Order

        // Encode q+1 in unary (length q, then a 1 bit)
        int len = msb_position(q + 1) + 1;

        // Write len-1 zeros
        for (int i = 0; i < len - 1; ++i) {
            sink.write(false);
        }

        // Write q+1 in binary (including implicit leading 1)
        for (int i = len - 1; i >= 0; --i) {
            sink.write(((q + 1) >> i) & 1);
        }

        // Write remainder in binary (Order bits)
        for (size_t i = 0; i < Order; ++i) {
            sink.write((r >> i) & 1);
        }
    }

    template<std::unsigned_integral T>
    static T decode(auto& source) requires BitSource<decltype(source)> {
        // Count leading zeros
        int zeros = 0;
        while (!source.read()) {
            ++zeros;
        }

        // Read zeros more bits to get q+1
        T q_plus_1 = 1;
        for (int i = 0; i < zeros; ++i) {
            q_plus_1 = (q_plus_1 << 1) | source.read();
        }
        T q = q_plus_1 - 1;

        // Read remainder (Order bits)
        T r = 0;
        for (size_t i = 0; i < Order; ++i) {
            r |= static_cast<T>(source.read()) << i;
        }

        // Reconstruct mapped value
        T mapped = (q << Order) | r;

        // Unmap: mapped → value
        return mapped - (1u << Order) + 1;
    }
};

// Convenience aliases for common orders
using ExpGolomb0 = ExpGolomb<0>;  // Same as Elias Gamma
using ExpGolomb1 = ExpGolomb<1>;
using ExpGolomb2 = ExpGolomb<2>;

// ============================================================
//  Elias Omega - Recursive length encoding
//  More efficient than Delta for large integers
// ============================================================

struct EliasOmega {
    template<std::unsigned_integral T>
    static void encode(T value, auto& sink) requires BitSink<decltype(sink)> {
        ++value;  // Map 0→1, 1→2, etc.

        // Build list of binary representations bottom-up
        std::vector<std::pair<T, int>> stack;

        while (value > 1) {
            int len = msb_position(value) + 1;
            stack.emplace_back(value, len);
            value = len - 1;  // Encode length-1 next
        }

        // Write components in reverse order (top-down)
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            auto [val, len] = *it;
            // Write value in binary (MSB to LSB)
            for (int i = len - 1; i >= 0; --i) {
                sink.write((val >> i) & 1);
            }
        }

        // Terminator: write a 0 bit
        sink.write(false);
    }

    template<std::unsigned_integral T>
    static T decode(auto& source) requires BitSource<decltype(source)> {
        T n = 1;  // Start with implicit 1

        while (source.peek() && source.read()) {
            // Read a 1 bit: we need to read more
            // Current n tells us how many bits to read next
            T next = 1;  // Implicit leading 1

            for (T i = 0; i < n; ++i) {
                next = (next << 1) | source.read();
            }

            n = next;
        }

        // We've either read a 0 (terminator) or reached end
        // n contains the decoded value + 1
        return n - 1;  // Map back: 1→0, 2→1, etc.
    }
};

// Convenience aliases
using SignedGamma = Signed<EliasGamma>;
using SignedDelta = Signed<EliasDelta>;
using SignedFibonacci = Signed<Fibonacci>;
using SignedVByte = Signed<VByte>;
using SignedOmega = Signed<EliasOmega>;

} // namespace pfc::codecs