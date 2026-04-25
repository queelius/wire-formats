// allocator_support.hpp - Allocator-aware types for PFC library
#pragma once

#include <memory>
#include <memory_resource>
#include <vector>
#include <scoped_allocator>

namespace pfc {

// ============================================================
// Allocator-aware BitWriter
// ============================================================

template<typename Allocator = std::allocator<uint8_t>>
class BasicBitWriter {
public:
    using allocator_type = Allocator;
    using byte_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>;
    using buffer_type = std::vector<uint8_t, byte_allocator>;

    explicit BasicBitWriter(const Allocator& alloc = Allocator())
        : buffer_(alloc), byte_(0), bit_(0) {
        buffer_.reserve(64); // Initial capacity
    }

    explicit BasicBitWriter(size_t initial_capacity, const Allocator& alloc = Allocator())
        : buffer_(alloc), byte_(0), bit_(0) {
        buffer_.reserve(initial_capacity);
    }

    void write(bool b) {
        if (b) byte_ |= 1u << bit_;
        if (++bit_ == 8) flush();
    }

    void write_bit(bool b) {
        write(b);
    }

    void write_bits(uint64_t v, int n) {
        for (int i = 0; i < n; ++i) {
            write((v >> i) & 1);
        }
    }

    void align() {
        if (bit_ > 0) flush();
    }

    const buffer_type& buffer() const { return buffer_; }
    buffer_type& buffer() { return buffer_; }

    size_t size() const {
        return buffer_.size() + (bit_ > 0 ? 1 : 0);
    }

    size_t bit_count() const {
        return buffer_.size() * 8 + bit_;
    }

    allocator_type get_allocator() const {
        return buffer_.get_allocator();
    }

private:
    void flush() {
        buffer_.push_back(byte_);
        byte_ = 0;
        bit_ = 0;
    }

    buffer_type buffer_;
    uint8_t byte_;
    int bit_;
};

// ============================================================
// Allocator-aware BitReader
// ============================================================

template<typename Allocator = std::allocator<uint8_t>>
class BasicBitReader {
public:
    using allocator_type = Allocator;

    explicit BasicBitReader(const uint8_t* data, size_t size)
        : data_(data), size_(size), pos_(0), byte_(0), bit_(8) {}

    BasicBitReader(const std::vector<uint8_t, Allocator>& buffer)
        : data_(buffer.data()), size_(buffer.size()), pos_(0), byte_(0), bit_(8) {}

    bool read() {
        if (bit_ == 8) {
            if (pos_ >= size_) return false; // EOF
            byte_ = data_[pos_++];
            bit_ = 0;
        }
        return (byte_ >> bit_++) & 1u;
    }

    bool read_bit() {
        return read();
    }

    bool peek() const {
        return pos_ < size_ || bit_ < 8;
    }

    uint64_t read_bits(int n) {
        uint64_t v = 0;
        for (int i = 0; i < n; ++i) {
            v |= uint64_t(read()) << i;
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

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
    uint8_t byte_;
    int bit_;
};

// ============================================================
// PMR (Polymorphic Memory Resource) support
// ============================================================

namespace pmr {
    using BitWriter = BasicBitWriter<std::pmr::polymorphic_allocator<uint8_t>>;
    using BitReader = BasicBitReader<std::pmr::polymorphic_allocator<uint8_t>>;
}

// ============================================================
// Allocator-aware PackedBuffer
// ============================================================

template<typename Allocator = std::allocator<uint8_t>>
class BasicPackedBuffer {
public:
    using allocator_type = Allocator;
    using buffer_type = std::vector<uint8_t, Allocator>;

    explicit BasicPackedBuffer(const Allocator& alloc = Allocator())
        : data_(alloc) {}

    template<typename T, typename Codec>
    void pack(const T& value) {
        BasicBitWriter<Allocator> writer(get_allocator());
        Codec::encode(value, writer);
        writer.align();

        // Append to our buffer
        const auto& buf = writer.buffer();
        data_.insert(data_.end(), buf.begin(), buf.end());
    }

    template<typename T, typename Codec>
    T unpack(size_t& offset) const {
        BasicBitReader<Allocator> reader(data_.data() + offset, data_.size() - offset);
        T result = Codec::template decode<T>(reader);
        offset += (reader.position() + 7) / 8; // Round up to byte boundary
        return result;
    }

    const buffer_type& data() const { return data_; }
    size_t size() const { return data_.size(); }

    allocator_type get_allocator() const {
        return data_.get_allocator();
    }

    void clear() { data_.clear(); }
    void reserve(size_t n) { data_.reserve(n); }

private:
    buffer_type data_;
};

// ============================================================
// Allocator-aware containers
// ============================================================

template<typename T, typename Codec, typename Allocator = std::allocator<uint8_t>>
class BasicPackedVector {
public:
    using value_type = T;
    using codec_type = Codec;
    using allocator_type = Allocator;
    using size_type = size_t;

    explicit BasicPackedVector(const Allocator& alloc = Allocator())
        : buffer_(alloc), offsets_(alloc) {}

    void push_back(const T& value) {
        size_t offset = buffer_.size();

        // Use pack method which properly appends to buffer
        buffer_.template pack<T, Codec>(value);
        offsets_.push_back(offset);
    }

    T operator[](size_type idx) const {
        if (idx >= size()) {
            throw std::out_of_range("BasicPackedVector: index out of range");
        }
        size_t offset = offsets_[idx];
        return buffer_.template unpack<T, Codec>(offset);
    }

    size_type size() const { return offsets_.size(); }
    bool empty() const { return offsets_.empty(); }

    void clear() {
        buffer_.clear();
        offsets_.clear();
    }

    allocator_type get_allocator() const {
        return buffer_.get_allocator();
    }

private:
    BasicPackedBuffer<Allocator> buffer_;
    std::vector<size_t, typename std::allocator_traits<Allocator>::template rebind_alloc<size_t>> offsets_;
};

// Type aliases for common use cases - renamed to avoid conflict
template<typename T, typename Codec>
using AllocPackedVector = BasicPackedVector<T, Codec, std::allocator<uint8_t>>;

namespace pmr {
    template<typename T, typename Codec>
    using PackedVector = BasicPackedVector<T, Codec, std::pmr::polymorphic_allocator<uint8_t>>;
}

} // namespace pfc