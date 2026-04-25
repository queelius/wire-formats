// tutorial.cpp - Simple usage examples for the PFC library
// Shows the beauty and simplicity of the API

#include <pfc/pfc.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>

using namespace pfc;

// Example 1: Basic compression
void example_basic_compression() {
    std::cout << "\n=== Example 1: Basic Compression ===\n";
    
    // Some data to compress
    std::vector<uint32_t> data = {1, 2, 3, 5, 8, 13, 21, 34, 55, 89};
    
    // Compress using Elias Gamma (default)
    auto compressed = compress(data);
    
    // Decompress
    auto decompressed = decompress<uint32_t>(compressed);
    
    // Show results
    std::cout << "Original size: " << data.size() * sizeof(uint32_t) << " bytes\n";
    std::cout << "Compressed size: " << compressed.size() << " bytes\n";
    std::cout << "Compression ratio: " 
              << std::fixed << std::setprecision(2)
              << (double)(data.size() * sizeof(uint32_t)) / compressed.size() 
              << "x\n";
    
    // Verify
    std::cout << "Data matches: " << (data == decompressed ? "Yes" : "No") << "\n";
}

// Example 2: Different codecs for different distributions
void example_codec_selection() {
    std::cout << "\n=== Example 2: Codec Selection ===\n";
    
    std::mt19937 gen(42);
    
    // Small numbers (geometric distribution)
    std::geometric_distribution<> geo_dist(0.3);
    std::vector<uint32_t> small_numbers;
    for (int i = 0; i < 1000; ++i) {
        small_numbers.push_back(geo_dist(gen));
    }
    
    // Large numbers (uniform distribution)  
    std::uniform_int_distribution<uint32_t> uni_dist(0, 1000000);
    std::vector<uint32_t> large_numbers;
    for (int i = 0; i < 1000; ++i) {
        large_numbers.push_back(uni_dist(gen));
    }
    
    // Compare codecs
    auto test_codec = [](const auto& data, const char* name, auto encode_fn) {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> buffer(data.size() * sizeof(uint32_t) * 2); // Extra space
        BitWriter writer{std::span<uint8_t>{buffer}};
        
        for (const auto& val : data) {
            encode_fn(val, writer);
        }
        writer.align();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        size_t compressed_size = writer.bytes_written();
        double bits_per_value = (compressed_size * 8.0) / data.size();
        
        std::cout << std::setw(15) << name << ": "
                  << std::setw(6) << compressed_size << " bytes, "
                  << std::setw(6) << std::fixed << std::setprecision(2) 
                  << bits_per_value << " bits/value, "
                  << std::setw(6) << duration.count() << " μs\n";
    };
    
    std::cout << "\nSmall numbers (geometric):";
    test_codec(small_numbers, "Elias Gamma", 
               [](uint32_t v, auto& w) { codecs::EliasGamma::encode(v, w); });
    test_codec(small_numbers, "Elias Delta", 
               [](uint32_t v, auto& w) { codecs::EliasDelta::encode(v, w); });
    test_codec(small_numbers, "Rice<3>", 
               [](uint32_t v, auto& w) { codecs::Rice<3>::encode(v, w); });
    
    std::cout << "\nLarge numbers (uniform):";
    test_codec(large_numbers, "Elias Gamma", 
               [](uint32_t v, auto& w) { codecs::EliasGamma::encode(v, w); });
    test_codec(large_numbers, "Elias Delta", 
               [](uint32_t v, auto& w) { codecs::EliasDelta::encode(v, w); });
    test_codec(large_numbers, "Fixed<20>", 
               [](uint32_t v, auto& w) { codecs::Fixed<20>::encode(v, w); });
}

// Example 3: Packed types composition
void example_packed_types() {
    std::cout << "\n=== Example 3: Packed Types ===\n";
    
    // Create a packed pair (rational number)
    using Rational = PackedPair<PackedU32<>, PackedU32<>>;
    Rational r({22, 7});  // pi approximation
    
    // Create a packed vector
    PackedVector<PackedU32<>> vec;
    vec.emplace_back(1);
    vec.emplace_back(2);
    vec.emplace_back(3);
    vec.emplace_back(5);
    vec.emplace_back(8);
    
    // Create a packed optional
    PackedOptional<PackedU32<>> maybe_value(42);
    PackedOptional<PackedU32<>> no_value;
    
    // Serialize everything
    std::vector<uint8_t> buffer(1024);
    BitWriter writer{std::span<uint8_t>{buffer}};
    
    Rational::encode(r, writer);
    PackedVector<PackedU32<>>::encode(vec, writer);
    PackedOptional<PackedU32<>>::encode(maybe_value, writer);
    PackedOptional<PackedU32<>>::encode(no_value, writer);
    writer.align();
    
    size_t total_bytes = writer.bytes_written();
    
    // Deserialize
    BitReader reader(buffer.data(), total_bytes);
    
    auto r2 = Rational::decode(reader);
    auto vec2 = PackedVector<PackedU32<>>::decode(reader);
    auto maybe2 = PackedOptional<PackedU32<>>::decode(reader);
    auto no2 = PackedOptional<PackedU32<>>::decode(reader);
    
    // Display results
    std::cout << "Rational: " << r2.first().value() << "/" << r2.second().value() << "\n";
    std::cout << "Vector size: " << vec2.size() << ", first element: " << vec2[0].value() << "\n";
    std::cout << "Optional with value: " << (maybe2.has_value() ? "Yes" : "No");
    if (maybe2) std::cout << ", value = " << (*maybe2).value();
    std::cout << "\nOptional without value: " << (no2.has_value() ? "Yes" : "No") << "\n";
    std::cout << "Total serialized size: " << total_bytes << " bytes\n";
}

// Example 4: Delta encoding for sorted data
void example_delta_encoding() {
    std::cout << "\n=== Example 4: Delta Encoding ===\n";
    
    // Sorted timestamps (milliseconds)
    std::vector<uint32_t> timestamps = {
        1000, 1010, 1015, 1020, 1025, 1030, 1040, 1050, 1055, 1060
    };
    
    // Regular encoding
    std::vector<uint8_t> regular_buffer(1024);
    BitWriter regular_writer{std::span<uint8_t>{regular_buffer}};
    for (auto ts : timestamps) {
        codecs::EliasGamma::encode(ts, regular_writer);
    }
    regular_writer.align();
    size_t regular_size = regular_writer.bytes_written();
    
    // Delta encoding
    std::vector<uint8_t> delta_buffer(1024);
    BitWriter delta_writer{std::span<uint8_t>{delta_buffer}};
    algorithms::DeltaCodec<uint32_t, codecs::EliasGamma>::encode_deltas(
        timestamps, delta_writer);
    delta_writer.align();
    size_t delta_size = delta_writer.bytes_written();
    
    // Decode to verify
    BitReader delta_reader(delta_buffer.data(), delta_size);
    auto decoded = algorithms::DeltaCodec<uint32_t, codecs::EliasGamma>::decode_deltas(
        delta_reader, timestamps.size());
    
    std::cout << "Regular encoding: " << regular_size << " bytes\n";
    std::cout << "Delta encoding: " << delta_size << " bytes\n";
    std::cout << "Improvement: " 
              << std::fixed << std::setprecision(1)
              << (1.0 - (double)delta_size / regular_size) * 100 << "%\n";
    std::cout << "Data matches: " << (timestamps == decoded ? "Yes" : "No") << "\n";
}

// Example 5: Signed integers
void example_signed_integers() {
    std::cout << "\n=== Example 5: Signed Integers ===\n";
    
    std::vector<int32_t> signed_data = {-5, -3, -1, 0, 1, 3, 5, 7, -10, 20};
    
    // Encode
    std::vector<uint8_t> buffer(256);
    BitWriter writer{std::span<uint8_t>{buffer}};
    
    for (auto val : signed_data) {
        codecs::SignedGamma::encode(val, writer);
    }
    writer.align();
    
    // Decode
    BitReader reader(buffer.data(), writer.bytes_written());
    std::vector<int32_t> decoded;
    
    for (size_t i = 0; i < signed_data.size(); ++i) {
        decoded.push_back(codecs::SignedGamma::decode<int32_t>(reader));
    }
    
    // Display
    std::cout << "Original: ";
    for (auto v : signed_data) std::cout << v << " ";
    std::cout << "\nDecoded:  ";
    for (auto v : decoded) std::cout << v << " ";
    std::cout << "\nMatches: " << (signed_data == decoded ? "Yes" : "No") << "\n";
}

int main() {
    std::cout << "PFC Library Tutorial\n";
    std::cout << "====================\n";
    
    example_basic_compression();
    example_codec_selection();
    example_packed_types();
    example_delta_encoding();
    example_signed_integers();
    
    std::cout << "\nAll examples completed successfully!\n";
    return 0;
}