// simple_test.cpp - Quick test to verify basic functionality

#include <pfc/pfc.hpp>
#include <pfc/algebraic.hpp>
#include <iostream>

using namespace pfc;

int main() {
    std::cout << "Testing basic packed types...\n";

    // Test basic packed integer
    PackedU32<> val{42};
    std::cout << "PackedU32 value: " << val.value() << "\n";

    // Test Unit type
    Unit unit;
    std::cout << "Unit type created\n";

    // Test simple variant
    using IntOrBool = PackedVariant<PackedU32<>, PackedBool>;
    IntOrBool var1{PackedU32<>{100}};
    IntOrBool var2{PackedBool{true}};

    std::cout << "Variant 1 index: " << var1.index() << "\n";
    std::cout << "Variant 2 index: " << var2.index() << "\n";

    // Test serialization
    std::vector<uint8_t> buffer(128);
    BitWriter writer(buffer.data());

    PackedU32<>::encode(val, writer);
    writer.align();

    BitReader reader(buffer.data(), writer.bytes_written());
    auto decoded = PackedU32<>::decode(reader);

    std::cout << "Encoded and decoded: " << decoded.value() << "\n";

    std::cout << "All basic tests passed!\n";

    return 0;
}