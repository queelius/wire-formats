#pragma once
// arithmetic_coding.hpp - Arithmetic and range coding for PFC library
// Optimal entropy coding with adaptive probability models
// "Information is the resolution of uncertainty" - Claude Shannon

#include "core.hpp"
#include "error_handling.hpp"
#include <vector>
#include <array>
#include <algorithm>
#include <numeric>
#include <cstdint>
#include <concepts>
#include <type_traits>

namespace pfc {

// ============================================================
//  Probability Models - The foundation of arithmetic coding
// ============================================================

// Concept for probability models
template<typename M>
concept ProbabilityModel = requires(M& model, std::size_t symbol) {
    { model.symbol_count() } -> std::convertible_to<std::size_t>;
    { model.cumulative_frequency(symbol) } -> std::convertible_to<uint32_t>;
    { model.frequency(symbol) } -> std::convertible_to<uint32_t>;
    { model.total_frequency() } -> std::convertible_to<uint32_t>;
    { model.find_symbol(uint32_t{}) } -> std::convertible_to<std::size_t>;
    { model.update(symbol) } -> std::same_as<void>;
};

// Static probability model with fixed frequencies
template<std::size_t Symbols>
class static_probability_model {
    std::array<uint32_t, Symbols> frequencies_;
    std::array<uint32_t, Symbols + 1> cumulative_;
    uint32_t total_;

public:
    static_probability_model() : frequencies_{}, cumulative_{}, total_(0) {
        reset_uniform();
    }

    explicit static_probability_model(const std::array<uint32_t, Symbols>& freqs)
        : frequencies_(freqs) {
        build_cumulative();
    }

    void reset_uniform() {
        std::fill(frequencies_.begin(), frequencies_.end(), 1);
        build_cumulative();
    }

    void set_frequencies(const std::array<uint32_t, Symbols>& freqs) {
        frequencies_ = freqs;
        build_cumulative();
    }

    [[nodiscard]] std::size_t symbol_count() const noexcept { return Symbols; }

    [[nodiscard]] uint32_t frequency(std::size_t symbol) const noexcept {
        return frequencies_[symbol];
    }

    [[nodiscard]] uint32_t cumulative_frequency(std::size_t symbol) const noexcept {
        return cumulative_[symbol];
    }

    [[nodiscard]] uint32_t total_frequency() const noexcept {
        return total_;
    }

    [[nodiscard]] std::size_t find_symbol(uint32_t value) const noexcept {
        // Binary search for efficiency
        auto it = std::upper_bound(cumulative_.begin(), cumulative_.end() - 1, value);
        return std::distance(cumulative_.begin(), it) - 1;
    }

    void update(std::size_t) {
        // Static model doesn't update
    }

private:
    void build_cumulative() {
        cumulative_[0] = 0;
        for (std::size_t i = 0; i < Symbols; ++i) {
            cumulative_[i + 1] = cumulative_[i] + frequencies_[i];
        }
        total_ = cumulative_[Symbols];
    }
};

// Adaptive probability model that learns from data
template<std::size_t Symbols>
class adaptive_probability_model {
    std::array<uint32_t, Symbols> frequencies_;
    std::array<uint32_t, Symbols + 1> cumulative_;
    uint32_t total_;
    static constexpr uint32_t max_frequency = 16383;  // Prevent overflow
    static constexpr uint32_t rescale_threshold = max_frequency * Symbols;

public:
    adaptive_probability_model() : frequencies_{}, cumulative_{}, total_(0) {
        reset();
    }

    void reset() {
        std::fill(frequencies_.begin(), frequencies_.end(), 1);
        build_cumulative();
    }

    [[nodiscard]] std::size_t symbol_count() const noexcept { return Symbols; }

    [[nodiscard]] uint32_t frequency(std::size_t symbol) const noexcept {
        return frequencies_[symbol];
    }

    [[nodiscard]] uint32_t cumulative_frequency(std::size_t symbol) const noexcept {
        return cumulative_[symbol];
    }

    [[nodiscard]] uint32_t total_frequency() const noexcept {
        return total_;
    }

    [[nodiscard]] std::size_t find_symbol(uint32_t value) const noexcept {
        // Linear search for small alphabets, binary search for large ones
        if constexpr (Symbols <= 16) {
            for (std::size_t i = 0; i < Symbols; ++i) {
                if (value < cumulative_[i + 1]) return i;
            }
            return Symbols - 1;
        } else {
            auto it = std::upper_bound(cumulative_.begin(), cumulative_.end() - 1, value);
            return std::distance(cumulative_.begin(), it) - 1;
        }
    }

    void update(std::size_t symbol) {
        frequencies_[symbol]++;

        // Update cumulative frequencies from symbol onward
        for (std::size_t i = symbol + 1; i <= Symbols; ++i) {
            cumulative_[i]++;
        }
        total_++;

        // Rescale if necessary to prevent overflow
        if (total_ >= rescale_threshold) {
            rescale();
        }
    }

private:
    void build_cumulative() {
        cumulative_[0] = 0;
        for (std::size_t i = 0; i < Symbols; ++i) {
            cumulative_[i + 1] = cumulative_[i] + frequencies_[i];
        }
        total_ = cumulative_[Symbols];
    }

    void rescale() {
        // Halve all frequencies (keeping minimum of 1)
        for (auto& freq : frequencies_) {
            freq = (freq + 1) / 2;
        }
        build_cumulative();
    }
};

// Context model for order-N modeling
template<std::size_t Symbols, std::size_t Order>
class context_model {
    static constexpr std::size_t context_count = 1u << (Order * 8);
    std::vector<adaptive_probability_model<Symbols>> models_;
    std::size_t current_context_ = 0;

public:
    context_model() : models_(context_count) {}

    void reset() {
        for (auto& model : models_) {
            model.reset();
        }
        current_context_ = 0;
    }

    void set_context(const uint8_t* context_bytes) {
        current_context_ = 0;
        for (std::size_t i = 0; i < Order; ++i) {
            current_context_ = (current_context_ << 8) | context_bytes[i];
        }
        current_context_ %= context_count;
    }

    [[nodiscard]] auto& current_model() { return models_[current_context_]; }
    [[nodiscard]] const auto& current_model() const { return models_[current_context_]; }
};

// ============================================================
//  Arithmetic Encoder - The heart of optimal compression
// ============================================================

class arithmetic_encoder {
    static constexpr uint32_t top_value = 0x7FFFFFFF;  // 31 bits
    static constexpr uint32_t first_quarter = top_value / 4 + 1;
    static constexpr uint32_t half = 2 * first_quarter;
    static constexpr uint32_t third_quarter = 3 * first_quarter;

    BitWriter* sink_;
    uint32_t low_ = 0;
    uint32_t high_ = top_value;
    uint32_t bits_to_follow_ = 0;

    void output_bit_plus_follow(bool bit) {
        sink_->write(bit);
        while (bits_to_follow_ > 0) {
            sink_->write(!bit);
            bits_to_follow_--;
        }
    }

public:
    explicit arithmetic_encoder(BitWriter& sink) : sink_(&sink) {}

    template<ProbabilityModel Model>
    void encode_symbol(std::size_t symbol, Model& model) {
        uint32_t range = high_ - low_ + 1;
        uint32_t total = model.total_frequency();

        // Ensure we have a valid total frequency
        if (total == 0) {
            return;  // Can't encode with empty model
        }

        uint32_t cum_freq = model.cumulative_frequency(symbol);
        uint32_t freq = model.frequency(symbol);

        // Update range based on symbol probability
        // Use uint64_t to prevent overflow in intermediate calculations
        uint64_t r = range;
        uint32_t new_high = low_ + static_cast<uint32_t>((r * (cum_freq + freq)) / total) - 1;
        uint32_t new_low = low_ + static_cast<uint32_t>((r * cum_freq) / total);

        // Ensure high_ > low_ (prevent range collapse)
        if (new_high <= new_low) {
            new_high = new_low + 1;
        }

        high_ = new_high;
        low_ = new_low;

        // Normalize and output bits
        while (true) {
            if (high_ < half) {
                output_bit_plus_follow(false);
                low_ *= 2;
                high_ = high_ * 2 + 1;
            } else if (low_ >= half) {
                output_bit_plus_follow(true);
                low_ = (low_ - half) * 2;
                high_ = (high_ - half) * 2 + 1;
            } else if (low_ >= first_quarter && high_ < third_quarter) {
                bits_to_follow_++;
                low_ = (low_ - first_quarter) * 2;
                high_ = (high_ - first_quarter) * 2 + 1;
            } else {
                break;
            }
        }

        model.update(symbol);
    }

    void finish() {
        // Output final bits
        bits_to_follow_++;
        if (low_ < first_quarter) {
            output_bit_plus_follow(false);
        } else {
            output_bit_plus_follow(true);
        }
    }
};

// ============================================================
//  Arithmetic Decoder - Reversing the entropy coding
// ============================================================

class arithmetic_decoder {
    static constexpr uint32_t top_value = 0x7FFFFFFF;
    static constexpr uint32_t first_quarter = top_value / 4 + 1;
    static constexpr uint32_t half = 2 * first_quarter;
    static constexpr uint32_t third_quarter = 3 * first_quarter;

    BitReader* source_;
    uint32_t low_ = 0;
    uint32_t high_ = top_value;
    uint32_t value_ = 0;

    bool read_bit() {
        return source_->read();
    }

public:
    explicit arithmetic_decoder(BitReader& source) : source_(&source) {
        // Initialize decoder with first 31 bits
        for (int i = 0; i < 31; ++i) {
            value_ = (value_ << 1) | (read_bit() ? 1 : 0);
        }
    }

    template<ProbabilityModel Model>
    std::size_t decode_symbol(Model& model) {
        uint32_t range = high_ - low_ + 1;
        uint32_t total = model.total_frequency();

        // Ensure we have a valid total frequency
        if (total == 0) {
            return 0;  // Can't decode with empty model
        }

        // Ensure range is non-zero to prevent division by zero
        if (range == 0) {
            return 0;  // Invalid state
        }

        // Use uint64_t to prevent overflow in intermediate calculations
        // The encoder computes: new_low = low_ + (range * cum_freq) / total
        // To invert: cum_freq = ((value_ - low_) * total) / range
        uint64_t v = value_ - low_;
        uint32_t scaled_value = static_cast<uint32_t>((v * total) / range);

        // Find symbol
        std::size_t symbol = model.find_symbol(scaled_value);

        // Update range
        uint32_t cum_freq = model.cumulative_frequency(symbol);
        uint32_t freq = model.frequency(symbol);

        uint64_t r = range;
        uint32_t new_high = low_ + static_cast<uint32_t>((r * (cum_freq + freq)) / total) - 1;
        uint32_t new_low = low_ + static_cast<uint32_t>((r * cum_freq) / total);

        // Ensure high_ > low_ (prevent range collapse)
        if (new_high <= new_low) {
            new_high = new_low + 1;
        }

        high_ = new_high;
        low_ = new_low;

        // Normalize and read new bits
        while (true) {
            if (high_ < half) {
                low_ *= 2;
                high_ = high_ * 2 + 1;
                value_ = (value_ * 2) | (read_bit() ? 1 : 0);
            } else if (low_ >= half) {
                low_ = (low_ - half) * 2;
                high_ = (high_ - half) * 2 + 1;
                value_ = ((value_ - half) * 2) | (read_bit() ? 1 : 0);
            } else if (low_ >= first_quarter && high_ < third_quarter) {
                low_ = (low_ - first_quarter) * 2;
                high_ = (high_ - first_quarter) * 2 + 1;
                value_ = ((value_ - first_quarter) * 2) | (read_bit() ? 1 : 0);
            } else {
                break;
            }
        }

        model.update(symbol);
        return symbol;
    }
};

// ============================================================
//  Range Coder - Variant of arithmetic coding using 32-bit precision
//  Based on Dmitry Subbotin's carryless range coder design
// ============================================================

class range_encoder {
    // 32-bit range coder constants (well-tested parameters)
    static constexpr uint32_t top_value = 1u << 24;     // Normalize when range < 2^24
    static constexpr uint32_t bottom_value = 1u << 16;  // Bottom threshold for scaling

    BitWriter* sink_;
    uint32_t low_ = 0;
    uint32_t range_ = 0xFFFFFFFF;  // Start with full 32-bit range

    void output_byte(uint8_t byte) {
        sink_->write_bits(byte, 8);
    }

    void normalize() {
        while ((low_ ^ (low_ + range_)) < top_value || range_ < bottom_value) {
            if ((low_ ^ (low_ + range_)) >= top_value) {
                // Range straddles a power of 256 boundary
                range_ = (uint32_t)(-int32_t(low_)) & (bottom_value - 1);
            }
            output_byte(low_ >> 24);
            low_ <<= 8;
            range_ <<= 8;
        }
    }

public:
    explicit range_encoder(BitWriter& sink) : sink_(&sink) {}

    template<ProbabilityModel Model>
    void encode_symbol(std::size_t symbol, Model& model) {
        uint32_t total = model.total_frequency();
        if (total == 0) return;

        uint32_t cum_freq = model.cumulative_frequency(symbol);
        uint32_t freq = model.frequency(symbol);

        // Scale the range and update
        uint32_t tmp = range_ / total;
        low_ += cum_freq * tmp;
        range_ = freq * tmp;

        normalize();
        model.update(symbol);
    }

    void encode_bit(bool bit, uint32_t prob_zero) {
        static constexpr uint32_t prob_scale = 0x10000;

        uint32_t tmp = range_ / prob_scale;
        if (!bit) {
            range_ = prob_zero * tmp;
        } else {
            low_ += prob_zero * tmp;
            range_ = (prob_scale - prob_zero) * tmp;
        }

        normalize();
    }

    void finish() {
        // Output final 4 bytes (entire 32-bit low value)
        for (int i = 0; i < 4; ++i) {
            output_byte(low_ >> 24);
            low_ <<= 8;
        }
    }
};

class range_decoder {
    static constexpr uint32_t top_value = 1u << 24;
    static constexpr uint32_t bottom_value = 1u << 16;

    BitReader* source_;
    uint32_t low_ = 0;
    uint32_t code_ = 0;
    uint32_t range_ = 0xFFFFFFFF;

    uint8_t input_byte() {
        return static_cast<uint8_t>(source_->read_bits(8));
    }

    void normalize() {
        while ((low_ ^ (low_ + range_)) < top_value || range_ < bottom_value) {
            if ((low_ ^ (low_ + range_)) >= top_value) {
                range_ = (uint32_t)(-int32_t(low_)) & (bottom_value - 1);
            }
            code_ = (code_ << 8) | input_byte();
            low_ <<= 8;
            range_ <<= 8;
        }
    }

public:
    explicit range_decoder(BitReader& source) : source_(&source) {
        // Read initial 4 bytes into code
        for (int i = 0; i < 4; ++i) {
            code_ = (code_ << 8) | input_byte();
        }
    }

    template<ProbabilityModel Model>
    std::size_t decode_symbol(Model& model) {
        uint32_t total = model.total_frequency();
        if (total == 0) return 0;

        uint32_t tmp = range_ / total;

        // Find the symbol: compute count = (code - low) / tmp
        // This is the key formula that must be consistent with encoding
        uint32_t count = (code_ - low_) / tmp;

        // Clamp count to valid range
        if (count >= total) count = total - 1;

        // Find symbol from cumulative frequency
        std::size_t symbol = model.find_symbol(count);

        uint32_t cum_freq = model.cumulative_frequency(symbol);
        uint32_t freq = model.frequency(symbol);

        // Update range - exactly matches encoder
        low_ += cum_freq * tmp;
        range_ = freq * tmp;

        normalize();
        model.update(symbol);
        return symbol;
    }

    bool decode_bit(uint32_t prob_zero) {
        static constexpr uint32_t prob_scale = 0x10000;

        uint32_t tmp = range_ / prob_scale;
        uint32_t threshold = low_ + prob_zero * tmp;

        bool bit = (code_ >= threshold);

        if (!bit) {
            range_ = prob_zero * tmp;
        } else {
            low_ = threshold;
            range_ = (prob_scale - prob_zero) * tmp;
        }

        normalize();
        return bit;
    }
};

// ============================================================
//  High-Level API - Easy to use interfaces
// ============================================================

// Compress bytes using arithmetic coding with adaptive model
template<typename InputIt, typename OutputIt>
result<std::size_t> arithmetic_compress(InputIt first, InputIt last, OutputIt output) {
    try {
        std::vector<uint8_t> buffer;
        buffer.reserve(std::distance(first, last) + 1024);

        BitWriter writer(buffer.data());
        arithmetic_encoder encoder(writer);
        adaptive_probability_model<256> model;

        // Encode each byte
        for (auto it = first; it != last; ++it) {
            encoder.encode_symbol(static_cast<uint8_t>(*it), model);
        }

        encoder.finish();
        writer.align();

        // Copy to output
        std::copy(buffer.begin(), buffer.begin() + writer.bytes_written(), output);

        return writer.bytes_written();
    } catch (const std::exception&) {
        return make_error_code(pfc_error::compression_error);
    }
}

// Decompress bytes using arithmetic coding with adaptive model
template<typename InputIt, typename OutputIt>
result<std::size_t> arithmetic_decompress(InputIt first, InputIt last, OutputIt output, std::size_t count) {
    try {
        std::vector<uint8_t> input_buffer(first, last);
        BitReader reader(input_buffer.data(), input_buffer.size());
        arithmetic_decoder decoder(reader);
        adaptive_probability_model<256> model;

        // Decode bytes
        for (std::size_t i = 0; i < count; ++i) {
            auto symbol = decoder.decode_symbol(model);
            *output++ = static_cast<uint8_t>(symbol);
        }

        return count;
    } catch (const std::exception&) {
        return make_error_code(pfc_error::decompression_error);
    }
}

// Range coding variants
template<typename InputIt, typename OutputIt>
result<std::size_t> range_compress(InputIt first, InputIt last, OutputIt output) {
    try {
        std::vector<uint8_t> buffer;
        buffer.reserve(std::distance(first, last) + 1024);

        BitWriter writer(buffer.data());
        range_encoder encoder(writer);
        adaptive_probability_model<256> model;

        for (auto it = first; it != last; ++it) {
            encoder.encode_symbol(static_cast<uint8_t>(*it), model);
        }

        encoder.finish();
        writer.align();

        std::copy(buffer.begin(), buffer.begin() + writer.bytes_written(), output);

        return writer.bytes_written();
    } catch (const std::exception&) {
        return make_error_code(pfc_error::compression_error);
    }
}

template<typename InputIt, typename OutputIt>
result<std::size_t> range_decompress(InputIt first, InputIt last, OutputIt output, std::size_t count) {
    try {
        std::vector<uint8_t> input_buffer(first, last);
        BitReader reader(input_buffer.data(), input_buffer.size());
        range_decoder decoder(reader);
        adaptive_probability_model<256> model;

        for (std::size_t i = 0; i < count; ++i) {
            auto symbol = decoder.decode_symbol(model);
            *output++ = static_cast<uint8_t>(symbol);
        }

        return count;
    } catch (const std::exception&) {
        return make_error_code(pfc_error::decompression_error);
    }
}

// ============================================================
//  Codec Interface - Integration with PFC codec system
// ============================================================

// Arithmetic codec for single values (using static model)
template<typename T, std::size_t Symbols = 256>
struct ArithmeticCodec {
    using value_type = T;

    template<BitSink Sink>
    static void encode(const T& value, Sink& sink) {
        static_probability_model<Symbols> model;
        arithmetic_encoder encoder(sink);

        // For simple types, encode bytes
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            encoder.encode_symbol(bytes[i], model);
        }

        encoder.finish();
    }

    template<BitSource Source>
    static T decode(Source& source) {
        static_probability_model<Symbols> model;
        arithmetic_decoder decoder(source);

        T result;
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&result);
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            bytes[i] = static_cast<uint8_t>(decoder.decode_symbol(model));
        }

        return result;
    }
};

// Range codec variant
template<typename T, std::size_t Symbols = 256>
struct RangeCodec {
    using value_type = T;

    template<BitSink Sink>
    static void encode(const T& value, Sink& sink) {
        adaptive_probability_model<Symbols> model;
        range_encoder encoder(sink);

        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            encoder.encode_symbol(bytes[i], model);
        }

        encoder.finish();
    }

    template<BitSource Source>
    static T decode(Source& source) {
        adaptive_probability_model<Symbols> model;
        range_decoder decoder(source);

        T result;
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&result);
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            bytes[i] = static_cast<uint8_t>(decoder.decode_symbol(model));
        }

        return result;
    }
};

} // namespace pfc