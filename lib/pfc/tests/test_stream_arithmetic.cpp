// test_stream_arithmetic.cpp - Tests for stream I/O and arithmetic coding
// Ensuring correctness and robustness of the new features

#include "catch_amalgamated.hpp"
#include "../include/pfc/pfc.hpp"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <random>
#include <algorithm>
#include <numeric>

using namespace pfc;

// ============================================================
//  Stream I/O Tests
// ============================================================

TEST_CASE("Stream bit writer/reader basic operations", "[stream_io]") {
    std::ostringstream oss(std::ios::binary);

    SECTION("Single bit operations") {
        stream_bit_writer writer(oss);
        writer.write(true);
        writer.write(false);
        writer.write(true);
        writer.write(true);
        writer.flush();

        std::string data = oss.str();
        REQUIRE(!data.empty());

        std::istringstream iss(data, std::ios::binary);
        stream_bit_reader reader(iss);

        REQUIRE(reader.read() == true);
        REQUIRE(reader.read() == false);
        REQUIRE(reader.read() == true);
        REQUIRE(reader.read() == true);
    }

    SECTION("Multi-bit operations") {
        stream_bit_writer writer(oss);
        writer.write_bits(0b10110101, 8);  // 181 in decimal
        writer.write_bits(0b11001, 5);     // 25 in decimal
        writer.flush();

        std::string data = oss.str();
        std::istringstream iss(data, std::ios::binary);
        stream_bit_reader reader(iss);

        REQUIRE(reader.read_bits(8) == 0b10110101);
        REQUIRE(reader.read_bits(5) == 0b11001);
    }

    SECTION("Byte-aligned operations") {
        stream_bit_writer writer(oss);
        uint8_t test_data[] = {0x12, 0x34, 0x56, 0x78};
        writer.write_bytes(test_data, 4);

        std::string data = oss.str();
        std::istringstream iss(data, std::ios::binary);
        stream_bit_reader reader(iss);

        uint8_t read_data[4];
        reader.read_bytes(read_data, 4);

        for (int i = 0; i < 4; ++i) {
            REQUIRE(read_data[i] == test_data[i]);
        }
    }
}

TEST_CASE("Stream I/O with codecs", "[stream_io]") {
    SECTION("Elias Gamma codec through streams") {
        std::ostringstream oss(std::ios::binary);
        stream_bit_writer writer(oss);

        std::vector<uint32_t> test_values = {1, 5, 10, 100, 1000, 10000};

        for (auto val : test_values) {
            codecs::EliasGamma::encode(val, writer);
        }
        writer.flush();

        std::string data = oss.str();
        std::istringstream iss(data, std::ios::binary);
        stream_bit_reader reader(iss);

        for (auto expected : test_values) {
            auto decoded = codecs::EliasGamma::decode<uint32_t>(reader);
            REQUIRE(decoded == expected);
        }
    }

    SECTION("Mixed codec types") {
        std::ostringstream oss(std::ios::binary);
        stream_bit_writer writer(oss);

        codecs::EliasGamma::encode(42u, writer);
        codecs::EliasDelta::encode(1337u, writer);
        codecs::Unary::encode(7u, writer);
        writer.flush();

        std::string data = oss.str();
        std::istringstream iss(data, std::ios::binary);
        stream_bit_reader reader(iss);

        REQUIRE(codecs::EliasGamma::decode<uint32_t>(reader) == 42u);
        REQUIRE(codecs::EliasDelta::decode<uint32_t>(reader) == 1337u);
        REQUIRE(codecs::Unary::decode<uint32_t>(reader) == 7u);
    }
}

TEST_CASE("File I/O operations", "[stream_io]") {
    const std::string test_file = "test_compression.pfc";

    SECTION("Compress and decompress to/from file") {
        std::vector<uint32_t> original = {1, 2, 3, 5, 8, 13, 21, 34, 55, 89};

        // Write compressed data
        {
            auto result = write_compressed_file<uint32_t, codecs::EliasGamma>(
                test_file, original);
            REQUIRE(result.has_value());
            REQUIRE(result.value() > 0);
        }

        // Read compressed data
        {
            auto result = read_compressed_file<uint32_t, codecs::EliasGamma>(test_file);
            REQUIRE(result.has_value());
            REQUIRE(result.value() == original);
        }

        // Cleanup
        std::filesystem::remove(test_file);
    }

    SECTION("Large dataset compression") {
        std::vector<uint32_t> large_data(10000);
        std::iota(large_data.begin(), large_data.end(), 1);

        {
            auto result = write_compressed_file<uint32_t, codecs::EliasDelta>(
                test_file, large_data);
            REQUIRE(result.has_value());

            // Check compression ratio
            size_t uncompressed_size = large_data.size() * sizeof(uint32_t);
            size_t compressed_size = result.value();
            double ratio = static_cast<double>(compressed_size) / uncompressed_size;
            REQUIRE(ratio < 0.7);  // Should achieve some compression (sequential data)
        }

        {
            auto result = read_compressed_file<uint32_t, codecs::EliasDelta>(test_file);
            REQUIRE(result.has_value());
            REQUIRE(result.value() == large_data);
        }

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("Memory stream operations", "[stream_io]") {
    SECTION("String compression/decompression") {
        std::vector<uint16_t> data = {100, 200, 300, 400, 500};

        auto compressed_result = compress_to_string<uint16_t, codecs::EliasGamma>(data);
        REQUIRE(compressed_result.has_value());

        auto compressed = compressed_result.value();
        REQUIRE(!compressed.empty());

        auto decompressed_result = decompress_from_string<uint16_t, codecs::EliasGamma>(compressed);
        REQUIRE(decompressed_result.has_value());
        REQUIRE(decompressed_result.value() == data);
    }
}

// ============================================================
//  Arithmetic Coding Tests
// ============================================================

TEST_CASE("Probability models", "[arithmetic]") {
    SECTION("Static probability model") {
        static_probability_model<256> model;

        // Initially uniform
        for (size_t i = 0; i < 256; ++i) {
            REQUIRE(model.frequency(i) == 1);
        }
        REQUIRE(model.total_frequency() == 256);

        // Set custom frequencies
        std::array<uint32_t, 256> freqs{};
        freqs[0] = 100;
        freqs[1] = 50;
        freqs[2] = 25;
        // Rest remain zero

        model.set_frequencies(freqs);
        REQUIRE(model.frequency(0) == 100);
        REQUIRE(model.frequency(1) == 50);
        REQUIRE(model.frequency(2) == 25);
        REQUIRE(model.total_frequency() == 175);

        // Test symbol finding
        REQUIRE(model.find_symbol(0) == 0);    // [0, 100)
        REQUIRE(model.find_symbol(99) == 0);   // [0, 100)
        REQUIRE(model.find_symbol(100) == 1);  // [100, 150)
        REQUIRE(model.find_symbol(149) == 1);  // [100, 150)
        REQUIRE(model.find_symbol(150) == 2);  // [150, 175)
    }

    SECTION("Adaptive probability model") {
        adaptive_probability_model<256> model;

        // Initially uniform
        REQUIRE(model.total_frequency() == 256);

        // Update frequencies
        for (int i = 0; i < 10; ++i) {
            model.update(65);  // Update 'A'
        }

        // 'A' should have higher frequency now
        REQUIRE(model.frequency(65) > model.frequency(66));

        // Total should increase
        REQUIRE(model.total_frequency() > 256);
    }
}

TEST_CASE("Arithmetic encoder/decoder", "[arithmetic]") {
    SECTION("Basic encoding/decoding") {
        std::vector<uint8_t> buffer(1024);
        BitWriter writer(buffer.data());

        arithmetic_encoder encoder(writer);
        adaptive_probability_model<256> encode_model;

        // Encode symbols
        std::vector<size_t> symbols = {72, 101, 108, 108, 111};  // "Hello"
        for (auto sym : symbols) {
            encoder.encode_symbol(sym, encode_model);
        }
        encoder.finish();
        writer.align();

        // Decode
        BitReader reader(buffer.data(), writer.bytes_written());
        arithmetic_decoder decoder(reader);
        adaptive_probability_model<256> decode_model;

        std::vector<size_t> decoded;
        for (size_t i = 0; i < symbols.size(); ++i) {
            decoded.push_back(decoder.decode_symbol(decode_model));
        }

        REQUIRE(decoded == symbols);
    }

    SECTION("Random data round-trip") {
        std::mt19937 gen(42);
        std::uniform_int_distribution<> dist(0, 255);

        std::vector<uint8_t> original(1000);
        std::generate(original.begin(), original.end(),
                     [&]() { return dist(gen); });

        // Compress
        std::vector<uint8_t> compressed(original.size() * 2);
        BitWriter writer(compressed.data());

        arithmetic_encoder encoder(writer);
        adaptive_probability_model<256> encode_model;

        for (auto byte : original) {
            encoder.encode_symbol(byte, encode_model);
        }
        encoder.finish();
        writer.align();

        compressed.resize(writer.bytes_written());

        // Decompress
        BitReader reader(compressed.data(), compressed.size());
        arithmetic_decoder decoder(reader);
        adaptive_probability_model<256> decode_model;

        std::vector<uint8_t> decompressed;
        for (size_t i = 0; i < original.size(); ++i) {
            decompressed.push_back(decoder.decode_symbol(decode_model));
        }

        REQUIRE(decompressed == original);
    }
}

TEST_CASE("Range encoder/decoder", "[arithmetic]") {
    SECTION("Basic encoding/decoding") {
        std::vector<uint8_t> buffer(1024);
        BitWriter writer(buffer.data());

        range_encoder encoder(writer);
        adaptive_probability_model<256> encode_model;

        // Encode text
        std::string text = "The quick brown fox jumps over the lazy dog";
        for (char c : text) {
            encoder.encode_symbol(static_cast<uint8_t>(c), encode_model);
        }
        encoder.finish();
        writer.align();

        // Decode
        BitReader reader(buffer.data(), writer.bytes_written());
        range_decoder decoder(reader);
        adaptive_probability_model<256> decode_model;

        std::string decoded;
        for (size_t i = 0; i < text.size(); ++i) {
            decoded += static_cast<char>(decoder.decode_symbol(decode_model));
        }

        REQUIRE(decoded == text);
    }

    SECTION("Binary range coding") {
        std::vector<uint8_t> buffer(1024);
        BitWriter writer(buffer.data());

        range_encoder encoder(writer);

        // Encode bits with varying probabilities
        std::vector<bool> bits = {true, false, true, true, false, false, true};
        std::vector<uint32_t> probs = {0x8000, 0xC000, 0x4000, 0x8000, 0xA000, 0x6000, 0x8000};

        for (size_t i = 0; i < bits.size(); ++i) {
            encoder.encode_bit(bits[i], probs[i]);
        }
        encoder.finish();
        writer.align();

        // Decode
        BitReader reader(buffer.data(), writer.bytes_written());
        range_decoder decoder(reader);

        std::vector<bool> decoded;
        for (size_t i = 0; i < bits.size(); ++i) {
            decoded.push_back(decoder.decode_bit(probs[i]));
        }

        REQUIRE(decoded == bits);
    }
}

TEST_CASE("High-level arithmetic API", "[arithmetic]") {
    SECTION("Compress/decompress bytes") {
        std::string input = "abracadabra alakazam";
        std::vector<uint8_t> input_bytes(input.begin(), input.end());

        std::vector<uint8_t> compressed;
        auto compress_result = arithmetic_compress(input_bytes.begin(),
                                                  input_bytes.end(),
                                                  std::back_inserter(compressed));
        REQUIRE(compress_result.has_value());
        REQUIRE(compress_result.value() > 0);
        REQUIRE(compressed.size() < input_bytes.size());  // Should compress

        std::vector<uint8_t> decompressed;
        auto decompress_result = arithmetic_decompress(compressed.begin(),
                                                      compressed.end(),
                                                      std::back_inserter(decompressed),
                                                      input_bytes.size());
        REQUIRE(decompress_result.has_value());
        REQUIRE(decompressed == input_bytes);
    }

    SECTION("Range coding variant") {
        std::vector<uint8_t> data(256);
        std::iota(data.begin(), data.end(), 0);

        std::vector<uint8_t> compressed;
        auto compress_result = range_compress(data.begin(), data.end(),
                                             std::back_inserter(compressed));
        REQUIRE(compress_result.has_value());

        std::vector<uint8_t> decompressed;
        auto decompress_result = range_decompress(compressed.begin(),
                                                 compressed.end(),
                                                 std::back_inserter(decompressed),
                                                 data.size());
        REQUIRE(decompress_result.has_value());
        REQUIRE(decompressed == data);
    }
}

TEST_CASE("Compression efficiency", "[arithmetic]") {
    SECTION("Highly redundant data") {
        // Data with lots of repetition
        std::vector<uint8_t> redundant(1000, 'A');
        for (size_t i = 0; i < 100; ++i) {
            redundant[i * 10] = 'B';
        }

        std::vector<uint8_t> compressed;
        auto result = arithmetic_compress(redundant.begin(), redundant.end(),
                                        std::back_inserter(compressed));
        REQUIRE(result.has_value());

        double compression_ratio = static_cast<double>(compressed.size()) / redundant.size();
        REQUIRE(compression_ratio < 0.2);  // Should achieve excellent compression
    }

    SECTION("Random data") {
        std::mt19937 gen(12345);
        std::uniform_int_distribution<> dist(0, 255);

        std::vector<uint8_t> random_data(1000);
        std::generate(random_data.begin(), random_data.end(),
                     [&]() { return dist(gen); });

        std::vector<uint8_t> compressed;
        auto result = arithmetic_compress(random_data.begin(), random_data.end(),
                                        std::back_inserter(compressed));
        REQUIRE(result.has_value());

        double compression_ratio = static_cast<double>(compressed.size()) / random_data.size();
        REQUIRE(compression_ratio > 0.9);  // Random data shouldn't compress much
    }
}

// ============================================================
//  Codec Template Tests (RangeCodec<T> and ArithmeticCodec<T>)
// ============================================================

TEST_CASE("ArithmeticCodec template", "[arithmetic][codec]") {
    SECTION("Encodes and decodes uint32_t") {
        std::vector<uint32_t> test_values = {0, 1, 255, 256, 65535, 0xDEADBEEF, 0xFFFFFFFF};

        for (auto original : test_values) {
            std::vector<uint8_t> buffer(64);
            BitWriter writer(buffer.data());

            ArithmeticCodec<uint32_t>::encode(original, writer);
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            auto decoded = ArithmeticCodec<uint32_t>::decode(reader);

            REQUIRE(decoded == original);
        }
    }

    SECTION("Encodes and decodes uint64_t") {
        std::vector<uint64_t> test_values = {0, 1, 0xFFFFFFFFULL, 0x123456789ABCDEFULL};

        for (auto original : test_values) {
            std::vector<uint8_t> buffer(128);
            BitWriter writer(buffer.data());

            ArithmeticCodec<uint64_t>::encode(original, writer);
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            auto decoded = ArithmeticCodec<uint64_t>::decode(reader);

            REQUIRE(decoded == original);
        }
    }

    SECTION("Encodes and decodes struct") {
        struct TestStruct {
            uint32_t a;
            uint16_t b;
            uint8_t c;
            uint8_t d;

            bool operator==(const TestStruct& other) const {
                return a == other.a && b == other.b && c == other.c && d == other.d;
            }
        };

        TestStruct original{0x12345678, 0xABCD, 0xEF, 0x01};

        std::vector<uint8_t> buffer(128);
        BitWriter writer(buffer.data());

        ArithmeticCodec<TestStruct>::encode(original, writer);
        writer.align();

        BitReader reader(buffer.data(), writer.bytes_written());
        auto decoded = ArithmeticCodec<TestStruct>::decode(reader);

        REQUIRE(decoded == original);
    }
}

TEST_CASE("RangeCodec template", "[arithmetic][codec]") {
    SECTION("Encodes and decodes uint32_t") {
        std::vector<uint32_t> test_values = {0, 1, 255, 256, 65535, 0xDEADBEEF, 0xFFFFFFFF};

        for (auto original : test_values) {
            std::vector<uint8_t> buffer(64);
            BitWriter writer(buffer.data());

            RangeCodec<uint32_t>::encode(original, writer);
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            auto decoded = RangeCodec<uint32_t>::decode(reader);

            REQUIRE(decoded == original);
        }
    }

    SECTION("Encodes and decodes uint64_t") {
        std::vector<uint64_t> test_values = {0, 1, 0xFFFFFFFFULL, 0x123456789ABCDEFULL};

        for (auto original : test_values) {
            std::vector<uint8_t> buffer(128);
            BitWriter writer(buffer.data());

            RangeCodec<uint64_t>::encode(original, writer);
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            auto decoded = RangeCodec<uint64_t>::decode(reader);

            REQUIRE(decoded == original);
        }
    }

    SECTION("Encodes and decodes int32_t including negatives") {
        std::vector<int32_t> test_values = {0, 1, -1, 127, -128, 32767, -32768, INT32_MAX, INT32_MIN};

        for (auto original : test_values) {
            std::vector<uint8_t> buffer(64);
            BitWriter writer(buffer.data());

            RangeCodec<int32_t>::encode(original, writer);
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            auto decoded = RangeCodec<int32_t>::decode(reader);

            REQUIRE(decoded == original);
        }
    }
}

// ============================================================
//  Single-Symbol and Edge Case Tests
// ============================================================

TEST_CASE("Single symbol encoding", "[arithmetic][edge]") {
    SECTION("Arithmetic coder single symbol") {
        for (size_t sym = 0; sym < 256; ++sym) {
            std::vector<uint8_t> buffer(64);
            BitWriter writer(buffer.data());

            arithmetic_encoder encoder(writer);
            adaptive_probability_model<256> encode_model;
            encoder.encode_symbol(sym, encode_model);
            encoder.finish();
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            arithmetic_decoder decoder(reader);
            adaptive_probability_model<256> decode_model;
            auto decoded = decoder.decode_symbol(decode_model);

            REQUIRE(decoded == sym);
        }
    }

    SECTION("Range coder single symbol") {
        for (size_t sym = 0; sym < 256; ++sym) {
            std::vector<uint8_t> buffer(64);
            BitWriter writer(buffer.data());

            range_encoder encoder(writer);
            adaptive_probability_model<256> encode_model;
            encoder.encode_symbol(sym, encode_model);
            encoder.finish();
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            range_decoder decoder(reader);
            adaptive_probability_model<256> decode_model;
            auto decoded = decoder.decode_symbol(decode_model);

            REQUIRE(decoded == sym);
        }
    }
}

TEST_CASE("Binary encoding edge probabilities", "[arithmetic][edge]") {
    SECTION("Range coder with extreme probabilities") {
        // Test probability extremes (but not 0 or 0x10000 which would cause issues)
        std::vector<uint32_t> edge_probs = {1, 2, 0x7FFF, 0x8000, 0x8001, 0xFFFE, 0xFFFF};

        for (auto prob : edge_probs) {
            std::vector<uint8_t> buffer(64);
            BitWriter writer(buffer.data());

            range_encoder encoder(writer);

            // Encode alternating bits with this probability
            std::vector<bool> bits = {false, true, false, true};
            for (bool bit : bits) {
                encoder.encode_bit(bit, prob);
            }
            encoder.finish();
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            range_decoder decoder(reader);

            std::vector<bool> decoded;
            for (size_t i = 0; i < bits.size(); ++i) {
                decoded.push_back(decoder.decode_bit(prob));
            }

            REQUIRE(decoded == bits);
        }
    }

    SECTION("Range coder with 50/50 probability") {
        std::vector<uint8_t> buffer(64);
        BitWriter writer(buffer.data());

        range_encoder encoder(writer);
        uint32_t prob_50_50 = 0x8000;  // 50% probability of zero

        std::vector<bool> bits = {true, true, false, false, true, false};
        for (bool bit : bits) {
            encoder.encode_bit(bit, prob_50_50);
        }
        encoder.finish();
        writer.align();

        BitReader reader(buffer.data(), writer.bytes_written());
        range_decoder decoder(reader);

        std::vector<bool> decoded;
        for (size_t i = 0; i < bits.size(); ++i) {
            decoded.push_back(decoder.decode_bit(prob_50_50));
        }

        REQUIRE(decoded == bits);
    }
}

TEST_CASE("Static probability model with range coder", "[arithmetic][edge]") {
    SECTION("Non-uniform static model") {
        // Create a heavily skewed distribution
        std::array<uint32_t, 256> freqs{};
        freqs[0] = 1000;   // 'A' very common
        freqs[1] = 100;    // 'B' less common
        freqs[2] = 10;     // 'C' rare
        for (size_t i = 3; i < 256; ++i) {
            freqs[i] = 1;  // Others very rare
        }

        // Encode a sequence that matches the distribution
        std::vector<size_t> symbols = {0, 0, 0, 1, 0, 0, 2, 0, 1, 0};

        std::vector<uint8_t> buffer(256);
        BitWriter writer(buffer.data());

        static_probability_model<256> encode_model;
        encode_model.set_frequencies(freqs);
        arithmetic_encoder encoder(writer);

        for (auto sym : symbols) {
            encoder.encode_symbol(sym, encode_model);
        }
        encoder.finish();
        writer.align();

        BitReader reader(buffer.data(), writer.bytes_written());
        static_probability_model<256> decode_model;
        decode_model.set_frequencies(freqs);
        arithmetic_decoder decoder(reader);

        std::vector<size_t> decoded;
        for (size_t i = 0; i < symbols.size(); ++i) {
            decoded.push_back(decoder.decode_symbol(decode_model));
        }

        REQUIRE(decoded == symbols);
    }
}

// ============================================================
//  Model Rescaling Tests
// ============================================================

TEST_CASE("Adaptive model rescaling", "[arithmetic][rescale]") {
    SECTION("Direct model rescaling verification") {
        // For 256 symbols, rescale_threshold = 16383 * 256 = 4,194,048
        // We'll use a smaller alphabet to make testing faster
        adaptive_probability_model<4> model;  // threshold = 16383 * 4 = 65,532

        // Initial state: each symbol has frequency 1, total = 4
        REQUIRE(model.total_frequency() == 4);

        // Update symbol 0 many times to trigger rescaling
        for (uint32_t i = 0; i < 70000; ++i) {
            model.update(0);
        }

        // After rescaling, total should be much less than 70004
        // (70000 updates + 4 initial = 70004 if no rescaling)
        REQUIRE(model.total_frequency() < 70004);

        // Frequency of symbol 0 should still be highest
        REQUIRE(model.frequency(0) > model.frequency(1));
    }

    SECTION("Range coder survives rescaling") {
        // Use a small alphabet model that will rescale during encoding
        // Generate enough data to trigger rescaling
        std::mt19937 gen(42);
        std::uniform_int_distribution<> dist(0, 3);  // 4 symbols

        // Generate data - enough to trigger multiple rescales
        std::vector<size_t> original(20000);
        // Bias towards symbol 0 to make rescaling happen faster
        for (auto& sym : original) {
            sym = (dist(gen) == 0) ? 0 : dist(gen);
        }

        std::vector<uint8_t> buffer(original.size() * 2);
        BitWriter writer(buffer.data());

        range_encoder encoder(writer);
        adaptive_probability_model<4> encode_model;

        for (auto sym : original) {
            encoder.encode_symbol(sym, encode_model);
        }
        encoder.finish();
        writer.align();

        BitReader reader(buffer.data(), writer.bytes_written());
        range_decoder decoder(reader);
        adaptive_probability_model<4> decode_model;

        std::vector<size_t> decoded;
        for (size_t i = 0; i < original.size(); ++i) {
            decoded.push_back(decoder.decode_symbol(decode_model));
        }

        REQUIRE(decoded == original);
    }
}

// ============================================================
//  Property-Based Tests
// ============================================================

TEST_CASE("Property: decode(encode(x)) == x", "[arithmetic][property]") {
    std::mt19937 gen(12345);

    SECTION("Random uint32_t values with ArithmeticCodec") {
        std::uniform_int_distribution<uint32_t> dist;

        for (int trial = 0; trial < 100; ++trial) {
            uint32_t original = dist(gen);

            std::vector<uint8_t> buffer(64);
            BitWriter writer(buffer.data());
            ArithmeticCodec<uint32_t>::encode(original, writer);
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            auto decoded = ArithmeticCodec<uint32_t>::decode(reader);

            REQUIRE(decoded == original);
        }
    }

    SECTION("Random uint32_t values with RangeCodec") {
        std::uniform_int_distribution<uint32_t> dist;

        for (int trial = 0; trial < 100; ++trial) {
            uint32_t original = dist(gen);

            std::vector<uint8_t> buffer(64);
            BitWriter writer(buffer.data());
            RangeCodec<uint32_t>::encode(original, writer);
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            auto decoded = RangeCodec<uint32_t>::decode(reader);

            REQUIRE(decoded == original);
        }
    }

    SECTION("Random byte sequences with range coder") {
        std::uniform_int_distribution<> len_dist(1, 500);
        std::uniform_int_distribution<> byte_dist(0, 255);

        for (int trial = 0; trial < 20; ++trial) {
            size_t len = len_dist(gen);
            std::vector<uint8_t> original(len);
            std::generate(original.begin(), original.end(),
                         [&]() { return byte_dist(gen); });

            std::vector<uint8_t> buffer(original.size() * 2 + 64);
            BitWriter writer(buffer.data());

            range_encoder encoder(writer);
            adaptive_probability_model<256> encode_model;
            for (auto byte : original) {
                encoder.encode_symbol(byte, encode_model);
            }
            encoder.finish();
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            range_decoder decoder(reader);
            adaptive_probability_model<256> decode_model;

            std::vector<uint8_t> decoded;
            for (size_t i = 0; i < original.size(); ++i) {
                decoded.push_back(static_cast<uint8_t>(decoder.decode_symbol(decode_model)));
            }

            REQUIRE(decoded == original);
        }
    }

    SECTION("Random byte sequences with arithmetic coder") {
        std::uniform_int_distribution<> len_dist(1, 500);
        std::uniform_int_distribution<> byte_dist(0, 255);

        for (int trial = 0; trial < 20; ++trial) {
            size_t len = len_dist(gen);
            std::vector<uint8_t> original(len);
            std::generate(original.begin(), original.end(),
                         [&]() { return byte_dist(gen); });

            std::vector<uint8_t> buffer(original.size() * 2 + 64);
            BitWriter writer(buffer.data());

            arithmetic_encoder encoder(writer);
            adaptive_probability_model<256> encode_model;
            for (auto byte : original) {
                encoder.encode_symbol(byte, encode_model);
            }
            encoder.finish();
            writer.align();

            BitReader reader(buffer.data(), writer.bytes_written());
            arithmetic_decoder decoder(reader);
            adaptive_probability_model<256> decode_model;

            std::vector<uint8_t> decoded;
            for (size_t i = 0; i < original.size(); ++i) {
                decoded.push_back(static_cast<uint8_t>(decoder.decode_symbol(decode_model)));
            }

            REQUIRE(decoded == original);
        }
    }
}

TEST_CASE("Property: compression is deterministic", "[arithmetic][property]") {
    SECTION("Same input produces same output") {
        std::string input = "The quick brown fox jumps over the lazy dog";

        auto compress_once = [&]() {
            std::vector<uint8_t> buffer(input.size() * 2);
            BitWriter writer(buffer.data());

            range_encoder encoder(writer);
            adaptive_probability_model<256> model;
            for (char c : input) {
                encoder.encode_symbol(static_cast<uint8_t>(c), model);
            }
            encoder.finish();
            writer.align();

            buffer.resize(writer.bytes_written());
            return buffer;
        };

        auto result1 = compress_once();
        auto result2 = compress_once();

        REQUIRE(result1 == result2);
    }
}

// ============================================================
//  Integration Tests
// ============================================================

TEST_CASE("Stream I/O with arithmetic coding", "[integration]") {
    SECTION("File compression pipeline") {
        const std::string test_file = "test_arithmetic.pfc";

        // Create test data with some structure
        std::vector<uint32_t> data;
        for (int i = 0; i < 1000; ++i) {
            data.push_back(i % 100);  // Repeating pattern
        }

        // Compress to file using arithmetic coding
        {
            std::ofstream file(test_file, std::ios::binary);
            REQUIRE(file.is_open());

            stream_bit_writer writer(file);

            // Write header
            codecs::Fixed<32>::encode(static_cast<uint32_t>(data.size()), writer);

            // Setup arithmetic encoder
            std::vector<uint8_t> temp_buffer(data.size() * 4);
            BitWriter temp_writer(temp_buffer.data());

            range_encoder encoder(temp_writer);
            adaptive_probability_model<256> model;

            // Encode data
            for (auto val : data) {
                // Encode each byte of the value
                for (int i = 0; i < 4; ++i) {
                    encoder.encode_symbol((val >> (i * 8)) & 0xFF, model);
                }
            }
            encoder.finish();
            temp_writer.align();

            // Write compressed data
            writer.write_bytes(temp_buffer.data(), temp_writer.bytes_written());
            writer.flush();
        }

        // Decompress from file
        {
            std::ifstream file(test_file, std::ios::binary);
            REQUIRE(file.is_open());

            stream_bit_reader reader(file);

            // Read header
            auto size = codecs::Fixed<32>::decode<uint32_t>(reader);
            REQUIRE(size == data.size());

            // Read compressed data into buffer
            std::vector<uint8_t> compressed_buffer(10000);
            auto bytes_read = reader.read_bytes(compressed_buffer.data(), compressed_buffer.size());

            // Setup arithmetic decoder
            BitReader temp_reader(compressed_buffer.data(), bytes_read);
            range_decoder decoder(temp_reader);
            adaptive_probability_model<256> model;

            // Decode data
            std::vector<uint32_t> decoded;
            for (uint32_t i = 0; i < size; ++i) {
                uint32_t val = 0;
                for (int j = 0; j < 4; ++j) {
                    val |= decoder.decode_symbol(model) << (j * 8);
                }
                decoded.push_back(val);
            }

            REQUIRE(decoded == data);
        }

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("Error handling", "[stream_io][arithmetic]") {
    SECTION("Invalid file operations") {
        auto result = read_compressed_file<uint32_t>("/nonexistent/path/file.pfc");
        REQUIRE(!result.has_value());
        REQUIRE(result.error() == make_error_code(pfc_error::io_error));
    }

    // Note: Corrupted data handling is not tested here because the stream reader
    // doesn't gracefully handle EOF conditions when more data is expected.
    // This would require adding proper EOF detection to the bit reader.
}