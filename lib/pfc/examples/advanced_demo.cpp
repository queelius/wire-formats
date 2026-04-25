// advanced_demo.cpp - Demonstrating the full power of the packed codec library
// Where algebraic types, STL integration, and elegant APIs converge

#include <pfc/pfc.hpp>
#include <pfc/algebraic.hpp>
#include <pfc/numeric_codecs.hpp>
#include <pfc/stl_integration.hpp>
#include <pfc/coordinates.hpp>

#include <iostream>
#include <iomanip>
#include <chrono>
#include <numeric>
#include <random>
#include <execution>

using namespace pfc;

// ============================================================
//  Demonstration 1: Algebraic Types in Action
// ============================================================

void demo_algebraic_types() {
    std::cout << "\n=== Algebraic Types Demo ===\n\n";

    // Sum types for error handling
    // Use a simple error code instead of string for now
    using ErrorCode = PackedU32<codecs::Fixed<8>>;
    using Result = PackedResult<PackedU32<>, ErrorCode>;

    auto compute = [](int x) -> Result {
        if (x < 0) {
            return Result{ErrorCode{1}};  // Error code 1 = negative input
        }
        return Result{PackedU32<>{static_cast<uint32_t>(x * 2)}};
    };

    // Process results
    std::vector<int> inputs = {5, -3, 10, -1, 20};
    std::vector<uint8_t> buffer(1024);
    BitWriter writer(buffer.data());

    for (int input : inputs) {
        auto result = compute(input);
        Result::encode(result, writer);
    }
    writer.align();

    // Decode and process
    BitReader reader(buffer.data(), writer.bytes_written());

    std::cout << "Processing results:\n";
    for (int input : inputs) {
        auto result = Result::decode(reader);

        std::cout << "  Input " << input << ": ";
        result.visit([](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, PackedU32<>>) {
                std::cout << "Success -> " << value.value() << "\n";
            } else {
                std::cout << "Error code -> " << value.value() << "\n";
            }
        });
    }

    // Recursive list structure
    std::cout << "\nRecursive List:\n";
    using IntList = PackedList<PackedU32<>>;

    std::vector<uint32_t> numbers = {1, 2, 3, 4, 5};
    IntList list{numbers};

    std::cout << "  List created with " << numbers.size() << " elements\n";

    // Simple traversal
    std::cout << "  Values: ";
    auto vals = list.value();
    for (auto v : vals) {
        std::cout << v << " ";
    }
    std::cout << "\n";

    // Binary tree
    std::cout << "\nBinary Tree:\n";
    using IntTree = PackedTree<PackedI32<>>;

    // Create a simple tree
    auto left = std::make_shared<IntTree>();  // empty
    auto right = std::make_shared<IntTree>();  // empty
    auto tree = std::make_shared<IntTree>(PackedI32<>{5}, left, right);

    std::cout << "  Tree created with root value: ";
    if (auto val = tree->value()) {
        std::cout << val->value() << "\n";
    } else {
        std::cout << "empty\n";
    }
    std::cout << "  Tree is empty? " << tree->empty() << "\n";
}

// ============================================================
//  Demonstration 2: Advanced Numeric Codecs
// ============================================================

void demo_numeric_codecs() {
    std::cout << "\n=== Numeric Codecs Demo ===\n\n";

    // Floating point with custom precision
    using CustomFloat = Packed<double, codecs::FloatingPoint<16, 5>>;

    std::vector<double> floats = {3.14159, -2.71828, 0.0, 1e10, -1e-10};
    std::vector<uint8_t> buffer(1024);

    std::cout << "Custom Float Encoding (16-bit mantissa, 5-bit exponent):\n";
    for (double val : floats) {
        BitWriter writer(buffer.data());
        CustomFloat::encode(CustomFloat{val}, writer);
        writer.align();

        BitReader reader(buffer.data(), writer.bytes_written());
        auto decoded = CustomFloat::decode(reader);

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "  " << std::setw(12) << val << " -> "
                  << writer.bytes_written() << " bytes -> "
                  << std::setw(12) << decoded.value() << " (error: "
                  << std::scientific << std::abs(val - decoded.value()) << ")\n";
    }

    // Fixed decimal for money
    std::cout << "\nFixed Decimal (Money):\n";
    using PackedMoney = Packed<double, codecs::Money>;

    std::vector<double> amounts = {19.99, 100.00, 0.01, -50.50, 1234567.89};
    for (double amount : amounts) {
        BitWriter writer(buffer.data());
        PackedMoney::encode(PackedMoney{amount}, writer);
        writer.align();

        BitReader reader(buffer.data(), writer.bytes_written());
        auto decoded = PackedMoney::decode(reader);

        std::cout << "  $" << std::fixed << std::setprecision(2)
                  << std::setw(12) << amount << " -> "
                  << writer.bytes_written() << " bytes -> $"
                  << std::setw(12) << decoded.value() << "\n";
    }

    // Rational numbers
    std::cout << "\nRational Numbers:\n";
    using PackedRational = Packed<double, codecs::Rational<>>;

    std::vector<double> rationals = {0.5, 0.333333, 0.142857, 3.14159, 2.71828};
    for (double val : rationals) {
        BitWriter writer(buffer.data());
        PackedRational::encode(PackedRational{val}, writer);
        writer.align();

        BitReader reader(buffer.data(), writer.bytes_written());
        auto decoded = PackedRational::decode(reader);

        // Also decode as fraction
        BitReader reader2(buffer.data(), writer.bytes_written());
        auto fraction = codecs::Rational<>::decode_fraction(reader2);

        std::cout << "  " << std::fixed << std::setprecision(6)
                  << std::setw(10) << val << " -> "
                  << fraction.numerator << "/" << fraction.denominator
                  << " = " << decoded.value() << "\n";
    }
}

// ============================================================
//  Demonstration 3: STL Integration with Zero-Copy Operations
// ============================================================

void demo_stl_integration() {
    std::cout << "\n=== STL Integration Demo ===\n\n";

    // Create a packed container
    using PackedInt = PackedU32<codecs::EliasGamma>;
    PackedContainer<PackedInt> container;

    // Fill with data
    std::cout << "Populating container with 1000 values...\n";
    for (uint32_t i = 0; i < 1000; ++i) {
        container.push_back(PackedInt{i * i});
    }

    std::cout << "  Size: " << container.size() << " elements\n";
    std::cout << "  Packed bytes: " << container.data_bytes() << "\n";
    std::cout << "  Unpacked bytes: " << container.size() * sizeof(uint32_t) << "\n";
    std::cout << "  Compression ratio: " << std::fixed << std::setprecision(2)
              << container.compression_ratio() << "x\n\n";

    // STL algorithms work seamlessly
    std::cout << "Using STL algorithms:\n";

    // std::find_if
    auto it = std::find_if(container.begin(), container.end(),
                           [](const auto& proxy) {
                               return PackedInt::value_type(proxy) > 500000;
                           });

    if (it != container.end()) {
        size_t index = std::distance(container.begin(), it);
        std::cout << "  First value > 500000 at index " << index
                  << ": " << container[index] << "\n";
    }

    // std::accumulate
    auto sum = std::accumulate(container.begin(), container.begin() + 100,
                               uint64_t(0),
                               [](uint64_t acc, const auto& proxy) {
                                   return acc + PackedInt::value_type(proxy);
                               });
    std::cout << "  Sum of first 100 elements: " << sum << "\n";

    // Custom zero-copy algorithms
    auto result = algorithms::packed_find_if(container,
                                            [](uint32_t val) { return val == 144; });
    if (result) {
        std::cout << "  Found 144 (12^2) at index: " << *result << "\n";
    }

    // Transform with packing
    std::cout << "\nTransform operation (square root):\n";
    auto transformed = algorithms::packed_transform(container,
                                                   [](uint32_t val) {
                                                       return static_cast<uint32_t>(std::sqrt(val));
                                                   });

    std::cout << "  First 10 transformed values: ";
    for (size_t i = 0; i < 10; ++i) {
        std::cout << transformed[i] << " ";
    }
    std::cout << "\n";

    // Parallel execution
    std::cout << "\nParallel transform:\n";
    auto start = std::chrono::high_resolution_clock::now();

    auto par_result = algorithms::packed_transform_par(
        std::execution::par,
        container,
        [](uint32_t val) { return val / 2; }
    );

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "  Parallel transform completed in " << duration.count() << " μs\n";
}

// ============================================================
//  Demonstration 4: Coordinate Structures and Linear Algebra
// ============================================================

void demo_coordinates() {
    std::cout << "\n=== Coordinate Structures Demo ===\n\n";

    // 3D vectors with operations
    std::cout << "3D Vector Operations:\n";
    PackedVec3 v1{1.0, 2.0, 3.0};
    PackedVec3 v2{4.0, 5.0, 6.0};

    auto v3 = v1 + v2;
    auto v4 = v1.cross(v2);
    auto dot = v1.dot(v2);

    std::cout << "  v1 = (" << v1[0].value() << ", " << v1[1].value()
              << ", " << v1[2].value() << ")\n";
    std::cout << "  v2 = (" << v2[0].value() << ", " << v2[1].value()
              << ", " << v2[2].value() << ")\n";
    std::cout << "  v1 + v2 = (" << v3[0].value() << ", " << v3[1].value()
              << ", " << v3[2].value() << ")\n";
    std::cout << "  v1 × v2 = (" << v4[0].value() << ", " << v4[1].value()
              << ", " << v4[2].value() << ")\n";
    std::cout << "  v1 · v2 = " << dot << "\n";
    std::cout << "  |v1| = " << v1.magnitude() << "\n";

    // Matrix operations
    std::cout << "\nMatrix Operations:\n";
    PackedMatrix2x2::value_type m1_vals = {{{1.0, 2.0}, {3.0, 4.0}}};
    PackedMatrix2x2::value_type m2_vals = {{{5.0, 6.0}, {7.0, 8.0}}};
    PackedMatrix2x2 m1{m1_vals};
    PackedMatrix2x2 m2{m2_vals};

    auto m3 = m1 * m2;
    auto det = m1.determinant();

    std::cout << "  M1 = [" << m1(0,0).value() << " " << m1(0,1).value() << "]\n";
    std::cout << "       [" << m1(1,0).value() << " " << m1(1,1).value() << "]\n";
    std::cout << "  det(M1) = " << det << "\n";
    std::cout << "  M1 * M2 = [" << m3(0,0).value() << " " << m3(0,1).value() << "]\n";
    std::cout << "            [" << m3(1,0).value() << " " << m3(1,1).value() << "]\n";

    // Affine transformations
    std::cout << "\n2D Affine Transformations:\n";
    PackedVec<2> point{1.0, 0.0};

    auto rotate = PackedTransform2D::rotation_2d(M_PI / 4);  // 45 degrees
    auto translate = PackedTransform2D::translation(PackedVec<2>{2.0, 3.0});
    auto scale = PackedTransform2D::scale(2.0);

    // Compose transformations
    auto combined = translate * rotate * scale;

    auto transformed = combined.transform_point(point);
    std::cout << "  Point (1, 0) after scale(2) → rotate(45°) → translate(2, 3):\n";
    std::cout << "  Result: (" << transformed[0].value() << ", "
              << transformed[1].value() << ")\n";

    // Serialize transformation
    std::vector<uint8_t> buffer(256);
    BitWriter writer(buffer.data());
    PackedTransform2D::encode(combined, writer);
    writer.align();

    std::cout << "  Serialized transform: " << writer.bytes_written() << " bytes\n";
}

// ============================================================
//  Demonstration 5: Type-Erased Containers for Heterogeneous Data
// ============================================================

void demo_type_erasure() {
    std::cout << "\n=== Type-Erased Container Demo ===\n\n";

    // Create different packed containers
    auto int_container = TypeErasedPackedContainer::create<PackedU32<>>();
    auto float_container = TypeErasedPackedContainer::create<
        Packed<double, codecs::Float32>
    >();

    // Add data
    for (int i = 0; i < 10; ++i) {
        int_container.push_back(static_cast<uint32_t>(i * i));
        float_container.push_back(std::sqrt(static_cast<double>(i)));
    }

    // Store in a collection
    std::vector<TypeErasedPackedContainer> containers;
    containers.push_back(std::move(int_container));
    containers.push_back(std::move(float_container));

    // Process generically
    std::cout << "Processing heterogeneous containers:\n";
    for (size_t i = 0; i < containers.size(); ++i) {
        auto& cont = containers[i];
        std::cout << "  Container " << i << ":\n";
        std::cout << "    Type: " << cont.stored_type().name() << "\n";
        std::cout << "    Size: " << cont.size() << " elements\n";
        std::cout << "    Bytes: " << cont.data_bytes() << "\n";
        std::cout << "    First 3 values: ";

        for (size_t j = 0; j < std::min(size_t(3), cont.size()); ++j) {
            if (cont.holds_type<uint32_t>()) {
                if (auto val = cont.get<uint32_t>(j)) {
                    std::cout << *val << " ";
                }
            } else if (cont.holds_type<double>()) {
                if (auto val = cont.get<double>(j)) {
                    std::cout << std::fixed << std::setprecision(3) << *val << " ";
                }
            }
        }
        std::cout << "\n";
    }
}

// ============================================================
//  Benchmark: Compression Efficiency
// ============================================================

void benchmark_compression() {
    std::cout << "\n=== Compression Benchmark ===\n\n";

    const size_t N = 10000;

    struct Result {
        std::string codec_name;
        size_t packed_bytes;
        size_t unpacked_bytes;
        double ratio;
        double encode_time_ms;
        double decode_time_ms;
    };

    std::vector<Result> results;

    // Generate test data - geometric distribution
    std::vector<uint32_t> data;
    std::mt19937 gen(42);
    std::geometric_distribution<uint32_t> dist(0.1);
    for (size_t i = 0; i < N; ++i) {
        data.push_back(dist(gen));
    }

    // Test different codecs
    auto benchmark_codec = [&]<typename Codec>(const std::string& name) {
        using Packed = pfc::Packed<uint32_t, Codec>;
        std::vector<uint8_t> buffer(N * 16);  // Plenty of space

        // Encode
        auto start = std::chrono::high_resolution_clock::now();
        BitWriter writer(buffer.data());
        for (uint32_t val : data) {
            Packed::encode(Packed{val}, writer);
        }
        writer.align();
        auto mid = std::chrono::high_resolution_clock::now();

        // Decode
        BitReader reader(buffer.data(), writer.bytes_written());
        std::vector<uint32_t> decoded;
        decoded.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            decoded.push_back(Packed::decode(reader).value());
        }
        auto end = std::chrono::high_resolution_clock::now();

        // Verify
        bool correct = (decoded == data);
        if (!correct) {
            std::cout << "ERROR: " << name << " codec failed verification!\n";
        }

        // Record results
        auto encode_time = std::chrono::duration<double, std::milli>(mid - start).count();
        auto decode_time = std::chrono::duration<double, std::milli>(end - mid).count();

        results.push_back({
            name,
            writer.bytes_written(),
            N * sizeof(uint32_t),
            static_cast<double>(N * sizeof(uint32_t)) / writer.bytes_written(),
            encode_time,
            decode_time
        });
    };

    // Benchmark various codecs
    benchmark_codec.operator()<codecs::Unary>("Unary");
    benchmark_codec.operator()<codecs::EliasGamma>("Elias Gamma");
    benchmark_codec.operator()<codecs::EliasDelta>("Elias Delta");
    benchmark_codec.operator()<codecs::Fibonacci>("Fibonacci");
    benchmark_codec.operator()<codecs::Rice<3>>("Rice(k=3)");
    benchmark_codec.operator()<codecs::Rice<5>>("Rice(k=5)");
    benchmark_codec.operator()<codecs::Fixed<32>>("Fixed(32)");

    // Display results
    std::cout << "Dataset: " << N << " values from geometric distribution (p=0.1)\n\n";
    std::cout << std::left << std::setw(15) << "Codec"
              << std::right << std::setw(12) << "Packed"
              << std::setw(12) << "Unpacked"
              << std::setw(10) << "Ratio"
              << std::setw(12) << "Encode(ms)"
              << std::setw(12) << "Decode(ms)" << "\n";
    std::cout << std::string(71, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::left << std::setw(15) << r.codec_name
                  << std::right << std::setw(12) << r.packed_bytes
                  << std::setw(12) << r.unpacked_bytes
                  << std::setw(10) << std::fixed << std::setprecision(2) << r.ratio << "x"
                  << std::setw(12) << std::setprecision(3) << r.encode_time_ms
                  << std::setw(12) << r.decode_time_ms << "\n";
    }

    // Find best codec
    auto best = std::min_element(results.begin(), results.end(),
                                 [](const auto& a, const auto& b) {
                                     return a.packed_bytes < b.packed_bytes;
                                 });
    std::cout << "\nBest compression: " << best->codec_name
              << " (" << best->ratio << "x)\n";
}

// ============================================================
//  Main Function
// ============================================================

int main() {
    std::cout << "====================================\n";
    std::cout << "   Advanced Packed Codec Library   \n";
    std::cout << "====================================\n";

    try {
        demo_algebraic_types();
        demo_numeric_codecs();
        demo_stl_integration();
        demo_coordinates();
        demo_type_erasure();
        benchmark_compression();

        std::cout << "\n====================================\n";
        std::cout << "        All Demos Complete!        \n";
        std::cout << "====================================\n\n";

        std::cout << "Key Achievements:\n";
        std::cout << "  ✓ Full algebraic type system with sum and product types\n";
        std::cout << "  ✓ Recursive data structures (lists, trees)\n";
        std::cout << "  ✓ Advanced numeric codecs (floating point, rational, complex)\n";
        std::cout << "  ✓ STL-compliant containers with proxy iterators\n";
        std::cout << "  ✓ Zero-copy algorithms and parallel execution\n";
        std::cout << "  ✓ Stepanov-inspired coordinate structures\n";
        std::cout << "  ✓ Type-erased containers for heterogeneous data\n";
        std::cout << "  ✓ Elegant, composable API throughout\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}