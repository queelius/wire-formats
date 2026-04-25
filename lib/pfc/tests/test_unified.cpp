#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include <pfc/pfc.hpp>
#include <random>
#include <limits>

using namespace pfc;

TEST_CASE("BitWriter and BitReader", "[core]") {
    SECTION("Single bits") {
        uint8_t buffer[10] = {0};
        BitWriter writer(buffer);
        
        writer.write(true);
        writer.write(false);
        writer.write(true);
        writer.write(true);
        writer.align();
        
        BitReader reader(buffer, 10);
        REQUIRE(reader.read() == true);
        REQUIRE(reader.read() == false);
        REQUIRE(reader.read() == true);
        REQUIRE(reader.read() == true);
    }
    
    SECTION("Multiple bits") {
        uint8_t buffer[10] = {0};
        BitWriter writer(buffer);
        
        writer.write_bits(0b10110101, 8);
        writer.write_bits(0b1111, 4);
        writer.align();
        
        BitReader reader(buffer, 10);
        REQUIRE(reader.read_bits(8) == 0b10110101);
        REQUIRE(reader.read_bits(4) == 0b1111);
    }
}

TEST_CASE("Elias Gamma Codec", "[codecs]") {
    auto test_value = [](uint32_t value) {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        codecs::EliasGamma::encode(value, writer);
        writer.align();
        
        BitReader reader(buffer, 100);
        uint32_t decoded = codecs::EliasGamma::decode<uint32_t>(reader);
        REQUIRE(decoded == value);
    };
    
    SECTION("Small values") {
        test_value(0);
        test_value(1);
        test_value(2);
        test_value(10);
    }
    
    SECTION("Large values") {
        test_value(100);
        test_value(1000);
        test_value(65535);
        test_value(1000000);
    }
    
    SECTION("Random values") {
        std::mt19937 gen(42);
        std::uniform_int_distribution<uint32_t> dist(0, 1000000);
        
        for (int i = 0; i < 100; ++i) {
            test_value(dist(gen));
        }
    }
}

TEST_CASE("Elias Delta Codec", "[codecs]") {
    auto test_value = [](uint32_t value) {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        codecs::EliasDelta::encode(value, writer);
        writer.align();
        
        BitReader reader(buffer, 100);
        uint32_t decoded = codecs::EliasDelta::decode<uint32_t>(reader);
        REQUIRE(decoded == value);
    };
    
    SECTION("Edge cases") {
        test_value(0);
        test_value(1);
        test_value(std::numeric_limits<uint16_t>::max());
    }
    
    SECTION("Powers of 2") {
        for (int i = 0; i < 20; ++i) {
            test_value(1u << i);
            test_value((1u << i) - 1);
            test_value((1u << i) + 1);
        }
    }
}

TEST_CASE("Rice Codec", "[codecs]") {
    SECTION("Rice<3>") {
        auto test_value = [](uint32_t value) {
            uint8_t buffer[100] = {0};
            BitWriter writer(buffer);
            codecs::Rice<3>::encode(value, writer);
            writer.align();
            
            BitReader reader(buffer, 100);
            uint32_t decoded = codecs::Rice<3>::decode<uint32_t>(reader);
            REQUIRE(decoded == value);
        };
        
        for (uint32_t i = 0; i < 100; ++i) {
            test_value(i);
        }
    }
    
    SECTION("Different K values") {
        uint32_t test_values[] = {0, 1, 7, 8, 15, 16, 31, 32, 100, 255, 256};
        
        for (auto value : test_values) {
            uint8_t buffer[100] = {0};
            
            // K=1
            BitWriter w1(buffer);
            codecs::Rice<1>::encode(value, w1);
            w1.align();
            BitReader r1(buffer, 100);
            REQUIRE(codecs::Rice<1>::decode<uint32_t>(r1) == value);
            
            // K=4
            BitWriter w4(buffer);
            codecs::Rice<4>::encode(value, w4);
            w4.align();
            BitReader r4(buffer, 100);
            REQUIRE(codecs::Rice<4>::decode<uint32_t>(r4) == value);
        }
    }
}

TEST_CASE("Fibonacci Codec", "[codecs]") {
    auto test_value = [](uint32_t value) {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        codecs::Fibonacci::encode(value, writer);
        writer.align();
        
        BitReader reader(buffer, 100);
        uint32_t decoded = codecs::Fibonacci::decode<uint32_t>(reader);
        REQUIRE(decoded == value);
    };
    
    SECTION("Fibonacci numbers") {
        uint32_t fibs[] = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144};
        for (auto f : fibs) {
            test_value(f);
        }
    }
    
    SECTION("Random values") {
        std::mt19937 gen(42);
        std::uniform_int_distribution<uint32_t> dist(0, 10000);
        
        for (int i = 0; i < 50; ++i) {
            test_value(dist(gen));
        }
    }
}

TEST_CASE("Signed Integer Codec", "[codecs]") {
    auto test_value = [](int32_t value) {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        codecs::SignedGamma::encode(value, writer);
        writer.align();
        
        BitReader reader(buffer, 100);
        int32_t decoded = codecs::SignedGamma::decode<int32_t>(reader);
        REQUIRE(decoded == value);
    };
    
    SECTION("Positive and negative") {
        test_value(0);
        test_value(1);
        test_value(-1);
        test_value(100);
        test_value(-100);
        test_value(std::numeric_limits<int16_t>::max());
        test_value(std::numeric_limits<int16_t>::min());
    }
    
    SECTION("Zigzag pattern") {
        for (int32_t i = -10; i <= 10; ++i) {
            test_value(i);
        }
    }
}

TEST_CASE("Packed Types", "[packed]") {
    SECTION("Basic Packed") {
        PackedU32<> p(42);
        REQUIRE(p.value() == 42);
        
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        PackedU32<>::encode(p, writer);
        writer.align();
        
        BitReader reader(buffer, 100);
        auto decoded = PackedU32<>::decode(reader);
        REQUIRE(decoded.value() == 42);
    }
    
    SECTION("PackedPair") {
        using Rational = PackedPair<PackedU32<>, PackedU32<>>;
        Rational r({22, 7});
        
        REQUIRE(r.first().value() == 22);
        REQUIRE(r.second().value() == 7);
        
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        Rational::encode(r, writer);
        writer.align();
        
        BitReader reader(buffer, 100);
        auto decoded = Rational::decode(reader);
        REQUIRE(decoded.first().value() == 22);
        REQUIRE(decoded.second().value() == 7);
    }
    
    SECTION("PackedOptional") {
        PackedOptional<PackedU32<>> some(42);
        PackedOptional<PackedU32<>> none;
        
        REQUIRE(some.has_value());
        REQUIRE(!none.has_value());
        REQUIRE((*some).value() == 42);
        
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        PackedOptional<PackedU32<>>::encode(some, writer);
        PackedOptional<PackedU32<>>::encode(none, writer);
        writer.align();
        
        BitReader reader(buffer, 100);
        auto decoded_some = PackedOptional<PackedU32<>>::decode(reader);
        auto decoded_none = PackedOptional<PackedU32<>>::decode(reader);
        
        REQUIRE(decoded_some.has_value());
        REQUIRE((*decoded_some).value() == 42);
        REQUIRE(!decoded_none.has_value());
    }
    
    SECTION("PackedVector") {
        PackedVector<PackedU32<>> vec;
        vec.emplace_back(1);
        vec.emplace_back(2);
        vec.emplace_back(3);
        vec.emplace_back(5);
        vec.emplace_back(8);
        
        REQUIRE(vec.size() == 5);
        REQUIRE(vec[0].value() == 1);
        REQUIRE(vec[4].value() == 8);
        
        uint8_t buffer[200] = {0};
        BitWriter writer(buffer);
        PackedVector<PackedU32<>>::encode(vec, writer);
        writer.align();
        
        BitReader reader(buffer, 200);
        auto decoded = PackedVector<PackedU32<>>::decode(reader);
        
        REQUIRE(decoded.size() == 5);
        REQUIRE(decoded[0].value() == 1);
        REQUIRE(decoded[4].value() == 8);
    }
}

TEST_CASE("High-level API", "[api]") {
    SECTION("compress/decompress") {
        std::vector<uint32_t> data = {1, 1, 2, 3, 5, 8, 13, 21, 34, 55};
        
        auto compressed = compress(data);
        auto decompressed = decompress<uint32_t>(compressed);
        
        REQUIRE(data == decompressed);
        REQUIRE(compressed.size() < data.size() * sizeof(uint32_t));
    }
    
    SECTION("Different codecs") {
        std::vector<uint32_t> data;
        for (uint32_t i = 0; i < 100; ++i) {
            data.push_back(i * i);
        }
        
        auto gamma_compressed = compress<uint32_t, codecs::EliasGamma>(data);
        auto delta_compressed = compress<uint32_t, codecs::EliasDelta>(data);
        
        auto gamma_decompressed = decompress<uint32_t, codecs::EliasGamma>(gamma_compressed);
        auto delta_decompressed = decompress<uint32_t, codecs::EliasDelta>(delta_compressed);
        
        REQUIRE(data == gamma_decompressed);
        REQUIRE(data == delta_decompressed);
    }
}

TEST_CASE("Delta Encoding", "[algorithms]") {
    std::vector<uint32_t> sorted_data = {100, 105, 110, 120, 130, 145, 160, 180};
    
    uint8_t buffer[200] = {0};
    BitWriter writer(buffer);
    algorithms::DeltaCodec<uint32_t, codecs::EliasGamma>::encode_deltas(
        sorted_data, writer);
    writer.align();
    
    BitReader reader(buffer, 200);
    auto decoded = algorithms::DeltaCodec<uint32_t, codecs::EliasGamma>::decode_deltas(
        reader, sorted_data.size());
    
    REQUIRE(decoded == sorted_data);
}

TEST_CASE("Compression Analysis", "[algorithms]") {
    SECTION("Geometric distribution") {
        std::mt19937 gen(42);
        std::geometric_distribution<> dist(0.3);
        
        std::vector<uint32_t> data;
        for (int i = 0; i < 1000; ++i) {
            data.push_back(dist(gen));
        }
        
        auto stats = algorithms::analyze_compression<uint32_t, codecs::EliasGamma>(data);
        
        REQUIRE(stats.ratio > 1.0);  // Should compress
        REQUIRE(stats.bits_per_value < 32.0);  // Better than fixed width
    }
}

TEST_CASE("Edge Cases", "[edge]") {
    SECTION("Empty data") {
        std::vector<uint32_t> empty;
        auto compressed = compress(empty);
        auto decompressed = decompress<uint32_t>(compressed);
        REQUIRE(decompressed.empty());
    }
    
    SECTION("Single value") {
        std::vector<uint32_t> single = {42};
        auto compressed = compress(single);
        auto decompressed = decompress<uint32_t>(compressed);
        REQUIRE(decompressed.size() == 1);
        REQUIRE(decompressed[0] == 42);
    }
    
    SECTION("Maximum values") {
        uint32_t max_val = std::numeric_limits<uint32_t>::max();
        std::vector<uint32_t> data = {0, max_val / 2, max_val - 1};
        
        auto compressed = compress<uint32_t, codecs::EliasDelta>(data);
        auto decompressed = decompress<uint32_t, codecs::EliasDelta>(compressed);
        
        REQUIRE(decompressed == data);
    }
}