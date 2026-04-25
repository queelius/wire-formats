#pragma once
// stream_io.hpp - Stream I/O integration for PFC library
// Seamless integration with standard C++ I/O streams
// "Streams are the veins through which data flows" - Unix Philosophy

#include "core.hpp"
#include "error_handling.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>

namespace pfc {

// ============================================================
//  Stream Buffer Adaptors - RAII and zero-copy where possible
// ============================================================

// Forward declarations
class stream_bit_writer;
class stream_bit_reader;

// Custom streambuf for bit-oriented I/O
class bit_streambuf : public std::streambuf {
protected:
    static constexpr std::size_t default_buffer_size = 8192;
    std::vector<char> buffer_;
    std::size_t bit_pos_ = 0;
    uint8_t current_byte_ = 0;

public:
    explicit bit_streambuf(std::size_t buffer_size = default_buffer_size)
        : buffer_(buffer_size) {
        setp(buffer_.data(), buffer_.data() + buffer_.size() - 1);
    }

    // Flush any pending bits
    void flush_bits() {
        if (bit_pos_ > 0) {
            *pptr() = static_cast<char>(current_byte_);
            pbump(1);
            bit_pos_ = 0;
            current_byte_ = 0;
        }
    }

    // Get/set bit position within current byte
    std::size_t bit_position() const noexcept { return bit_pos_; }
    void reset_bit_position() noexcept { bit_pos_ = 0; current_byte_ = 0; }
};

// Output bit streambuf - writes bits to an underlying ostream
class output_bit_streambuf : public bit_streambuf {
    std::ostream* stream_;

protected:
    int overflow(int ch) override {
        if (stream_ && pptr() == epptr()) {
            flush_bits();
            stream_->write(pbase(), pptr() - pbase());
            if (!stream_->good()) return traits_type::eof();
            setp(buffer_.data(), buffer_.data() + buffer_.size() - 1);
        }
        if (ch != traits_type::eof()) {
            *pptr() = static_cast<char>(ch);
            pbump(1);
        }
        return ch;
    }

    int sync() override {
        flush_bits();
        if (stream_ && pptr() > pbase()) {
            stream_->write(pbase(), pptr() - pbase());
            if (!stream_->good()) return -1;
            setp(buffer_.data(), buffer_.data() + buffer_.size() - 1);
        }
        if (stream_) stream_->flush();
        return 0;
    }

public:
    explicit output_bit_streambuf(std::ostream& os, std::size_t buffer_size = default_buffer_size)
        : bit_streambuf(buffer_size), stream_(&os) {}

    ~output_bit_streambuf() {
        sync();
    }
};

// Input bit streambuf - reads bits from an underlying istream
class input_bit_streambuf : public bit_streambuf {
    std::istream* stream_;

protected:
    int underflow() override {
        if (stream_ && gptr() == egptr()) {
            stream_->read(buffer_.data(), buffer_.size());
            auto bytes_read = stream_->gcount();
            if (bytes_read == 0) return traits_type::eof();
            setg(buffer_.data(), buffer_.data(), buffer_.data() + bytes_read);
        }
        return gptr() < egptr() ? traits_type::to_int_type(*gptr()) : traits_type::eof();
    }

public:
    explicit input_bit_streambuf(std::istream& is, std::size_t buffer_size = default_buffer_size)
        : bit_streambuf(buffer_size), stream_(&is) {
        setg(buffer_.data(), buffer_.data(), buffer_.data());
    }
};

// ============================================================
//  Stream Bit Writer - BitSink that writes to std::ostream
// ============================================================

class stream_bit_writer {
    std::unique_ptr<output_bit_streambuf> buf_;
    std::ostream* stream_;
    uint8_t byte_ = 0;
    uint8_t bit_pos_ = 0;
    std::size_t bytes_written_ = 0;

public:
    explicit stream_bit_writer(std::ostream& os, std::size_t buffer_size = 8192)
        : buf_(std::make_unique<output_bit_streambuf>(os, buffer_size))
        , stream_(&os) {}

    stream_bit_writer(stream_bit_writer&&) = default;
    stream_bit_writer& operator=(stream_bit_writer&&) = default;

    // BitSink interface
    void write(bool bit) {
        byte_ |= (bit ? 1u : 0u) << bit_pos_;
        if (++bit_pos_ == 8) {
            stream_->put(static_cast<char>(byte_));
            byte_ = 0;
            bit_pos_ = 0;
            ++bytes_written_;
        }
    }

    // Write multiple bits at once (LSB first)
    void write_bits(uint64_t bits, std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            write((bits >> i) & 1);
        }
    }

    // Write bytes directly (bypassing bit-level operations)
    void write_bytes(const uint8_t* data, std::size_t size) {
        align();  // Ensure byte alignment first
        stream_->write(reinterpret_cast<const char*>(data), size);
        bytes_written_ += size;
    }

    // Align to byte boundary
    void align() {
        if (bit_pos_ > 0) {
            stream_->put(static_cast<char>(byte_));
            byte_ = 0;
            bit_pos_ = 0;
            ++bytes_written_;
        }
    }

    // Flush the underlying stream
    void flush() {
        align();
        stream_->flush();
    }

    // Get statistics
    [[nodiscard]] std::size_t bytes_written() const noexcept {
        return bytes_written_ + (bit_pos_ > 0 ? 1 : 0);
    }

    [[nodiscard]] bool good() const noexcept { return stream_->good(); }
    [[nodiscard]] bool fail() const noexcept { return stream_->fail(); }
    [[nodiscard]] bool bad() const noexcept { return stream_->bad(); }
    [[nodiscard]] bool eof() const noexcept { return stream_->eof(); }
    [[nodiscard]] explicit operator bool() const noexcept { return good(); }

    ~stream_bit_writer() {
        if (stream_ && stream_->good()) {
            flush();
        }
    }
};

// ============================================================
//  Stream Bit Reader - BitSource that reads from std::istream
// ============================================================

class stream_bit_reader {
    std::unique_ptr<input_bit_streambuf> buf_;
    std::istream* stream_;
    uint8_t byte_ = 0;
    uint8_t bit_pos_ = 8;  // Start at 8 to force initial read
    std::size_t bytes_read_ = 0;
    bool eof_reached_ = false;

public:
    explicit stream_bit_reader(std::istream& is, std::size_t buffer_size = 8192)
        : buf_(std::make_unique<input_bit_streambuf>(is, buffer_size))
        , stream_(&is) {}

    stream_bit_reader(stream_bit_reader&&) = default;
    stream_bit_reader& operator=(stream_bit_reader&&) = default;

    // BitSource interface
    [[nodiscard]] bool peek() const noexcept {
        return !eof_reached_ && (bit_pos_ < 8 || stream_->good());
    }

    bool read() {
        if (bit_pos_ == 8) {
            if (!stream_->good()) {
                eof_reached_ = true;
                return false;
            }
            int ch = stream_->get();
            if (ch == std::char_traits<char>::eof()) {
                eof_reached_ = true;
                return false;
            }
            byte_ = static_cast<uint8_t>(ch);
            bit_pos_ = 0;
            ++bytes_read_;
        }
        return (byte_ >> bit_pos_++) & 1;
    }

    // Read multiple bits at once (LSB first)
    [[nodiscard]] uint64_t read_bits(std::size_t count) {
        uint64_t result = 0;
        for (std::size_t i = 0; i < count; ++i) {
            result |= static_cast<uint64_t>(read()) << i;
        }
        return result;
    }

    // Read bytes directly (bypassing bit-level operations)
    std::size_t read_bytes(uint8_t* data, std::size_t size) {
        align();  // Ensure byte alignment first
        stream_->read(reinterpret_cast<char*>(data), size);
        auto bytes = stream_->gcount();
        bytes_read_ += bytes;
        return bytes;
    }

    // Skip to next byte boundary
    void align() {
        bit_pos_ = 8;
    }

    // Get statistics
    [[nodiscard]] std::size_t bytes_read() const noexcept {
        return bytes_read_;
    }

    [[nodiscard]] bool good() const noexcept { return stream_->good() && !eof_reached_; }
    [[nodiscard]] bool fail() const noexcept { return stream_->fail(); }
    [[nodiscard]] bool bad() const noexcept { return stream_->bad(); }
    [[nodiscard]] bool eof() const noexcept { return eof_reached_ || stream_->eof(); }
    [[nodiscard]] explicit operator bool() const noexcept { return good(); }
};

// ============================================================
//  Stream Manipulators - STL-style I/O customization
// ============================================================

// Manipulator for setting compression codec
template<typename Codec>
struct codec_manip {
    explicit codec_manip() = default;
};

template<typename Codec>
codec_manip<Codec> with_codec() {
    return codec_manip<Codec>{};
}

// Manipulator for buffer size
struct buffer_size_manip {
    std::size_t size;
    explicit buffer_size_manip(std::size_t s) : size(s) {}
};

inline buffer_size_manip buffer_size(std::size_t size) {
    return buffer_size_manip{size};
}

// ============================================================
//  File I/O Convenience Functions
// ============================================================

// Write compressed data to file
template<typename T, typename Codec = codecs::EliasGamma, typename Allocator = std::allocator<T>>
result<std::size_t> write_compressed_file(const std::string& filename,
                                          const std::vector<T, Allocator>& data,
                                          std::ios::openmode mode = std::ios::binary) {
    std::ofstream file(filename, mode | std::ios::binary);
    if (!file) {
        return make_error_code(pfc_error::io_error);
    }

    try {
        stream_bit_writer writer(file);

        // Write header with size
        Codec::encode(static_cast<uint64_t>(data.size()), writer);

        // Write data
        for (const auto& item : data) {
            Codec::encode(item, writer);
        }

        writer.flush();
        std::size_t bytes = writer.bytes_written();

        if (!writer.good()) {
            return make_error_code(pfc_error::io_error);
        }

        return bytes;
    } catch (const std::exception&) {
        return make_error_code(pfc_error::compression_error);
    }
}

// Read compressed data from file
template<typename T, typename Codec = codecs::EliasGamma, typename Allocator = std::allocator<T>>
result<std::vector<T, Allocator>> read_compressed_file(const std::string& filename,
                                                       std::ios::openmode mode = std::ios::binary) {
    std::ifstream file(filename, mode | std::ios::binary);
    if (!file) {
        return make_error_code(pfc_error::io_error);
    }

    try {
        stream_bit_reader reader(file);

        // Read header with size
        auto size = Codec::template decode<uint64_t>(reader);

        // Read data
        std::vector<T, Allocator> data;
        data.reserve(size);

        for (uint64_t i = 0; i < size; ++i) {
            data.push_back(Codec::template decode<T>(reader));
        }

        if (reader.fail() || reader.bad()) {
            return make_error_code(pfc_error::corrupted_data);
        }

        return data;
    } catch (const std::exception&) {
        return make_error_code(pfc_error::decompression_error);
    }
}

// ============================================================
//  Memory Stream Support
// ============================================================

// Compress to memory stream
template<typename T, typename Codec = codecs::EliasGamma>
result<std::string> compress_to_string(const std::vector<T>& data) {
    std::ostringstream oss(std::ios::binary);

    try {
        stream_bit_writer writer(oss);

        Codec::encode(static_cast<uint64_t>(data.size()), writer);
        for (const auto& item : data) {
            Codec::encode(item, writer);
        }

        writer.flush();

        if (!writer.good()) {
            return make_error_code(pfc_error::compression_error);
        }

        return oss.str();
    } catch (const std::exception&) {
        return make_error_code(pfc_error::compression_error);
    }
}

// Decompress from memory stream
template<typename T, typename Codec = codecs::EliasGamma>
result<std::vector<T>> decompress_from_string(const std::string& compressed) {
    std::istringstream iss(compressed, std::ios::binary);

    try {
        stream_bit_reader reader(iss);

        auto size = Codec::template decode<uint64_t>(reader);

        std::vector<T> data;
        data.reserve(size);

        for (uint64_t i = 0; i < size; ++i) {
            data.push_back(Codec::template decode<T>(reader));
        }

        if (reader.fail() || reader.bad()) {
            return make_error_code(pfc_error::corrupted_data);
        }

        return data;
    } catch (const std::exception&) {
        return make_error_code(pfc_error::decompression_error);
    }
}

// ============================================================
//  Stream Operators for Packed Types
// ============================================================

// Output operator for packed types
template<typename T, typename Codec>
std::ostream& operator<<(std::ostream& os, const Packed<T, Codec>& packed) {
    stream_bit_writer writer(os);
    Codec::encode(packed.value(), writer);
    writer.flush();
    return os;
}

// Input operator for packed types
template<typename T, typename Codec>
std::istream& operator>>(std::istream& is, Packed<T, Codec>& packed) {
    stream_bit_reader reader(is);
    packed = Packed<T, Codec>{Codec::template decode<T>(reader)};
    return is;
}

// ============================================================
//  Async I/O Support (future extension point)
// ============================================================

// Placeholder for async I/O support
// This would integrate with std::async, std::future, or coroutines
template<typename T, typename Codec = codecs::EliasGamma>
class async_stream_writer {
    // Implementation would use async I/O mechanisms
    // Currently a placeholder for future extension
};

template<typename T, typename Codec = codecs::EliasGamma>
class async_stream_reader {
    // Implementation would use async I/O mechanisms
    // Currently a placeholder for future extension
};

} // namespace pfc