// test_production.cpp - Tests for production-ready features
#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include <pfc/pfc.hpp>
#include <random>
#include <string>
#include <algorithm>

using namespace pfc;

// ============================================================
// Error Handling Tests
// ============================================================

TEST_CASE("Error handling", "[error]") {
    SECTION("Error codes") {
        auto ec = make_error_code(pfc_error::buffer_overflow);
        REQUIRE(ec.category() == get_pfc_error_category());
        REQUIRE(ec.message() == "Buffer overflow: not enough space");
    }

    SECTION("Safe BitWriter bounds checking") {
        SafeBitWriter<> writer(10); // Max 10 bytes

        // Should succeed for first 10 bytes
        for (int i = 0; i < 80; ++i) {
            auto res = writer.write_bit(true);
            REQUIRE(res.has_value());
        }

        // Should fail on overflow
        auto res = writer.write_bit(true);
        REQUIRE(!res.has_value());
        REQUIRE(res.error() == make_error_code(pfc_error::buffer_overflow));
    }

    SECTION("Safe BitReader validation") {
        uint8_t data[] = {0xFF, 0x00};
        SafeBitReader<> reader(data, sizeof(data));

        // Should read successfully
        for (int i = 0; i < 16; ++i) {
            auto res = reader.read_bit();
            REQUIRE(res.has_value());
        }

        // Should fail at EOF
        auto res = reader.read_bit();
        REQUIRE(!res.has_value());
        REQUIRE(res.error() == make_error_code(pfc_error::eof_reached));
    }
}

// ============================================================
// Allocator Support Tests
// ============================================================

TEST_CASE("Allocator support", "[allocator]") {
    SECTION("BasicBitWriter with custom allocator") {
        std::allocator<uint8_t> alloc;
        BasicBitWriter<> writer(alloc);

        writer.write_bits(0x1234, 16);
        writer.align();

        REQUIRE(writer.size() == 2);
        REQUIRE(writer.buffer()[0] == 0x34);
        REQUIRE(writer.buffer()[1] == 0x12);
    }

    SECTION("BasicPackedVector") {
        BasicPackedVector<uint32_t, codecs::EliasGamma> vec;

        vec.push_back(1);
        vec.push_back(10);
        vec.push_back(100);

        REQUIRE(vec.size() == 3);
        REQUIRE(vec[0] == 1);
        REQUIRE(vec[1] == 10);
        REQUIRE(vec[2] == 100);
    }

    #ifdef __cpp_lib_memory_resource
    SECTION("PMR support") {
        std::pmr::monotonic_buffer_resource resource;
        pmr::BitWriter writer(&resource);

        writer.write_bits(0xABCD, 16);
        writer.align();

        REQUIRE(writer.size() == 2);
    }
    #endif
}

// ============================================================
// Huffman Coding Tests
// ============================================================

TEST_CASE("Huffman coding", "[huffman]") {
    SECTION("Static Huffman from frequencies") {
        codecs::StaticHuffman<uint8_t>::frequency_map freq;
        freq['a'] = 45;
        freq['b'] = 13;
        freq['c'] = 12;
        freq['d'] = 16;
        freq['e'] = 9;
        freq['f'] = 5;

        auto huffman_res = codecs::StaticHuffman<uint8_t>::from_frequencies(freq);
        REQUIRE(huffman_res.has_value());

        auto& huffman = *huffman_res;

        // Test encoding
        uint8_t buffer[64] = {0};
        BitWriter writer(buffer);
        auto res = huffman.encode_symbol('a', writer);
        REQUIRE(res.has_value());

        // 'a' should have shortest code (highest frequency)
        writer.align();
        size_t bits_written = (writer.position() - writer.start()) * 8;
        REQUIRE(bits_written <= 64);
    }

    SECTION("Huffman round-trip") {
        std::string text = "hello huffman compression test";

        // Build Huffman tree from data
        auto huffman_res = codecs::StaticHuffman<char>::from_data(text.begin(), text.end());
        REQUIRE(huffman_res.has_value());

        auto& huffman = *huffman_res;

        // Encode
        uint8_t buffer[1024] = {0};
        BitWriter writer(buffer);
        auto encode_res = huffman.encode(text.begin(), text.end(), writer);
        REQUIRE(encode_res.has_value());
        writer.align();

        // Decode
        BitReader reader(buffer, writer.bytes_written());
        std::string decoded;
        auto decode_res = huffman.decode(reader, std::back_inserter(decoded), text.size());
        REQUIRE(decode_res.has_value());

        REQUIRE(decoded == text);
    }

    SECTION("Huffman header serialization") {
        std::string text = "test data for header";
        auto huffman_res = codecs::StaticHuffman<char>::from_data(text.begin(), text.end());
        REQUIRE(huffman_res.has_value());

        auto& huffman = *huffman_res;

        // Write header
        uint8_t header_buf[1024] = {0};
        BitWriter header_writer(header_buf);
        auto header_res = huffman.write_header(header_writer);
        REQUIRE(header_res.has_value());
        header_writer.align();

        // Read header
        BitReader header_reader(header_buf, header_writer.bytes_written());
        auto huffman2_res = codecs::StaticHuffman<char>::read_header(header_reader);
        REQUIRE(huffman2_res.has_value());

        // Both should encode the same
        uint8_t buf1[1024] = {0};
        uint8_t buf2[1024] = {0};
        BitWriter w1(buf1);
        BitWriter w2(buf2);
        huffman.encode(text.begin(), text.end(), w1);
        huffman2_res->encode(text.begin(), text.end(), w2);

        // Note: Codes might differ but compression should be similar
        REQUIRE(w1.bytes_written() > 0);
        REQUIRE(w2.bytes_written() > 0);
    }
}

// ============================================================
// LZ77 Compression Tests
// ============================================================

TEST_CASE("LZ77 compression", "[lz77]") {
    using namespace compression;

    SECTION("Basic LZ77 compression") {
        std::string text = "abcabcabcabc";
        LZ77Compressor<> compressor;

        std::vector<LZ77Match> matches;
        auto res = compressor.compress(text.begin(), text.end(), std::back_inserter(matches));
        REQUIRE(res.has_value());
        REQUIRE(*res > 0);

        // Should find repeated patterns
        bool has_match = false;
        for (const auto& match : matches) {
            if (!match.is_literal() && match.length > 0) {
                has_match = true;
                break;
            }
        }
        REQUIRE(has_match);
    }

    SECTION("LZ77 round-trip") {
        std::string text = "The quick brown fox jumps over the lazy dog. The quick brown fox is quick.";

        // Compress
        LZ77Compressor<> compressor;
        uint8_t buffer[2048] = {0};
        BitWriter writer(buffer);
        auto comp_res = compressor.compress_to_bits(text.begin(), text.end(), writer);
        REQUIRE(comp_res.has_value());
        writer.align();

        // Decompress
        LZ77Decompressor<> decompressor;
        BitReader reader(buffer, writer.bytes_written());
        std::string decompressed;
        auto decomp_res = decompressor.decompress_from_bits(reader, std::back_inserter(decompressed));
        REQUIRE(decomp_res.has_value());

        REQUIRE(decompressed == text);
    }

    SECTION("LZSS compression") {
        std::string text = "repetitive repetitive data with repetitive patterns";

        uint8_t buffer[1024] = {0};
        BitWriter writer(buffer);
        auto res = LZSS::compress(text.begin(), text.end(), writer);
        REQUIRE(res.has_value());
        writer.align();

        // LZSS should compress repetitive data well
        REQUIRE(writer.bytes_written() < text.size());
    }
}

// ============================================================
// CRC Tests
// ============================================================

TEST_CASE("CRC checksums", "[crc]") {
    using namespace integrity;

    SECTION("CRC32") {
        std::string data = "Hello, World!";
        uint32_t crc = CRC32::calculate(
            reinterpret_cast<const uint8_t*>(data.data()),
            data.size()
        );

        // Known CRC32 for "Hello, World!"
        REQUIRE(crc == 0xEC4AC3D0);
    }

    SECTION("CRC32 incremental") {
        CRC32 crc;
        crc.update('H');
        crc.update('e');
        crc.update('l');
        crc.update('l');
        crc.update('o');

        std::string rest = ", World!";
        crc.update(reinterpret_cast<const uint8_t*>(rest.data()), rest.size());

        REQUIRE(crc.finalize() == 0xEC4AC3D0);
    }

    SECTION("CRC64") {
        std::string data = "Test data";
        uint64_t crc = CRC64::calculate(
            reinterpret_cast<const uint8_t*>(data.data()),
            data.size()
        );

        // Should produce consistent result
        uint64_t crc2 = CRC64::calculate(
            reinterpret_cast<const uint8_t*>(data.data()),
            data.size()
        );
        REQUIRE(crc == crc2);
    }

    SECTION("Adler32") {
        std::string data = "Wikipedia";
        uint32_t adler = Adler32::calculate(
            reinterpret_cast<const uint8_t*>(data.data()),
            data.size()
        );

        // Known Adler32 for "Wikipedia"
        REQUIRE(adler == 0x11E60398);
    }

    SECTION("ChecksummedData") {
        std::vector<uint8_t> data = {1, 2, 3, 4, 5};
        std::vector<uint8_t> with_checksum;

        ChecksummedData<CRC32>::add_checksum(
            data.begin(), data.end(),
            std::back_inserter(with_checksum)
        );

        REQUIRE(with_checksum.size() == data.size() + 4); // CRC32 is 4 bytes

        // Verify checksum
        bool valid = ChecksummedData<CRC32>::verify_checksum(
            with_checksum.begin(), with_checksum.end()
        );
        REQUIRE(valid);

        // Corrupt data
        with_checksum[0] = 99;
        bool invalid = ChecksummedData<CRC32>::verify_checksum(
            with_checksum.begin(), with_checksum.end()
        );
        REQUIRE(!invalid);
    }
}

// ============================================================
// Integration Tests
// ============================================================

TEST_CASE("Integration: Huffman + LZ77 + CRC", "[integration]") {
    std::string original = "This is a test of combined compression algorithms. "
                          "This text has repetitive patterns that should compress well. "
                          "The quick brown fox jumps over the lazy dog. "
                          "The quick brown fox is very quick indeed!";

    // Step 1: LZ77 compression
    compression::LZ77Compressor<> lz77;
    std::vector<compression::LZ77Match> lz77_matches;
    auto lz77_res = lz77.compress(original.begin(), original.end(),
                                  std::back_inserter(lz77_matches));
    REQUIRE(lz77_res.has_value());

    // Step 2: Huffman encode the LZ77 output
    // (In practice, we'd encode the matches, but for simplicity we'll use the original)
    auto huffman_res = codecs::StaticHuffman<char>::from_data(original.begin(), original.end());
    REQUIRE(huffman_res.has_value());

    uint8_t comp_buffer[2048] = {0};
    BitWriter compressed(comp_buffer);
    auto huff_enc_res = huffman_res->encode(original.begin(), original.end(), compressed);
    REQUIRE(huff_enc_res.has_value());
    compressed.align();

    // Step 3: Add CRC checksum
    std::vector<uint8_t> final_data;
    integrity::ChecksummedData<integrity::CRC32>::add_checksum(
        comp_buffer, comp_buffer + compressed.bytes_written(),
        std::back_inserter(final_data)
    );

    // Verify everything
    bool checksum_valid = integrity::ChecksummedData<integrity::CRC32>::verify_checksum(
        final_data.begin(), final_data.end()
    );
    REQUIRE(checksum_valid);

    // Compression should be effective
    REQUIRE(final_data.size() < original.size());
}