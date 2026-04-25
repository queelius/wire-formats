// mini_demo.cpp - Minimal demonstration of advanced features

#include <pfc/pfc.hpp>
#include <pfc/algebraic.hpp>
#include <pfc/numeric_codecs.hpp>
#include <pfc/stl_integration.hpp>
#include <pfc/coordinates.hpp>

#include <iostream>
#include <iomanip>

using namespace pfc;

int main() {
    std::cout << "====================================\n";
    std::cout << "   Packed Codec Library Demo        \n";
    std::cout << "====================================\n\n";

    // 1. Algebraic Types
    std::cout << "1. Algebraic Types:\n";
    using IntOrBool = PackedVariant<PackedU32<>, PackedBool>;
    IntOrBool var1{PackedU32<>{42}};
    std::cout << "   Variant holds int: " << (var1.index() == 0) << "\n";

    // 2. Numeric Codecs
    std::cout << "\n2. Numeric Codecs:\n";
    using CustomFloat = Packed<double, codecs::FloatingPoint<16, 5>>;
    CustomFloat pi{3.14159};
    std::cout << "   Float value: " << pi.value() << "\n";

    // 3. STL Integration
    std::cout << "\n3. STL Integration:\n";
    PackedContainer<PackedU32<>> container;
    for (uint32_t i = 0; i < 10; ++i) {
        container.push_back(PackedU32<>{i * i});
    }
    std::cout << "   Container size: " << container.size() << "\n";
    std::cout << "   Compression ratio: " << std::fixed << std::setprecision(2)
              << container.compression_ratio() << "x\n";

    // 4. Coordinates
    std::cout << "\n4. Coordinate Structures:\n";
    PackedVec3 v1{1.0, 2.0, 3.0};
    PackedVec3 v2{4.0, 5.0, 6.0};
    auto v3 = v1 + v2;
    std::cout << "   v1 + v2 = (" << v3[0].value() << ", "
              << v3[1].value() << ", " << v3[2].value() << ")\n";

    std::cout << "\n====================================\n";
    std::cout << "        Demo Complete!              \n";
    std::cout << "====================================\n";

    return 0;
}