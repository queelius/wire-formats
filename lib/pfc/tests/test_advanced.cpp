#include <pfc/pfc.hpp>
#include <pfc/algebraic.hpp>
#include <pfc/numeric_codecs.hpp>
#include <pfc/stl_integration.hpp>
#include <pfc/coordinates.hpp>
#include "catch_amalgamated.hpp"

#include <random>
#include <numeric>
#include <execution>

using namespace pfc;

// ============================================================
//  Algebraic Types Tests
// ============================================================

TEST_CASE("Unit type", "[algebraic]") {
    Unit unit1, unit2;

    SECTION("Equality") {
        REQUIRE(unit1 == unit2);
    }

    SECTION("Serialization") {
        uint8_t buffer[10] = {0};
        BitWriter writer(buffer);
        Unit::encode(unit1, writer);
        writer.align();

        REQUIRE(writer.bytes_written() == 0);  // Unit encodes to nothing

        BitReader reader(buffer, 10);
        auto decoded = Unit::decode(reader);
        REQUIRE(decoded == unit1);
    }
}

TEST_CASE("PackedVariant (Sum types)", "[algebraic]") {
    using IntOrString = PackedVariant<
        PackedU32<>,
        Packed<bool, codecs::Boolean>
    >;

    SECTION("Construction and access") {
        IntOrString var1{PackedU32<>{42}};
        IntOrString var2{Packed<bool, codecs::Boolean>{true}};

        REQUIRE(var1.index() == 0);
        REQUIRE(var2.index() == 1);

        REQUIRE(var1.get_if<PackedU32<>>() != nullptr);
        REQUIRE(var1.get_if<PackedU32<>>()->value() == 42);
        REQUIRE(var2.get_if<Packed<bool, codecs::Boolean>>() != nullptr);
        REQUIRE(var2.get_if<Packed<bool, codecs::Boolean>>()->value() == true);
    }

    SECTION("Visitor pattern") {
        IntOrString var{PackedU32<>{123}};

        bool visited = false;
        var.visit([&visited](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, PackedU32<>>) {
                REQUIRE(val.value() == 123);
                visited = true;
            }
        });
        REQUIRE(visited);
    }

    SECTION("Serialization") {
        std::vector<IntOrString> values = {
            IntOrString{PackedU32<>{100}},
            IntOrString{Packed<bool, codecs::Boolean>{false}},
            IntOrString{PackedU32<>{200}},
            IntOrString{Packed<bool, codecs::Boolean>{true}}
        };

        uint8_t buffer[256] = {0};
        BitWriter writer(buffer);

        for (const auto& val : values) {
            IntOrString::encode(val, writer);
        }
        writer.align();

        BitReader reader(buffer, writer.bytes_written());

        for (size_t i = 0; i < values.size(); ++i) {
            auto decoded = IntOrString::decode(reader);
            REQUIRE(decoded.index() == values[i].index());

            if (decoded.index() == 0) {
                REQUIRE(decoded.get_if<PackedU32<>>()->value() ==
                       values[i].get_if<PackedU32<>>()->value());
            } else {
                REQUIRE(decoded.get_if<Packed<bool, codecs::Boolean>>()->value() ==
                       values[i].get_if<Packed<bool, codecs::Boolean>>()->value());
            }
        }
    }
}

TEST_CASE("PackedList (Recursive type)", "[algebraic]") {
    using IntList = PackedList<PackedU32<>>;

    SECTION("Construction from vector") {
        std::vector<uint32_t> vec = {1, 2, 3, 4, 5};
        IntList list{vec};

        auto decoded = list.value();
        REQUIRE(decoded == vec);
    }

    SECTION("Map operation") {
        std::vector<uint32_t> vec = {1, 2, 3};
        IntList list{vec};

        auto doubled = list.map([](uint32_t x) { return x * 2; });
        auto result = doubled.value();

        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == 2);
        REQUIRE(result[1] == 4);
        REQUIRE(result[2] == 6);
    }

    SECTION("Empty list") {
        IntList empty;
        REQUIRE(empty.empty());
        REQUIRE(empty.value().empty());
    }
}

// ============================================================
//  Numeric Codecs Tests
// ============================================================

TEST_CASE("FloatingPoint codec", "[numeric]") {
    using Float16 = codecs::FloatingPoint<10, 5>;

    auto test_float = [](double value) {
        uint8_t buffer[16] = {0};
        BitWriter writer(buffer);
        Float16::encode(value, writer);
        writer.align();

        BitReader reader(buffer, 16);
        double decoded = Float16::decode(reader);

        // Check relative error for normal numbers
        if (std::isfinite(value) && value != 0.0) {
            double rel_error = std::abs((decoded - value) / value);
            REQUIRE(rel_error < 0.01);  // 1% error for half precision
        }
        // Check special values
        else if (std::isnan(value)) {
            REQUIRE(std::isnan(decoded));
        }
        else if (std::isinf(value)) {
            REQUIRE(std::isinf(decoded));
            REQUIRE(std::signbit(value) == std::signbit(decoded));
        }
        else if (value == 0.0) {
            REQUIRE(decoded == 0.0);
            REQUIRE(std::signbit(value) == std::signbit(decoded));
        }
    };

    SECTION("Normal values") {
        test_float(3.14159);
        test_float(-2.71828);
        test_float(1.0);
        test_float(-1.0);
    }

    SECTION("Special values") {
        test_float(0.0);
        test_float(-0.0);
        test_float(std::numeric_limits<double>::infinity());
        test_float(-std::numeric_limits<double>::infinity());
        test_float(std::numeric_limits<double>::quiet_NaN());
    }

    SECTION("Edge cases") {
        // Use Float16-appropriate values (max ~65,504, min normal ~6.1e-5)
        test_float(65000.0);    // Near Float16 max
        test_float(0.0001);     // Near Float16 min normal
        test_float(0.001);      // Near Float16 epsilon
        test_float(100.0);
        test_float(0.5);
    }
}

TEST_CASE("FixedDecimal codec", "[numeric]") {
    using Money = codecs::FixedDecimal<10, 2>;

    auto test_money = [](double value) {
        uint8_t buffer[16] = {0};
        BitWriter writer(buffer);
        Money::encode(value, writer);
        writer.align();

        BitReader reader(buffer, 16);
        double decoded = Money::decode(reader);

        // Money should be exact to 2 decimal places
        double rounded = std::round(value * 100) / 100;
        REQUIRE(std::abs(decoded - rounded) < 1e-10);
    };

    SECTION("Typical amounts") {
        test_money(19.99);
        test_money(100.00);
        test_money(0.01);
        test_money(-50.50);
        test_money(1234.56);
    }

    SECTION("Rounding") {
        test_money(19.994);  // Should round to 19.99
        test_money(19.995);  // Should round to 20.00
        test_money(19.996);  // Should round to 20.00
    }
}

TEST_CASE("Rational codec", "[numeric]") {
    using Rational = codecs::Rational<>;

    SECTION("Simple fractions") {
        auto test = [](double value) {
            uint8_t buffer[32] = {0};
            BitWriter writer(buffer);
            Rational::encode(value, writer);
            writer.align();

            BitReader reader(buffer, 32);
            double decoded = Rational::decode(reader);

            REQUIRE(std::abs(decoded - value) < 1e-6);
        };

        test(0.5);      // 1/2
        test(0.333333); // ~1/3
        test(0.25);     // 1/4
        test(0.125);    // 1/8
        test(2.5);      // 5/2
    }

    SECTION("Fraction representation") {
        uint8_t buffer[32] = {0};
        BitWriter writer(buffer);

        auto frac = Rational::Fraction::from_double(0.333333);
        Rational::encode_fraction(frac, writer);
        writer.align();

        BitReader reader(buffer, 32);
        auto decoded = Rational::decode_fraction(reader);

        // Should approximate 1/3
        REQUIRE(std::abs(decoded.to_double() - 1.0/3.0) < 1e-6);
    }
}

// ============================================================
//  STL Integration Tests
// ============================================================

TEST_CASE("PackedContainer with STL algorithms", "[stl]") {
    using PackedInt = PackedU32<codecs::EliasGamma>;
    PackedContainer<PackedInt> container;

    // Fill with test data
    for (uint32_t i = 0; i < 100; ++i) {
        container.push_back(PackedInt{i * 2});
    }

    SECTION("Size and access") {
        REQUIRE(container.size() == 100);
        REQUIRE(container[0] == 0);
        REQUIRE(container[50] == 100);
        REQUIRE(container[99] == 198);
    }

    SECTION("Iterator operations") {
        auto it = container.begin();
        REQUIRE(PackedInt::value_type(*it) == 0);

        ++it;
        REQUIRE(PackedInt::value_type(*it) == 2);

        it += 10;
        REQUIRE(PackedInt::value_type(*it) == 22);
    }

    SECTION("STL find_if") {
        auto it = std::find_if(container.begin(), container.end(),
                               [](const auto& proxy) {
                                   return PackedInt::value_type(proxy) == 100;
                               });

        REQUIRE(it != container.end());
        REQUIRE(std::distance(container.begin(), it) == 50);
    }

    SECTION("STL accumulate") {
        auto sum = std::accumulate(container.begin(), container.end(),
                                   uint64_t(0),
                                   [](uint64_t acc, const auto& proxy) {
                                       return acc + PackedInt::value_type(proxy);
                                   });

        // Sum of 0, 2, 4, ..., 198 = 2 * (0 + 1 + ... + 99) = 2 * 4950 = 9900
        REQUIRE(sum == 9900);
    }

    SECTION("Compression ratio") {
        double ratio = container.compression_ratio();
        REQUIRE(ratio > 1.0);  // Should have some compression
    }
}

TEST_CASE("Zero-copy algorithms", "[stl]") {
    using PackedInt = PackedU32<codecs::EliasGamma>;
    PackedContainer<PackedInt> container;

    for (uint32_t i = 0; i < 50; ++i) {
        container.push_back(PackedInt{i});
    }

    SECTION("packed_transform") {
        auto doubled = algorithms::packed_transform(container,
                                                   [](uint32_t x) { return x * 2; });

        REQUIRE(doubled.size() == container.size());
        REQUIRE(doubled[0] == 0);
        REQUIRE(doubled[25] == 50);
        REQUIRE(doubled[49] == 98);
    }

    SECTION("packed_accumulate") {
        auto sum = algorithms::packed_accumulate(container,
                                                uint32_t(0),
                                                std::plus<>{});
        REQUIRE(sum == 1225);  // Sum of 0..49
    }

    SECTION("packed_find_if") {
        auto result = algorithms::packed_find_if(container,
                                                [](uint32_t x) { return x == 25; });
        REQUIRE(result.has_value());
        REQUIRE(*result == 25);

        auto not_found = algorithms::packed_find_if(container,
                                                   [](uint32_t x) { return x > 100; });
        REQUIRE(!not_found.has_value());
    }
}

// ============================================================
//  Coordinate Structures Tests
// ============================================================

TEST_CASE("PackedVec operations", "[coordinates]") {
    PackedVec3 v1{1.0, 2.0, 3.0};
    PackedVec3 v2{4.0, 5.0, 6.0};

    SECTION("Addition and subtraction") {
        auto sum = v1 + v2;
        REQUIRE(sum[0].value() == Catch::Approx(5.0));
        REQUIRE(sum[1].value() == Catch::Approx(7.0));
        REQUIRE(sum[2].value() == Catch::Approx(9.0));

        auto diff = v2 - v1;
        REQUIRE(diff[0].value() == Catch::Approx(3.0));
        REQUIRE(diff[1].value() == Catch::Approx(3.0));
        REQUIRE(diff[2].value() == Catch::Approx(3.0));
    }

    SECTION("Scalar operations") {
        auto scaled = v1 * 2.0;
        REQUIRE(scaled[0].value() == Catch::Approx(2.0));
        REQUIRE(scaled[1].value() == Catch::Approx(4.0));
        REQUIRE(scaled[2].value() == Catch::Approx(6.0));

        auto divided = v1 / 2.0;
        REQUIRE(divided[0].value() == Catch::Approx(0.5));
        REQUIRE(divided[1].value() == Catch::Approx(1.0));
        REQUIRE(divided[2].value() == Catch::Approx(1.5));
    }

    SECTION("Dot product") {
        double dot = v1.dot(v2);
        REQUIRE(dot == Catch::Approx(32.0));  // 1*4 + 2*5 + 3*6
    }

    SECTION("Cross product") {
        auto cross = v1.cross(v2);
        REQUIRE(cross[0].value() == Catch::Approx(-3.0));  // 2*6 - 3*5
        REQUIRE(cross[1].value() == Catch::Approx(6.0));   // 3*4 - 1*6
        REQUIRE(cross[2].value() == Catch::Approx(-3.0));  // 1*5 - 2*4
    }

    SECTION("Magnitude") {
        double mag = v1.magnitude();
        REQUIRE(mag == Catch::Approx(std::sqrt(14.0)));

        auto normalized = v1.normalized();
        REQUIRE(normalized.magnitude() == Catch::Approx(1.0));
    }
}

TEST_CASE("PackedMatrix operations", "[coordinates]") {
    std::array<std::array<double, 2>, 2> v1 = {{{1.0, 2.0}, {3.0, 4.0}}};
    std::array<std::array<double, 2>, 2> v2 = {{{5.0, 6.0}, {7.0, 8.0}}};
    PackedMatrix2x2 m1(v1);
    PackedMatrix2x2 m2(v2);

    SECTION("Matrix addition") {
        auto sum = m1 + m2;
        REQUIRE(sum(0, 0).value() == Catch::Approx(6.0));
        REQUIRE(sum(0, 1).value() == Catch::Approx(8.0));
        REQUIRE(sum(1, 0).value() == Catch::Approx(10.0));
        REQUIRE(sum(1, 1).value() == Catch::Approx(12.0));
    }

    SECTION("Matrix multiplication") {
        auto prod = m1 * m2;
        REQUIRE(prod(0, 0).value() == Catch::Approx(19.0));  // 1*5 + 2*7
        REQUIRE(prod(0, 1).value() == Catch::Approx(22.0));  // 1*6 + 2*8
        REQUIRE(prod(1, 0).value() == Catch::Approx(43.0));  // 3*5 + 4*7
        REQUIRE(prod(1, 1).value() == Catch::Approx(50.0));  // 3*6 + 4*8
    }

    SECTION("Determinant") {
        double det = m1.determinant();
        REQUIRE(det == Catch::Approx(-2.0));  // 1*4 - 2*3
    }

    SECTION("Identity matrix") {
        auto id = PackedMatrix2x2::identity();
        auto result = m1 * id;

        REQUIRE(result(0, 0).value() == Catch::Approx(m1(0, 0).value()));
        REQUIRE(result(0, 1).value() == Catch::Approx(m1(0, 1).value()));
        REQUIRE(result(1, 0).value() == Catch::Approx(m1(1, 0).value()));
        REQUIRE(result(1, 1).value() == Catch::Approx(m1(1, 1).value()));
    }
}

TEST_CASE("Affine transformations", "[coordinates]") {
    PackedVec<2> point{1.0, 0.0};

    SECTION("Translation") {
        auto translate = PackedTransform2D::translation(PackedVec<2>{2.0, 3.0});
        auto result = translate.transform_point(point);

        REQUIRE(result[0].value() == Catch::Approx(3.0));
        REQUIRE(result[1].value() == Catch::Approx(3.0));
    }

    SECTION("Scaling") {
        auto scale = PackedTransform2D::scale(2.0);
        auto result = scale.transform_point(point);

        REQUIRE(result[0].value() == Catch::Approx(2.0));
        REQUIRE(result[1].value() == Catch::Approx(0.0));
    }

    SECTION("Rotation") {
        auto rotate = PackedTransform2D::rotation_2d(M_PI / 2);  // 90 degrees
        auto result = rotate.transform_point(point);

        REQUIRE(result[0].value() == Catch::Approx(0.0).margin(1e-10));
        REQUIRE(result[1].value() == Catch::Approx(1.0));
    }

    SECTION("Composition") {
        auto scale = PackedTransform2D::scale(2.0);
        auto rotate = PackedTransform2D::rotation_2d(M_PI / 2);
        auto translate = PackedTransform2D::translation(PackedVec<2>{1.0, 1.0});

        auto combined = translate * rotate * scale;
        auto result = combined.transform_point(point);

        // scale by 2 -> (2, 0)
        // rotate 90° -> (0, 2)
        // translate -> (1, 3)
        REQUIRE(result[0].value() == Catch::Approx(1.0).margin(1e-10));
        REQUIRE(result[1].value() == Catch::Approx(3.0));
    }
}