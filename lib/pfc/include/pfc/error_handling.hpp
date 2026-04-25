// error_handling.hpp - Error handling infrastructure for PFC library
#pragma once

#include <system_error>
#include <expected>
#include <string>
#include <variant>

namespace pfc {

// ============================================================
// Error codes for PFC operations
// ============================================================

enum class pfc_error {
    success = 0,
    buffer_overflow,
    buffer_underflow,
    invalid_input,
    corrupted_data,
    unsupported_codec,
    allocation_failure,
    io_error,
    checksum_mismatch,
    compression_error,
    decompression_error,
    eof_reached,
    incomplete_data,
    invalid_header,
    invalid_parameter
};

// Error category for PFC errors
class pfc_error_category : public std::error_category {
public:
    const char* name() const noexcept override {
        return "pfc";
    }

    std::string message(int ev) const override {
        switch (static_cast<pfc_error>(ev)) {
            case pfc_error::success:
                return "Success";
            case pfc_error::buffer_overflow:
                return "Buffer overflow: not enough space";
            case pfc_error::buffer_underflow:
                return "Buffer underflow: not enough data";
            case pfc_error::invalid_input:
                return "Invalid input data";
            case pfc_error::corrupted_data:
                return "Corrupted or malformed data";
            case pfc_error::unsupported_codec:
                return "Unsupported codec type";
            case pfc_error::allocation_failure:
                return "Memory allocation failed";
            case pfc_error::io_error:
                return "I/O operation failed";
            case pfc_error::checksum_mismatch:
                return "Checksum verification failed";
            case pfc_error::compression_error:
                return "Compression failed";
            case pfc_error::decompression_error:
                return "Decompression failed";
            case pfc_error::eof_reached:
                return "End of file reached";
            case pfc_error::incomplete_data:
                return "Incomplete data stream";
            case pfc_error::invalid_header:
                return "Invalid or corrupted header";
            case pfc_error::invalid_parameter:
                return "Invalid parameter";
            default:
                return "Unknown error";
        }
    }
};

// Get the singleton error category
inline const pfc_error_category& get_pfc_error_category() {
    static pfc_error_category instance;
    return instance;
}

// Make error_code from pfc_error
inline std::error_code make_error_code(pfc_error e) {
    return {static_cast<int>(e), get_pfc_error_category()};
}

} // namespace pfc

// Register pfc_error with std::error_code
namespace std {
    template<>
    struct is_error_code_enum<pfc::pfc_error> : true_type {};
}

namespace pfc {

// ============================================================
// Result type using std::expected (C++23) or fallback
// ============================================================

#if __cplusplus >= 202302L && defined(__cpp_lib_expected)
    template<typename T>
    using result = std::expected<T, std::error_code>;

    template<typename T>
    using expected = std::expected<T, std::error_code>;

    // Specialization for void
    template<>
    using result<void> = std::expected<void, std::error_code>;
#else
    // Fallback implementation for pre-C++23
    template<typename T>
    class result {
    public:
        result(T value) : data_(std::move(value)) {}
        result(std::error_code ec) : data_(ec) {}

        bool has_value() const { return std::holds_alternative<T>(data_); }
        explicit operator bool() const { return has_value(); }

        T& value() {
            if (!has_value()) throw std::system_error(error());
            return std::get<T>(data_);
        }

        const T& value() const {
            if (!has_value()) throw std::system_error(error());
            return std::get<T>(data_);
        }

        T& operator*() { return value(); }
        const T& operator*() const { return value(); }
        T* operator->() { return &value(); }
        const T* operator->() const { return &value(); }

        std::error_code error() const {
            return has_value() ? std::error_code() : std::get<std::error_code>(data_);
        }

    private:
        std::variant<T, std::error_code> data_;
    };

    template<typename T>
    using expected = result<T>;

    // Specialization for void
    template<>
    class result<void> {
    public:
        result() : has_value_(true) {}
        result(std::error_code ec) : has_value_(false), error_(ec) {}

        bool has_value() const { return has_value_; }
        explicit operator bool() const { return has_value(); }

        void value() const {
            if (!has_value()) throw std::system_error(error());
        }

        std::error_code error() const {
            return has_value_ ? std::error_code() : error_;
        }

    private:
        bool has_value_;
        std::error_code error_;
    };
#endif

// ============================================================
// Safe BitWriter with bounds checking
// ============================================================

template<typename Allocator = std::allocator<uint8_t>>
class SafeBitWriter {
public:
    using allocator_type = Allocator;
    using byte_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>;
    using buffer_type = std::vector<uint8_t, byte_allocator>;

    explicit SafeBitWriter(size_t max_size, const Allocator& alloc = Allocator())
        : buffer_(alloc), byte_(0), bit_(0), max_size_(max_size) {
        buffer_.reserve(std::min(size_t(64), max_size));
    }

    result<void> write_bit(bool b) {
        if (buffer_.size() >= max_size_ && bit_ == 0) {
            return make_error_code(pfc_error::buffer_overflow);
        }

        if (b) byte_ |= 1u << bit_;
        if (++bit_ == 8) {
            buffer_.push_back(byte_);
            byte_ = 0;
            bit_ = 0;
        }
        return {};
    }

    result<void> write_bits(uint64_t v, int n) {
        for (int i = 0; i < n; ++i) {
            if (auto res = write_bit((v >> i) & 1); !res) {
                return res;
            }
        }
        return {};
    }

    result<void> align() {
        if (bit_ > 0) {
            if (buffer_.size() >= max_size_) {
                return make_error_code(pfc_error::buffer_overflow);
            }
            buffer_.push_back(byte_);
            byte_ = 0;
            bit_ = 0;
        }
        return {};
    }

    const buffer_type& buffer() const { return buffer_; }
    size_t size() const { return buffer_.size() + (bit_ > 0 ? 1 : 0); }
    size_t bit_count() const { return buffer_.size() * 8 + bit_; }
    size_t max_size() const { return max_size_; }

    allocator_type get_allocator() const {
        return buffer_.get_allocator();
    }

private:
    buffer_type buffer_;
    uint8_t byte_;
    int bit_;
    size_t max_size_;
};

// ============================================================
// Safe BitReader with validation
// ============================================================

template<typename Allocator = std::allocator<uint8_t>>
class SafeBitReader {
public:
    using allocator_type = Allocator;

    SafeBitReader(const uint8_t* data, size_t size)
        : data_(data), size_(size), pos_(0), byte_(0), bit_(8) {
        if (!data_ && size_ > 0) {
            valid_ = false;
        }
    }

    SafeBitReader(const std::vector<uint8_t, Allocator>& buffer)
        : data_(buffer.data()), size_(buffer.size()), pos_(0), byte_(0), bit_(8) {}

    result<bool> read_bit() {
        if (!valid_) {
            return make_error_code(pfc_error::invalid_input);
        }

        if (bit_ == 8) {
            if (pos_ >= size_) {
                return make_error_code(pfc_error::eof_reached);
            }
            byte_ = data_[pos_++];
            bit_ = 0;
        }
        return (byte_ >> bit_++) & 1u;
    }

    result<uint64_t> read_bits(int n) {
        if (n > 64) {
            return make_error_code(pfc_error::invalid_parameter);
        }

        uint64_t v = 0;
        for (int i = 0; i < n; ++i) {
            auto bit_res = read_bit();
            if (!bit_res) {
                return bit_res.error();
            }
            v |= uint64_t(*bit_res) << i;
        }
        return v;
    }

    void align() {
        bit_ = 8;
    }

    bool eof() const {
        return pos_ >= size_ && bit_ == 8;
    }

    size_t position() const {
        return pos_ * 8 + bit_;
    }

    bool is_valid() const { return valid_; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
    uint8_t byte_;
    int bit_;
    bool valid_ = true;
};

// ============================================================
// Exception-safe RAII wrappers
// ============================================================

template<typename Writer>
class WriterTransaction {
public:
    explicit WriterTransaction(Writer& writer)
        : writer_(writer), checkpoint_(writer.size()), committed_(false) {}

    ~WriterTransaction() {
        if (!committed_) {
            // Rollback by truncating to checkpoint
            // Note: This requires Writer to support truncation
        }
    }

    void commit() { committed_ = true; }
    void rollback() { committed_ = false; }

private:
    Writer& writer_;
    size_t checkpoint_;
    bool committed_;
};

// ============================================================
// Validation utilities
// ============================================================

class DataValidator {
public:
    static result<void> validate_header(const uint8_t* data, size_t size) {
        if (size < 4) {
            return make_error_code(pfc_error::incomplete_data);
        }
        // Check magic number
        if (data[0] != 'P' || data[1] != 'F' || data[2] != 'C' || data[3] != '1') {
            return make_error_code(pfc_error::invalid_header);
        }
        return {};
    }

    static result<void> validate_codec_id(uint32_t codec_id) {
        // Validate codec ID is in supported range
        if (codec_id > 1000) { // Arbitrary max
            return make_error_code(pfc_error::unsupported_codec);
        }
        return {};
    }
};

} // namespace pfc