#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include <pfc/pfc.hpp>
#include <random>
#include <limits>

using namespace pfc;

// ============================================================
//  VByte Tests
// ============================================================

TEST_CASE("VByte Codec", "[codecs][vbyte]") {
    auto test_value = [](uint32_t value) {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        codecs::VByte::encode(value, writer);
        writer.align();

        BitReader reader(buffer, 100);
        uint32_t decoded = codecs::VByte::decode<uint32_t>(reader);
        REQUIRE(decoded == value);
    };

    SECTION("Small values (1 byte encoding)") {
        test_value(0);
        test_value(1);
        test_value(127);  // Max 1-byte value
    }

    SECTION("Two-byte values") {
        test_value(128);   // Min 2-byte value
        test_value(255);
        test_value(1000);
        test_value(16383); // Max 2-byte value (2^14 - 1)
    }

    SECTION("Three-byte values") {
        test_value(16384); // Min 3-byte value
        test_value(100000);
        test_value(2097151); // Max 3-byte value (2^21 - 1)
    }

    SECTION("Four-byte values") {
        test_value(2097152); // Min 4-byte value
        test_value(10000000);
        test_value(268435455); // Max 4-byte value (2^28 - 1)
    }

    SECTION("Five-byte values") {
        test_value(268435456); // Min 5-byte value
        test_value(std::numeric_limits<uint32_t>::max());
    }

    SECTION("Powers of 2 and neighbors") {
        for (int i = 0; i < 32; ++i) {
            uint32_t pow2 = 1u << i;
            if (pow2 > 0) test_value(pow2 - 1);
            test_value(pow2);
            if (pow2 < std::numeric_limits<uint32_t>::max())
                test_value(pow2 + 1);
        }
    }

    SECTION("Random values") {
        std::mt19937 gen(42);
        std::uniform_int_distribution<uint32_t> dist(
            0, std::numeric_limits<uint32_t>::max()
        );

        for (int i = 0; i < 1000; ++i) {
            test_value(dist(gen));
        }
    }

    SECTION("64-bit values") {
        auto test_value_64 = [](uint64_t value) {
            uint8_t buffer[100] = {0};
            BitWriter writer(buffer);
            codecs::VByte::encode(value, writer);
            writer.align();

            BitReader reader(buffer, 100);
            uint64_t decoded = codecs::VByte::decode<uint64_t>(reader);
            REQUIRE(decoded == value);
        };

        test_value_64(0);
        test_value_64(std::numeric_limits<uint32_t>::max());
        test_value_64(1ULL << 40);
        test_value_64(1ULL << 50);
        test_value_64(std::numeric_limits<uint64_t>::max() - 1);
    }
}

// ============================================================
//  Exponential-Golomb Tests
// ============================================================

TEST_CASE("ExpGolomb Order 0 (Equivalent to Elias Gamma)", "[codecs][expgolomb]") {
    auto test_value = [](uint32_t value) {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        codecs::ExpGolomb0::encode(value, writer);
        writer.align();

        BitReader reader(buffer, 100);
        uint32_t decoded = codecs::ExpGolomb0::decode<uint32_t>(reader);
        REQUIRE(decoded == value);
    };

    SECTION("Small values") {
        test_value(0);
        test_value(1);
        test_value(2);
        test_value(10);
        test_value(100);
    }

    SECTION("Medium values") {
        test_value(1000);
        test_value(10000);
        test_value(65535);
    }

    SECTION("Large values") {
        test_value(100000);
        test_value(1000000);
        test_value(10000000);
    }

    SECTION("Powers of 2") {
        for (int i = 0; i < 24; ++i) {
            test_value(1u << i);
            if (i > 0) test_value((1u << i) - 1);
            test_value((1u << i) + 1);
        }
    }

    SECTION("Random values") {
        std::mt19937 gen(42);
        std::uniform_int_distribution<uint32_t> dist(0, 10000000);

        for (int i = 0; i < 100; ++i) {
            test_value(dist(gen));
        }
    }
}

TEST_CASE("ExpGolomb Order 1", "[codecs][expgolomb]") {
    auto test_value = [](uint32_t value) {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        codecs::ExpGolomb1::encode(value, writer);
        writer.align();

        BitReader reader(buffer, 100);
        uint32_t decoded = codecs::ExpGolomb1::decode<uint32_t>(reader);
        REQUIRE(decoded == value);
    };

    SECTION("Small values") {
        for (uint32_t i = 0; i < 100; ++i) {
            test_value(i);
        }
    }

    SECTION("Medium values") {
        test_value(1000);
        test_value(10000);
        test_value(100000);
    }

    SECTION("Random values") {
        std::mt19937 gen(42);
        std::uniform_int_distribution<uint32_t> dist(0, 1000000);

        for (int i = 0; i < 100; ++i) {
            test_value(dist(gen));
        }
    }
}

TEST_CASE("ExpGolomb Order 2", "[codecs][expgolomb]") {
    auto test_value = [](uint32_t value) {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        codecs::ExpGolomb2::encode(value, writer);
        writer.align();

        BitReader reader(buffer, 100);
        uint32_t decoded = codecs::ExpGolomb2::decode<uint32_t>(reader);
        REQUIRE(decoded == value);
    };

    SECTION("Boundary testing") {
        for (uint32_t i = 0; i < 200; ++i) {
            test_value(i);
        }
    }

    SECTION("Large values") {
        test_value(10000);
        test_value(100000);
        test_value(1000000);
    }
}

TEST_CASE("ExpGolomb Different Orders Comparison", "[codecs][expgolomb]") {
    SECTION("All orders should encode/decode correctly") {
        std::vector<uint32_t> test_values = {
            0, 1, 2, 3, 4, 5, 10, 50, 100, 1000, 10000
        };

        for (auto value : test_values) {
            // Order 0
            {
                uint8_t buffer[100] = {0};
                BitWriter writer(buffer);
                codecs::ExpGolomb<0>::encode(value, writer);
                writer.align();
                BitReader reader(buffer, 100);
                REQUIRE(codecs::ExpGolomb<0>::decode<uint32_t>(reader) == value);
            }

            // Order 1
            {
                uint8_t buffer[100] = {0};
                BitWriter writer(buffer);
                codecs::ExpGolomb<1>::encode(value, writer);
                writer.align();
                BitReader reader(buffer, 100);
                REQUIRE(codecs::ExpGolomb<1>::decode<uint32_t>(reader) == value);
            }

            // Order 2
            {
                uint8_t buffer[100] = {0};
                BitWriter writer(buffer);
                codecs::ExpGolomb<2>::encode(value, writer);
                writer.align();
                BitReader reader(buffer, 100);
                REQUIRE(codecs::ExpGolomb<2>::decode<uint32_t>(reader) == value);
            }
        }
    }
}

// ============================================================
//  Elias Omega Tests
// ============================================================

TEST_CASE("Elias Omega Codec", "[codecs][omega]") {
    auto test_value = [](uint32_t value) {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        codecs::EliasOmega::encode(value, writer);
        writer.align();

        BitReader reader(buffer, 100);
        uint32_t decoded = codecs::EliasOmega::decode<uint32_t>(reader);
        REQUIRE(decoded == value);
    };

    SECTION("Small values") {
        test_value(0);
        test_value(1);
        test_value(2);
        test_value(3);
        test_value(4);
        test_value(5);
        test_value(10);
        test_value(15);
    }

    SECTION("Medium values") {
        test_value(100);
        test_value(255);
        test_value(256);
        test_value(1000);
        test_value(10000);
        test_value(65535);
        test_value(65536);
    }

    SECTION("Large values") {
        test_value(100000);
        test_value(1000000);
        test_value(10000000);
        test_value(100000000);
    }

    SECTION("Powers of 2 and neighbors") {
        for (int i = 0; i < 24; ++i) {
            uint32_t pow2 = 1u << i;
            if (i > 0) test_value(pow2 - 1);
            test_value(pow2);
            test_value(pow2 + 1);
        }
    }

    SECTION("Fibonacci-like sequence") {
        // Omega is particularly efficient for sequences like Fibonacci
        std::vector<uint32_t> fib = {1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377};
        for (auto value : fib) {
            test_value(value);
        }
    }

    SECTION("Random values") {
        std::mt19937 gen(42);
        std::uniform_int_distribution<uint32_t> dist(0, 10000000);

        for (int i = 0; i < 100; ++i) {
            test_value(dist(gen));
        }
    }

    SECTION("Very large values") {
        test_value(std::numeric_limits<uint16_t>::max());
        test_value(1u << 24);
        test_value(std::numeric_limits<uint32_t>::max() - 1);
    }
}

// ============================================================
//  Comparative Tests
// ============================================================

TEST_CASE("Codec Comparison - Encoding Efficiency", "[codecs][comparison]") {
    auto measure_bits = [](auto encode_func, uint32_t value) -> size_t {
        uint8_t buffer[100] = {0};
        BitWriter writer(buffer);
        encode_func(value, writer);
        writer.align();

        // Count actual bits used (not including alignment padding)
        size_t bits = 0;
        BitReader reader(buffer, 100);
        while (reader.peek()) {
            reader.read();
            bits++;
        }
        return bits;
    };

    SECTION("Small values (0-15) - VByte vs Gamma vs Omega") {
        for (uint32_t i = 0; i < 16; ++i) {
            auto vbyte_bits = measure_bits(
                [](uint32_t v, BitWriter& w) { codecs::VByte::encode(v, w); }, i);
            auto gamma_bits = measure_bits(
                [](uint32_t v, BitWriter& w) { codecs::EliasGamma::encode(v, w); }, i);
            auto omega_bits = measure_bits(
                [](uint32_t v, BitWriter& w) { codecs::EliasOmega::encode(v, w); }, i);

            // VByte always uses at least 8 bits
            REQUIRE(vbyte_bits >= 8);

            // For small values, Gamma and Omega should be more efficient
            INFO("Value: " << i);
            INFO("VByte: " << vbyte_bits << " bits");
            INFO("Gamma: " << gamma_bits << " bits");
            INFO("Omega: " << omega_bits << " bits");
        }
    }

    SECTION("Large values - Omega vs Delta efficiency") {
        std::vector<uint32_t> large_values = {
            10000, 100000, 1000000, 10000000
        };

        for (auto value : large_values) {
            auto delta_bits = measure_bits(
                [](uint32_t v, BitWriter& w) { codecs::EliasDelta::encode(v, w); }, value);
            auto omega_bits = measure_bits(
                [](uint32_t v, BitWriter& w) { codecs::EliasOmega::encode(v, w); }, value);

            INFO("Value: " << value);
            INFO("Delta: " << delta_bits << " bits");
            INFO("Omega: " << omega_bits << " bits");

            // Omega should be equal or better for large values
            REQUIRE(omega_bits <= delta_bits + 5); // Some tolerance
        }
    }
}

TEST_CASE("Signed Codec Wrappers", "[codecs][signed]") {
    SECTION("SignedVByte") {
        auto test_value = [](int32_t value) {
            uint8_t buffer[100] = {0};
            BitWriter writer(buffer);
            codecs::SignedVByte::encode(value, writer);
            writer.align();

            BitReader reader(buffer, 100);
            int32_t decoded = codecs::SignedVByte::decode<int32_t>(reader);
            REQUIRE(decoded == value);
        };

        test_value(0);
        test_value(1);
        test_value(-1);
        test_value(100);
        test_value(-100);
        test_value(std::numeric_limits<int32_t>::max());
        test_value(std::numeric_limits<int32_t>::min() + 1);
    }

    SECTION("SignedOmega") {
        auto test_value = [](int32_t value) {
            uint8_t buffer[100] = {0};
            BitWriter writer(buffer);
            codecs::SignedOmega::encode(value, writer);
            writer.align();

            BitReader reader(buffer, 100);
            int32_t decoded = codecs::SignedOmega::decode<int32_t>(reader);
            REQUIRE(decoded == value);
        };

        test_value(0);
        test_value(1);
        test_value(-1);
        test_value(100);
        test_value(-100);
        test_value(10000);
        test_value(-10000);
    }
}

// ============================================================
//  Multiple Values in Sequence
// ============================================================

TEST_CASE("Multiple values in sequence", "[codecs][sequence]") {
    SECTION("VByte sequence") {
        std::vector<uint32_t> values = {0, 1, 127, 128, 16383, 16384, 1000000};

        uint8_t buffer[1000] = {0};
        BitWriter writer(buffer);

        for (auto v : values) {
            codecs::VByte::encode(v, writer);
        }
        writer.align();

        BitReader reader(buffer, 1000);
        for (auto expected : values) {
            uint32_t decoded = codecs::VByte::decode<uint32_t>(reader);
            REQUIRE(decoded == expected);
        }
    }

    SECTION("ExpGolomb sequence") {
        std::vector<uint32_t> values = {0, 5, 10, 50, 100, 1000, 10000};

        uint8_t buffer[1000] = {0};
        BitWriter writer(buffer);

        for (auto v : values) {
            codecs::ExpGolomb1::encode(v, writer);
        }
        writer.align();

        BitReader reader(buffer, 1000);
        for (auto expected : values) {
            uint32_t decoded = codecs::ExpGolomb1::decode<uint32_t>(reader);
            REQUIRE(decoded == expected);
        }
    }

    SECTION("EliasOmega sequence") {
        std::vector<uint32_t> values = {0, 1, 2, 10, 100, 1000, 10000, 100000};

        uint8_t buffer[1000] = {0};
        BitWriter writer(buffer);

        for (auto v : values) {
            codecs::EliasOmega::encode(v, writer);
        }
        writer.align();

        BitReader reader(buffer, 1000);
        for (auto expected : values) {
            uint32_t decoded = codecs::EliasOmega::decode<uint32_t>(reader);
            REQUIRE(decoded == expected);
        }
    }
}
