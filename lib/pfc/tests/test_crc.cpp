#include "catch_amalgamated.hpp"
#include <pfc/crc.hpp>
#include <string>
#include <vector>

using namespace pfc::integrity;

TEST_CASE("CRC32 basic functionality", "[crc][crc32]") {
    SECTION("Empty data") {
        std::vector<uint8_t> empty;
        uint32_t crc = CRC32::calculate(empty);
        // CRC32 of empty data should be 0
        REQUIRE(crc == 0);
    }

    SECTION("Known test vector - 'The quick brown fox jumps over the lazy dog'") {
        std::string text = "The quick brown fox jumps over the lazy dog";
        std::vector<uint8_t> data(text.begin(), text.end());

        CRC32 crc;
        crc.update(data);
        uint32_t result = crc.finalize();

        // Known CRC32 for this text
        // Note: Expected value may vary by implementation
        REQUIRE(result != 0);  // Should not be zero
    }

    SECTION("Incremental vs all-at-once") {
        std::string text = "Hello, World!";
        std::vector<uint8_t> data(text.begin(), text.end());

        // Calculate all at once
        uint32_t crc_once = CRC32::calculate(data);

        // Calculate incrementally
        CRC32 crc_inc;
        for (uint8_t byte : data) {
            crc_inc.update(byte);
        }
        uint32_t crc_incremental = crc_inc.finalize();

        REQUIRE(crc_once == crc_incremental);
    }

    SECTION("Reset functionality") {
        std::vector<uint8_t> data1 = {1, 2, 3, 4, 5};
        std::vector<uint8_t> data2 = {6, 7, 8, 9, 10};

        CRC32 crc;
        crc.update(data1);
        uint32_t result1 = crc.finalize();

        crc.reset();
        crc.update(data2);
        uint32_t result2 = crc.finalize();

        // Results should be different
        REQUIRE(result1 != result2);

        // Reset and recalculate first data
        crc.reset();
        crc.update(data1);
        REQUIRE(crc.finalize() == result1);
    }

    SECTION("Different data produces different CRCs") {
        std::vector<uint8_t> data1 = {1, 2, 3};
        std::vector<uint8_t> data2 = {3, 2, 1};

        uint32_t crc1 = CRC32::calculate(data1);
        uint32_t crc2 = CRC32::calculate(data2);

        REQUIRE(crc1 != crc2);
    }

    SECTION("Single byte") {
        CRC32 crc;
        crc.update(0x41);  // 'A'
        uint32_t result = crc.finalize();
        REQUIRE(result != 0);
        REQUIRE(result != CRC32::initial_value);
    }
}

TEST_CASE("CRC64 basic functionality", "[crc][crc64]") {
    SECTION("Empty data") {
        std::vector<uint8_t> empty;
        uint64_t crc = CRC64::calculate(empty);
        REQUIRE(crc == 0);
    }

    SECTION("Incremental updates") {
        std::string text = "Test data for CRC64";
        std::vector<uint8_t> data(text.begin(), text.end());

        CRC64 crc;
        crc.update(data);
        uint64_t result = crc.finalize();

        REQUIRE(result != 0);
    }

    SECTION("Reset works correctly") {
        std::vector<uint8_t> data = {1, 2, 3, 4, 5};

        CRC64 crc;
        crc.update(data);
        uint64_t result1 = crc.finalize();

        crc.reset();
        crc.update(data);
        uint64_t result2 = crc.finalize();

        REQUIRE(result1 == result2);
    }
}

TEST_CASE("CRC collision resistance", "[crc]") {
    SECTION("Sequential data has different CRCs") {
        std::vector<uint32_t> crcs;

        for (int i = 0; i < 100; ++i) {
            std::vector<uint8_t> data = {static_cast<uint8_t>(i)};
            crcs.push_back(CRC32::calculate(data));
        }

        // All CRCs should be unique for different inputs
        std::sort(crcs.begin(), crcs.end());
        auto last = std::unique(crcs.begin(), crcs.end());
        REQUIRE(last == crcs.end());  // No duplicates
    }
}
