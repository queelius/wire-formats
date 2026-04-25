#pragma once
// coordinates.hpp - Stepanov-inspired coordinate structures
// Regular types with algebraic operations
// Mathematical beauty in computational form

#include "core.hpp"
#include "packed.hpp"
#include "numeric_codecs.hpp"
#include <array>
#include <cmath>
#include <numeric>

namespace pfc {

// ============================================================
//  Regular Type Concepts (from Elements of Programming)
// ============================================================

template<typename T>
concept Regular = std::copyable<T> && std::equality_comparable<T>;

template<typename T>
concept TotallyOrdered = Regular<T> && std::totally_ordered<T>;

template<typename T>
concept Additive = Regular<T> && requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { -a } -> std::convertible_to<T>;
    { a += b } -> std::same_as<T&>;
    { a -= b } -> std::same_as<T&>;
};

template<typename T, typename Scalar>
concept VectorSpace = Additive<T> && requires(T v, Scalar s) {
    { v * s } -> std::convertible_to<T>;
    { s * v } -> std::convertible_to<T>;
    { v / s } -> std::convertible_to<T>;
    { v *= s } -> std::same_as<T&>;
    { v /= s } -> std::same_as<T&>;
};

// ============================================================
//  Packed Point - N-dimensional coordinate
// ============================================================

template<size_t N, PackedValue Coordinate = PackedU32<>>
class PackedPoint {
public:
    using value_type = std::array<typename Coordinate::value_type, N>;
    using coordinate_type = Coordinate;
    static constexpr size_t dimensions = N;

private:
    std::array<Coordinate, N> coords_;

public:
    // Constructors
    PackedPoint() = default;

    explicit PackedPoint(const value_type& values) {
        for (size_t i = 0; i < N; ++i) {
            coords_[i] = Coordinate{values[i]};
        }
    }

    template<typename... Args>
    explicit PackedPoint(Args... args) requires (sizeof...(Args) == N) {
        size_t i = 0;
        ((coords_[i++] = Coordinate{args}), ...);
    }

    // Element access
    [[nodiscard]] const Coordinate& operator[](size_t i) const { return coords_[i]; }
    [[nodiscard]] Coordinate& operator[](size_t i) { return coords_[i]; }

    // Value extraction
    [[nodiscard]] value_type value() const {
        value_type result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = coords_[i].value();
        }
        return result;
    }

    // Equality
    bool operator==(const PackedPoint& other) const {
        for (size_t i = 0; i < N; ++i) {
            if (coords_[i].value() != other.coords_[i].value()) {
                return false;
            }
        }
        return true;
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedPoint& p, Sink& sink) requires BitSink<Sink> {
        for (const auto& coord : p.coords_) {
            Coordinate::encode(coord, sink);
        }
    }

    template<typename Source>
    static PackedPoint decode(Source& source) requires BitSource<Source> {
        PackedPoint result;
        for (auto& coord : result.coords_) {
            coord = Coordinate::decode(source);
        }
        return result;
    }
};

// Common point types
template<typename Coord = PackedI32<>>
using PackedPoint2D = PackedPoint<2, Coord>;

template<typename Coord = PackedI32<>>
using PackedPoint3D = PackedPoint<3, Coord>;

// ============================================================
//  Packed Vector - With algebraic operations
// ============================================================

template<size_t N, PackedValue Component = Packed<double, codecs::Float32>>
class PackedVec {
public:
    using value_type = std::array<typename Component::value_type, N>;
    using component_type = Component;
    using scalar_type = typename Component::value_type;
    static constexpr size_t dimensions = N;

private:
    std::array<Component, N> components_;

public:
    // Constructors
    PackedVec() = default;

    explicit PackedVec(const value_type& values) {
        for (size_t i = 0; i < N; ++i) {
            components_[i] = Component{values[i]};
        }
    }

    template<typename... Args>
    explicit PackedVec(Args... args) requires (sizeof...(Args) == N) {
        size_t i = 0;
        ((components_[i++] = Component{static_cast<scalar_type>(args)}), ...);
    }

    // Element access
    [[nodiscard]] const Component& operator[](size_t i) const { return components_[i]; }
    [[nodiscard]] Component& operator[](size_t i) { return components_[i]; }

    // Value extraction
    [[nodiscard]] value_type value() const {
        value_type result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = components_[i].value();
        }
        return result;
    }

    // Vector addition
    PackedVec operator+(const PackedVec& other) const {
        value_type result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = components_[i].value() + other.components_[i].value();
        }
        return PackedVec{result};
    }

    PackedVec& operator+=(const PackedVec& other) {
        for (size_t i = 0; i < N; ++i) {
            components_[i] = Component{components_[i].value() + other.components_[i].value()};
        }
        return *this;
    }

    // Vector subtraction
    PackedVec operator-(const PackedVec& other) const {
        value_type result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = components_[i].value() - other.components_[i].value();
        }
        return PackedVec{result};
    }

    PackedVec& operator-=(const PackedVec& other) {
        for (size_t i = 0; i < N; ++i) {
            components_[i] = Component{components_[i].value() - other.components_[i].value()};
        }
        return *this;
    }

    // Negation
    PackedVec operator-() const {
        value_type result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = -components_[i].value();
        }
        return PackedVec{result};
    }

    // Scalar multiplication
    PackedVec operator*(scalar_type s) const {
        value_type result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = components_[i].value() * s;
        }
        return PackedVec{result};
    }

    friend PackedVec operator*(scalar_type s, const PackedVec& v) {
        return v * s;
    }

    PackedVec& operator*=(scalar_type s) {
        for (size_t i = 0; i < N; ++i) {
            components_[i] = Component{components_[i].value() * s};
        }
        return *this;
    }

    // Scalar division
    PackedVec operator/(scalar_type s) const {
        value_type result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = components_[i].value() / s;
        }
        return PackedVec{result};
    }

    PackedVec& operator/=(scalar_type s) {
        for (size_t i = 0; i < N; ++i) {
            components_[i] = Component{components_[i].value() / s};
        }
        return *this;
    }

    // Dot product
    scalar_type dot(const PackedVec& other) const {
        scalar_type result = 0;
        for (size_t i = 0; i < N; ++i) {
            result += components_[i].value() * other.components_[i].value();
        }
        return result;
    }

    // Magnitude
    scalar_type magnitude() const {
        return std::sqrt(dot(*this));
    }

    scalar_type magnitude_squared() const {
        return dot(*this);
    }

    // Normalization
    PackedVec normalized() const {
        scalar_type mag = magnitude();
        if (mag == 0) return *this;
        return *this / mag;
    }

    // Cross product (3D only)
    PackedVec cross(const PackedVec& other) const requires (N == 3) {
        auto a = value();
        auto b = other.value();
        return PackedVec{
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]
        };
    }

    // Equality
    bool operator==(const PackedVec& other) const {
        for (size_t i = 0; i < N; ++i) {
            if (components_[i].value() != other.components_[i].value()) {
                return false;
            }
        }
        return true;
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedVec& v, Sink& sink) requires BitSink<Sink> {
        for (const auto& comp : v.components_) {
            Component::encode(comp, sink);
        }
    }

    template<typename Source>
    static PackedVec decode(Source& source) requires BitSource<Source> {
        PackedVec result;
        for (auto& comp : result.components_) {
            comp = Component::decode(source);
        }
        return result;
    }
};

// Common vector types
using PackedVec2 = PackedVec<2>;
using PackedVec3 = PackedVec<3>;
using PackedVec4 = PackedVec<4>;

// ============================================================
//  Packed Matrix - With matrix operations
// ============================================================

template<size_t Rows, size_t Cols,
         PackedValue Element = Packed<double, codecs::Float32>>
class PackedMatrix {
public:
    using value_type = std::array<std::array<typename Element::value_type, Cols>, Rows>;
    using element_type = Element;
    using scalar_type = typename Element::value_type;
    static constexpr size_t rows = Rows;
    static constexpr size_t cols = Cols;

private:
    std::array<std::array<Element, Cols>, Rows> data_;

public:
    // Constructors
    PackedMatrix() = default;

    explicit PackedMatrix(const value_type& values) {
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                data_[i][j] = Element{values[i][j]};
            }
        }
    }

    // Element access
    [[nodiscard]] const Element& operator()(size_t i, size_t j) const {
        return data_[i][j];
    }

    [[nodiscard]] Element& operator()(size_t i, size_t j) {
        return data_[i][j];
    }

    // Row access
    [[nodiscard]] PackedVec<Cols, Element> row(size_t i) const {
        std::array<scalar_type, Cols> values;
        for (size_t j = 0; j < Cols; ++j) {
            values[j] = data_[i][j].value();
        }
        return PackedVec<Cols, Element>{values};
    }

    // Column access
    [[nodiscard]] PackedVec<Rows, Element> column(size_t j) const {
        std::array<scalar_type, Rows> values;
        for (size_t i = 0; i < Rows; ++i) {
            values[i] = data_[i][j].value();
        }
        return PackedVec<Rows, Element>{values};
    }

    // Value extraction
    [[nodiscard]] value_type value() const {
        value_type result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[i][j] = data_[i][j].value();
            }
        }
        return result;
    }

    // Matrix addition
    PackedMatrix operator+(const PackedMatrix& other) const {
        value_type result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[i][j] = data_[i][j].value() + other.data_[i][j].value();
            }
        }
        return PackedMatrix{result};
    }

    // Matrix subtraction
    PackedMatrix operator-(const PackedMatrix& other) const {
        value_type result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[i][j] = data_[i][j].value() - other.data_[i][j].value();
            }
        }
        return PackedMatrix{result};
    }

    // Scalar multiplication
    PackedMatrix operator*(scalar_type s) const {
        value_type result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[i][j] = data_[i][j].value() * s;
            }
        }
        return PackedMatrix{result};
    }

    // Matrix-vector multiplication
    PackedVec<Rows, Element> operator*(const PackedVec<Cols, Element>& v) const {
        std::array<scalar_type, Rows> result;
        for (size_t i = 0; i < Rows; ++i) {
            result[i] = row(i).dot(v);
        }
        return PackedVec<Rows, Element>{result};
    }

    // Matrix multiplication
    template<size_t OtherCols>
    PackedMatrix<Rows, OtherCols, Element> operator*(
        const PackedMatrix<Cols, OtherCols, Element>& other) const {

        using ResultMatrix = PackedMatrix<Rows, OtherCols, Element>;
        typename ResultMatrix::value_type result{};

        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < OtherCols; ++j) {
                scalar_type sum = 0;
                for (size_t k = 0; k < Cols; ++k) {
                    sum += data_[i][k].value() * other(k, j).value();
                }
                result[i][j] = sum;
            }
        }

        return ResultMatrix{result};
    }

    // Transpose
    PackedMatrix<Cols, Rows, Element> transpose() const {
        typename PackedMatrix<Cols, Rows, Element>::value_type result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[j][i] = data_[i][j].value();
            }
        }
        return PackedMatrix<Cols, Rows, Element>{result};
    }

    // Identity matrix (square matrices only)
    static PackedMatrix identity() requires (Rows == Cols) {
        value_type result{};
        for (size_t i = 0; i < Rows; ++i) {
            result[i][i] = 1;
        }
        return PackedMatrix{result};
    }

    // Determinant (2x2 and 3x3 only for simplicity)
    scalar_type determinant() const requires (Rows == Cols && Rows == 2) {
        auto m = value();
        return m[0][0] * m[1][1] - m[0][1] * m[1][0];
    }

    scalar_type determinant() const requires (Rows == Cols && Rows == 3) {
        auto m = value();
        return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
             - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
             + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedMatrix& m, Sink& sink) requires BitSink<Sink> {
        for (const auto& row : m.data_) {
            for (const auto& elem : row) {
                Element::encode(elem, sink);
            }
        }
    }

    template<typename Source>
    static PackedMatrix decode(Source& source) requires BitSource<Source> {
        PackedMatrix result;
        for (auto& row : result.data_) {
            for (auto& elem : row) {
                elem = Element::decode(source);
            }
        }
        return result;
    }
};

// Common matrix types
using PackedMatrix2x2 = PackedMatrix<2, 2>;
using PackedMatrix3x3 = PackedMatrix<3, 3>;
using PackedMatrix4x4 = PackedMatrix<4, 4>;

// ============================================================
//  Affine Transformations
// ============================================================

template<size_t N, PackedValue Scalar = Packed<double, codecs::Float32>>
class PackedAffineTransform {
public:
    using matrix_type = PackedMatrix<N, N, Scalar>;
    using vector_type = PackedVec<N, Scalar>;
    using scalar_type = typename Scalar::value_type;

private:
    matrix_type matrix_;
    vector_type translation_;

public:
    // Constructors
    PackedAffineTransform()
        : matrix_(matrix_type::identity()), translation_() {}

    PackedAffineTransform(const matrix_type& m, const vector_type& t)
        : matrix_(m), translation_(t) {}

    // Apply transformation to a point
    vector_type transform_point(const vector_type& p) const {
        return matrix_ * p + translation_;
    }

    // Apply transformation to a vector (no translation)
    vector_type transform_vector(const vector_type& v) const {
        return matrix_ * v;
    }

    // Composition of transformations
    PackedAffineTransform operator*(const PackedAffineTransform& other) const {
        return PackedAffineTransform{
            matrix_ * other.matrix_,
            matrix_ * other.translation_ + translation_
        };
    }

    // Factory methods for common transformations
    static PackedAffineTransform translation(const vector_type& t) {
        return PackedAffineTransform{matrix_type::identity(), t};
    }

    static PackedAffineTransform scale(scalar_type s) {
        auto m = matrix_type::identity();
        auto vals = m.value();
        for (size_t i = 0; i < N; ++i) {
            vals[i][i] = s;
        }
        return PackedAffineTransform{matrix_type{vals}, vector_type{}};
    }

    // 2D rotation
    static PackedAffineTransform rotation_2d(scalar_type angle) requires (N == 2) {
        scalar_type c = std::cos(angle);
        scalar_type s = std::sin(angle);
        typename matrix_type::value_type m = {{
            {c, -s},
            {s, c}
        }};
        return PackedAffineTransform{matrix_type{m}, vector_type{}};
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedAffineTransform& t, Sink& sink) requires BitSink<Sink> {
        matrix_type::encode(t.matrix_, sink);
        vector_type::encode(t.translation_, sink);
    }

    template<typename Source>
    static PackedAffineTransform decode(Source& source) requires BitSource<Source> {
        auto m = matrix_type::decode(source);
        auto t = vector_type::decode(source);
        return PackedAffineTransform{m, t};
    }
};

// Common transform types
using PackedTransform2D = PackedAffineTransform<2>;
using PackedTransform3D = PackedAffineTransform<3>;

} // namespace pfc