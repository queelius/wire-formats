// new_codecs_demo.cpp - Demonstration of VByte, ExpGolomb, and EliasOmega
// Shows efficiency and usage patterns

#include <pfc/pfc.hpp>
#include <iostream>
#include <vector>
#include <iomanip>
#include <random>

using namespace pfc;

// Helper to display bits used by encoding
// Returns approximate bits (aligned to byte boundary)
template<typename Codec>
size_t count_bits(uint32_t value) {
    uint8_t buffer[100] = {0};
    BitWriter writer(buffer);
    Codec::encode(value, writer);
    writer.align();
    return writer.bytes_written() * 8;
}

void demonstrate_vbyte() {
    std::cout << "\n=== VByte Codec Demonstration ===\n\n";

    std::vector<uint32_t> test_values = {
        0, 127, 128, 255, 16383, 16384, 1000000
    };

    std::cout << "Value         Bytes  Bits   VByte Encoding\n";
    std::cout << "-----         -----  ----   --------------\n";

    for (auto value : test_values) {
        size_t bits = count_bits<codecs::VByte>(value);
        size_t bytes = (bits + 7) / 8;

        std::cout << std::setw(13) << value
                  << std::setw(7) << bytes
                  << std::setw(6) << bits
                  << "   ";

        // Show the actual encoding
        uint8_t buffer[20] = {0};
        BitWriter writer(buffer);
        codecs::VByte::encode(value, writer);
        writer.align();

        for (size_t i = 0; i < bytes; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(buffer[i]) << " ";
        }
        std::cout << std::dec << std::setfill(' ') << "\n";
    }

    std::cout << "\nNote: VByte is byte-aligned, making it cache-friendly.\n";
    std::cout << "The continuation bit (MSB) is 1 for the last byte.\n";
}

void demonstrate_expgolomb() {
    std::cout << "\n=== Exponential-Golomb Family ===\n\n";

    std::vector<uint32_t> test_values = {0, 1, 2, 5, 10, 50, 100, 1000};

    std::cout << "Value   Order-0   Order-1   Order-2   Fixed-32\n";
    std::cout << "-----   -------   -------   -------   --------\n";

    for (auto value : test_values) {
        size_t bits_0 = count_bits<codecs::ExpGolomb<0>>(value);
        size_t bits_1 = count_bits<codecs::ExpGolomb<1>>(value);
        size_t bits_2 = count_bits<codecs::ExpGolomb<2>>(value);
        size_t bits_fixed = 32;

        std::cout << std::setw(5) << value
                  << std::setw(10) << bits_0
                  << std::setw(10) << bits_1
                  << std::setw(10) << bits_2
                  << std::setw(11) << bits_fixed
                  << "\n";
    }

    std::cout << "\nNote: Higher orders flatten the distribution.\n";
    std::cout << "Order-0 is identical to Elias Gamma.\n";
}

void demonstrate_elias_omega() {
    std::cout << "\n=== Elias Omega vs Elias Delta ===\n\n";

    std::vector<uint32_t> test_values = {
        0, 1, 10, 100, 1000, 10000, 100000, 1000000
    };

    std::cout << "Value       Gamma   Delta   Omega   Fixed-32\n";
    std::cout << "-----       -----   -----   -----   --------\n";

    for (auto value : test_values) {
        size_t bits_gamma = count_bits<codecs::EliasGamma>(value);
        size_t bits_delta = count_bits<codecs::EliasDelta>(value);
        size_t bits_omega = count_bits<codecs::EliasOmega>(value);
        size_t bits_fixed = 32;

        std::cout << std::setw(11) << value
                  << std::setw(8) << bits_gamma
                  << std::setw(8) << bits_delta
                  << std::setw(8) << bits_omega
                  << std::setw(11) << bits_fixed
                  << "\n";
    }

    std::cout << "\nNote: Omega becomes more efficient than Delta for large values.\n";
}

void demonstrate_real_world_data() {
    std::cout << "\n=== Real-World Data Compression ===\n\n";

    // Simulate web analytics data (views per page)
    std::vector<uint32_t> page_views;
    std::mt19937 gen(42);

    // Most pages have few views (power-law distribution)
    std::geometric_distribution<uint32_t> dist(0.7);
    for (int i = 0; i < 10000; ++i) {
        page_views.push_back(dist(gen));
    }

    // Count total bits for each codec
    auto count_total_bits = [&page_views](auto encode_func) {
        uint8_t buffer[1000000] = {0};
        BitWriter writer(buffer);

        for (auto value : page_views) {
            encode_func(value, writer);
        }

        writer.align();
        return writer.bytes_written();
    };

    size_t vbyte_bytes = count_total_bits(
        [](uint32_t v, BitWriter& w) { codecs::VByte::encode(v, w); });

    size_t gamma_bytes = count_total_bits(
        [](uint32_t v, BitWriter& w) { codecs::EliasGamma::encode(v, w); });

    size_t delta_bytes = count_total_bits(
        [](uint32_t v, BitWriter& w) { codecs::EliasDelta::encode(v, w); });

    size_t omega_bytes = count_total_bits(
        [](uint32_t v, BitWriter& w) { codecs::EliasOmega::encode(v, w); });

    size_t expg1_bytes = count_total_bits(
        [](uint32_t v, BitWriter& w) { codecs::ExpGolomb<1>::encode(v, w); });

    size_t fixed_bytes = page_views.size() * 4;  // 32 bits each

    std::cout << "10,000 page view counts (geometric distribution):\n\n";
    std::cout << "Codec          Bytes     Bits/Value   Compression\n";
    std::cout << "-----          -----     ----------   -----------\n";

    auto print_stats = [](const char* name, size_t bytes, size_t count, size_t baseline) {
        double bits_per_value = (bytes * 8.0) / count;
        double compression = (1.0 - bytes / static_cast<double>(baseline)) * 100.0;

        std::cout << std::setw(14) << std::left << name
                  << std::setw(10) << std::right << bytes
                  << std::setw(13) << std::fixed << std::setprecision(2) << bits_per_value
                  << std::setw(12) << std::fixed << std::setprecision(1) << compression << "%\n";
    };

    print_stats("Fixed-32", fixed_bytes, page_views.size(), fixed_bytes);
    print_stats("VByte", vbyte_bytes, page_views.size(), fixed_bytes);
    print_stats("Elias Gamma", gamma_bytes, page_views.size(), fixed_bytes);
    print_stats("Elias Delta", delta_bytes, page_views.size(), fixed_bytes);
    print_stats("Elias Omega", omega_bytes, page_views.size(), fixed_bytes);
    print_stats("ExpGolomb-1", expg1_bytes, page_views.size(), fixed_bytes);

    std::cout << "\nNote: For small values (geometric distribution), Elias Gamma wins.\n";
    std::cout << "VByte's byte-alignment makes it fast but less space-efficient here.\n";
}

void demonstrate_zero_copy() {
    std::cout << "\n=== Zero-Copy Advantage ===\n\n";

    // Create a sequence of integers
    std::vector<uint32_t> data = {10, 20, 30, 100, 1000, 50, 5, 2};

    // Encode using different codecs
    uint8_t buffer[1000] = {0};
    BitWriter writer(buffer);

    std::cout << "Encoding sequence: ";
    for (auto v : data) std::cout << v << " ";
    std::cout << "\n\n";

    // Encode length + data
    codecs::EliasGamma::encode(static_cast<uint32_t>(data.size()), writer);
    for (auto v : data) {
        codecs::VByte::encode(v, writer);
    }
    writer.align();

    size_t bytes_used = writer.bytes_written();
    std::cout << "Encoded to " << bytes_used << " bytes\n";
    std::cout << "Fixed representation would use: " << (data.size() * 4) << " bytes\n";
    std::cout << "Space savings: "
              << std::fixed << std::setprecision(1)
              << (1.0 - bytes_used / (data.size() * 4.0)) * 100.0 << "%\n\n";

    // Decode (zero-copy read)
    BitReader reader(buffer, bytes_used);
    uint32_t length = codecs::EliasGamma::decode<uint32_t>(reader);

    std::cout << "Decoded sequence: ";
    for (uint32_t i = 0; i < length; ++i) {
        uint32_t value = codecs::VByte::decode<uint32_t>(reader);
        std::cout << value << " ";
    }
    std::cout << "\n\n";

    std::cout << "Key insight: Wire format = Memory format\n";
    std::cout << "  - No parsing step\n";
    std::cout << "  - No intermediate allocations\n";
    std::cout << "  - Direct memory mapping possible\n";
    std::cout << "  - Cache-friendly access patterns\n";
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     PFC Library - New Codecs Demonstration                 ║\n";
    std::cout << "║     VByte, Exponential-Golomb, Elias Omega                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    demonstrate_vbyte();
    demonstrate_expgolomb();
    demonstrate_elias_omega();
    demonstrate_real_world_data();
    demonstrate_zero_copy();

    std::cout << "\n=== Codec Selection Guidelines ===\n\n";
    std::cout << "VByte:\n";
    std::cout << "  ✓ When: Byte-aligned processing, SIMD optimization\n";
    std::cout << "  ✓ Best for: Moderate values, performance over compression\n";
    std::cout << "  ✓ Used in: Protocol Buffers, Lucene, LevelDB\n\n";

    std::cout << "Exponential-Golomb:\n";
    std::cout << "  ✓ When: Tunable distribution, video/audio codecs\n";
    std::cout << "  ✓ Best for: Known distribution characteristics\n";
    std::cout << "  ✓ Used in: H.264, HEVC, AAC\n\n";

    std::cout << "Elias Omega:\n";
    std::cout << "  ✓ When: Very large integers, unknown distribution\n";
    std::cout << "  ✓ Best for: Asymptotically optimal encoding\n";
    std::cout << "  ✓ Used in: Theoretical CS, succinct data structures\n\n";

    std::cout << "Elias Gamma:\n";
    std::cout << "  ✓ When: Small integers, simple implementation\n";
    std::cout << "  ✓ Best for: Geometric distributions\n";
    std::cout << "  ✓ Used in: Text compression, inverted indices\n\n";

    std::cout << "Elias Delta:\n";
    std::cout << "  ✓ When: Medium to large integers\n";
    std::cout << "  ✓ Best for: Balanced compression/speed\n";
    std::cout << "  ✓ Used in: Database indexes, compression algorithms\n\n";

    return 0;
}
