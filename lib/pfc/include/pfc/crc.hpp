// crc.hpp - CRC checksum support for data integrity
#pragma once

#include <cstdint>
#include <array>
#include <span>

namespace pfc {
namespace integrity {

// ============================================================
// CRC-32 Implementation (IEEE 802.3)
// ============================================================

class CRC32 {
public:
    static constexpr uint32_t polynomial = 0xEDB88320; // Reversed polynomial
    static constexpr uint32_t initial_value = 0xFFFFFFFF;
    static constexpr uint32_t final_xor = 0xFFFFFFFF;

    CRC32() : crc_(initial_value) {
        if (!table_initialized) {
            init_table();
        }
    }

    void update(uint8_t byte) {
        crc_ = (crc_ >> 8) ^ table_[(crc_ ^ byte) & 0xFF];
    }

    void update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            update(data[i]);
        }
    }

    void update(std::span<const uint8_t> data) {
        update(data.data(), data.size());
    }

    uint32_t finalize() const {
        return crc_ ^ final_xor;
    }

    void reset() {
        crc_ = initial_value;
    }

    // Static methods for convenience
    static uint32_t calculate(const uint8_t* data, size_t length) {
        CRC32 crc;
        crc.update(data, length);
        return crc.finalize();
    }

    static uint32_t calculate(std::span<const uint8_t> data) {
        return calculate(data.data(), data.size());
    }

private:
    uint32_t crc_;
    static std::array<uint32_t, 256> table_;
    static bool table_initialized;

    static void init_table() {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c >> 1) ^ ((c & 1) ? polynomial : 0);
            }
            table_[i] = c;
        }
        table_initialized = true;
    }
};

// Static member definitions
inline std::array<uint32_t, 256> CRC32::table_{};
inline bool CRC32::table_initialized = false;

// ============================================================
// CRC-64 Implementation (ECMA-182)
// ============================================================

class CRC64 {
public:
    static constexpr uint64_t polynomial = 0xC96C5795D7870F42ULL; // ECMA-182
    static constexpr uint64_t initial_value = 0xFFFFFFFFFFFFFFFFULL;
    static constexpr uint64_t final_xor = 0xFFFFFFFFFFFFFFFFULL;

    CRC64() : crc_(initial_value) {
        if (!table_initialized) {
            init_table();
        }
    }

    void update(uint8_t byte) {
        crc_ = (crc_ >> 8) ^ table_[(crc_ ^ byte) & 0xFF];
    }

    void update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            update(data[i]);
        }
    }

    void update(std::span<const uint8_t> data) {
        update(data.data(), data.size());
    }

    uint64_t finalize() const {
        return crc_ ^ final_xor;
    }

    void reset() {
        crc_ = initial_value;
    }

    static uint64_t calculate(const uint8_t* data, size_t length) {
        CRC64 crc;
        crc.update(data, length);
        return crc.finalize();
    }

    static uint64_t calculate(std::span<const uint8_t> data) {
        return calculate(data.data(), data.size());
    }

private:
    uint64_t crc_;
    static std::array<uint64_t, 256> table_;
    static bool table_initialized;

    static void init_table() {
        for (uint64_t i = 0; i < 256; ++i) {
            uint64_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c >> 1) ^ ((c & 1) ? polynomial : 0);
            }
            table_[i] = c;
        }
        table_initialized = true;
    }
};

// Static member definitions
inline std::array<uint64_t, 256> CRC64::table_{};
inline bool CRC64::table_initialized = false;

// ============================================================
// CRC-16 Implementation (IBM)
// ============================================================

class CRC16 {
public:
    static constexpr uint16_t polynomial = 0xA001; // Reversed polynomial
    static constexpr uint16_t initial_value = 0x0000;
    static constexpr uint16_t final_xor = 0x0000;

    CRC16() : crc_(initial_value) {
        if (!table_initialized) {
            init_table();
        }
    }

    void update(uint8_t byte) {
        crc_ = (crc_ >> 8) ^ table_[(crc_ ^ byte) & 0xFF];
    }

    void update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            update(data[i]);
        }
    }

    uint16_t finalize() const {
        return crc_ ^ final_xor;
    }

    void reset() {
        crc_ = initial_value;
    }

    static uint16_t calculate(const uint8_t* data, size_t length) {
        CRC16 crc;
        crc.update(data, length);
        return crc.finalize();
    }

private:
    uint16_t crc_;
    static std::array<uint16_t, 256> table_;
    static bool table_initialized;

    static void init_table() {
        for (uint16_t i = 0; i < 256; ++i) {
            uint16_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c >> 1) ^ ((c & 1) ? polynomial : 0);
            }
            table_[i] = c;
        }
        table_initialized = true;
    }
};

inline std::array<uint16_t, 256> CRC16::table_{};
inline bool CRC16::table_initialized = false;

// ============================================================
// Adler-32 Checksum (used in zlib)
// ============================================================

class Adler32 {
public:
    static constexpr uint32_t mod_adler = 65521;

    Adler32() : a_(1), b_(0) {}

    void update(uint8_t byte) {
        a_ = (a_ + byte) % mod_adler;
        b_ = (b_ + a_) % mod_adler;
    }

    void update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            a_ = (a_ + data[i]) % mod_adler;
            b_ = (b_ + a_) % mod_adler;
        }
    }

    void update(std::span<const uint8_t> data) {
        update(data.data(), data.size());
    }

    uint32_t finalize() const {
        return (b_ << 16) | a_;
    }

    void reset() {
        a_ = 1;
        b_ = 0;
    }

    static uint32_t calculate(const uint8_t* data, size_t length) {
        Adler32 adler;
        adler.update(data, length);
        return adler.finalize();
    }

    static uint32_t calculate(std::span<const uint8_t> data) {
        return calculate(data.data(), data.size());
    }

private:
    uint32_t a_;
    uint32_t b_;
};

// ============================================================
// xxHash (fast non-cryptographic hash)
// ============================================================

class XXHash32 {
public:
    static constexpr uint32_t prime1 = 0x9E3779B1U;
    static constexpr uint32_t prime2 = 0x85EBCA77U;
    static constexpr uint32_t prime3 = 0xC2B2AE3DU;
    static constexpr uint32_t prime4 = 0x27D4EB2FU;
    static constexpr uint32_t prime5 = 0x165667B1U;

    explicit XXHash32(uint32_t seed = 0) : seed_(seed) {
        reset();
    }

    void update(const uint8_t* data, size_t length);
    uint32_t finalize() const;
    void reset();

    static uint32_t calculate(const uint8_t* data, size_t length, uint32_t seed = 0) {
        XXHash32 hash(seed);
        hash.update(data, length);
        return hash.finalize();
    }

private:
    uint32_t seed_;
    uint32_t v1_, v2_, v3_, v4_;
    uint32_t total_len_;
    uint32_t memsize_;
    std::array<uint8_t, 16> memory_;

    static uint32_t rotl(uint32_t x, int r) {
        return (x << r) | (x >> (32 - r));
    }

    static uint32_t round(uint32_t acc, uint32_t input) {
        acc += input * prime2;
        acc = rotl(acc, 13);
        acc *= prime1;
        return acc;
    }
};

// ============================================================
// Checksum wrapper for compressed data
// ============================================================

template<typename Checksum = CRC32>
class ChecksummedData {
public:
    template<typename InputIt, typename OutputIt>
    static void add_checksum(InputIt first, InputIt last, OutputIt output) {
        // Copy data and calculate checksum
        Checksum checksum;
        for (auto it = first; it != last; ++it) {
            uint8_t byte = *it;
            checksum.update(byte);
            *output++ = byte;
        }

        // Append checksum
        auto sum = checksum.finalize();
        for (size_t i = 0; i < sizeof(sum); ++i) {
            *output++ = static_cast<uint8_t>((sum >> (i * 8)) & 0xFF);
        }
    }

    template<typename InputIt>
    static bool verify_checksum(InputIt first, InputIt last) {
        Checksum temp_checksum;
        auto checksum_size = sizeof(decltype(temp_checksum.finalize()));

        if (std::distance(first, last) < checksum_size) {
            return false;
        }

        // Calculate checksum of data
        Checksum checksum;
        auto data_end = last - checksum_size;
        for (auto it = first; it != data_end; ++it) {
            checksum.update(*it);
        }

        // Read stored checksum
        decltype(checksum.finalize()) stored_sum = 0;
        size_t shift = 0;
        for (auto it = data_end; it != last; ++it) {
            stored_sum |= static_cast<decltype(stored_sum)>(*it) << shift;
            shift += 8;
        }

        return checksum.finalize() == stored_sum;
    }
};

} // namespace integrity
} // namespace pfc