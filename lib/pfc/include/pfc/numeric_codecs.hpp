#pragma once
// numeric_codecs.hpp - Advanced numeric codecs
// Floating point, fixed decimal, rational numbers
// Precision and efficiency in perfect harmony

#include "core.hpp"
#include "codecs.hpp"
#include <cmath>
#include <numeric>

namespace pfc::codecs {

// ============================================================
//  Floating Point Codec - Configurable precision
// ============================================================

template<size_t MantissaBits = 23, size_t ExponentBits = 8, typename SignCodec = Boolean>
struct FloatingPoint {
    static_assert(MantissaBits > 0 && MantissaBits <= 52, "Invalid mantissa bits");
    static_assert(ExponentBits > 0 && ExponentBits <= 11, "Invalid exponent bits");

    // Encode a floating point number
    static void encode(double value, auto& sink) requires BitSink<decltype(sink)> {
        // Handle special cases
        if (std::isnan(value)) {
            sink.write(true);  // Special flag
            sink.write(true);  // NaN marker
            return;
        }

        if (std::isinf(value)) {
            sink.write(true);  // Special flag
            sink.write(false); // Inf marker
            SignCodec::encode(value < 0, sink);
            return;
        }

        sink.write(false);  // Not special

        // Zero is special
        if (value == 0.0) {
            sink.write(true);  // Zero flag
            SignCodec::encode(std::signbit(value), sink);  // Preserve -0.0
            return;
        }

        sink.write(false);  // Not zero

        // Decompose the number
        int exponent;
        double mantissa = std::frexp(value, &exponent);

        // Sign
        bool negative = mantissa < 0;
        SignCodec::encode(negative, sink);
        if (negative) mantissa = -mantissa;

        // Normalize mantissa to [0.5, 1)
        // frexp already gives us this

        // Encode exponent with bias
        int32_t bias = (1 << (ExponentBits - 1)) - 1;
        int32_t biased_exp = exponent + bias;

        // Clamp to representable range
        biased_exp = std::max(0, std::min((1 << ExponentBits) - 1, biased_exp));

        for (size_t i = 0; i < ExponentBits; ++i) {
            sink.write((biased_exp >> i) & 1);
        }

        // Encode mantissa (skip the implicit leading 1)
        mantissa = (mantissa - 0.5) * 2;  // Now in [0, 1)

        for (size_t i = 0; i < MantissaBits; ++i) {
            mantissa *= 2;
            bool bit = mantissa >= 1.0;
            sink.write(bit);
            if (bit) mantissa -= 1.0;
        }
    }

    static double decode(auto& source) requires BitSource<decltype(source)> {
        // Check for special values
        if (source.read()) {  // Special flag
            if (source.read()) {  // NaN
                return std::numeric_limits<double>::quiet_NaN();
            } else {  // Infinity
                bool negative = SignCodec::decode(source);
                return negative ? -std::numeric_limits<double>::infinity()
                                : std::numeric_limits<double>::infinity();
            }
        }

        // Check for zero
        if (source.read()) {  // Zero flag
            bool negative = SignCodec::decode(source);
            return negative ? -0.0 : 0.0;
        }

        // Decode sign
        bool negative = SignCodec::decode(source);

        // Decode exponent
        int32_t biased_exp = 0;
        for (size_t i = 0; i < ExponentBits; ++i) {
            biased_exp |= static_cast<int32_t>(source.read()) << i;
        }

        int32_t bias = (1 << (ExponentBits - 1)) - 1;
        int exponent = biased_exp - bias;

        // Decode mantissa
        double mantissa = 0.5;  // Implicit leading 1
        double bit_value = 0.25;

        for (size_t i = 0; i < MantissaBits; ++i) {
            if (source.read()) {
                mantissa += bit_value;
            }
            bit_value /= 2;
        }

        // Reconstruct the number
        double result = std::ldexp(mantissa, exponent);
        return negative ? -result : result;
    }
};

// Common floating point formats
using Float16 = FloatingPoint<10, 5>;     // Half precision
using Float32 = FloatingPoint<23, 8>;     // Single precision
using Float64 = FloatingPoint<52, 11>;    // Double precision
using BFloat16 = FloatingPoint<7, 8>;     // Brain float

// ============================================================
//  Fixed Decimal Codec - For monetary values
// ============================================================

template<size_t IntegerBits, size_t FractionalBits, typename IntCodec = EliasGamma>
struct FixedDecimal {
    static_assert(IntegerBits > 0 && IntegerBits <= 32, "Invalid integer bits");
    static_assert(FractionalBits > 0 && FractionalBits <= 32, "Invalid fractional bits");

    static constexpr double scale = std::pow(10.0, FractionalBits);

    static void encode(double value, auto& sink) requires BitSink<decltype(sink)> {
        // Convert to fixed point
        int64_t fixed = static_cast<int64_t>(std::round(value * scale));

        // Use signed codec for the integer
        Signed<IntCodec>::encode(fixed, sink);
    }

    static double decode(auto& source) requires BitSource<decltype(source)> {
        int64_t fixed = Signed<IntCodec>::template decode<int64_t>(source);
        return fixed / scale;
    }
};

// Common decimal formats
using Money = FixedDecimal<10, 2>;        // Up to 10 billion with cents
using Percentage = FixedDecimal<3, 4>;    // 0.0001 to 999.9999%
using HighPrecision = FixedDecimal<16, 6>; // 6 decimal places

// ============================================================
//  Rational Number Codec - Exact fractions
// ============================================================

template<typename NumeratorCodec = SignedGamma, typename DenominatorCodec = EliasGamma>
struct Rational {
    struct Fraction {
        int64_t numerator;
        uint64_t denominator;

        // Simplify the fraction
        void simplify() {
            if (denominator == 0) {
                denominator = 1;  // Avoid division by zero
                return;
            }

            uint64_t g = std::gcd(std::abs(numerator), denominator);
            numerator /= g;
            denominator /= g;
        }

        double to_double() const {
            return static_cast<double>(numerator) / denominator;
        }

        static Fraction from_double(double value, uint64_t max_denominator = 1000000) {
            // Use continued fractions to find best rational approximation
            if (std::isnan(value) || std::isinf(value)) {
                return {0, 1};
            }

            bool negative = value < 0;
            value = std::abs(value);

            uint64_t whole = static_cast<uint64_t>(value);
            double frac = value - whole;

            if (frac < 1e-10) {
                return {negative ? -static_cast<int64_t>(whole) : whole, 1};
            }

            // Continued fraction expansion
            uint64_t prev_num = 1, cur_num = whole;
            uint64_t prev_den = 0, cur_den = 1;

            while (cur_den < max_denominator && frac > 1e-10) {
                double inv = 1.0 / frac;
                uint64_t term = static_cast<uint64_t>(inv);
                frac = inv - term;

                uint64_t next_num = term * cur_num + prev_num;
                uint64_t next_den = term * cur_den + prev_den;

                if (next_den > max_denominator) break;

                prev_num = cur_num;
                cur_num = next_num;
                prev_den = cur_den;
                cur_den = next_den;
            }

            return {negative ? -static_cast<int64_t>(cur_num) : cur_num, cur_den};
        }
    };

    static void encode(double value, auto& sink) requires BitSink<decltype(sink)> {
        Fraction frac = Fraction::from_double(value);
        frac.simplify();

        NumeratorCodec::encode(frac.numerator, sink);
        DenominatorCodec::encode(frac.denominator, sink);
    }

    static double decode(auto& source) requires BitSource<decltype(source)> {
        int64_t num = NumeratorCodec::template decode<int64_t>(source);
        uint64_t den = DenominatorCodec::template decode<uint64_t>(source);

        if (den == 0) den = 1;  // Safety
        return static_cast<double>(num) / den;
    }

    // Alternative: encode/decode the Fraction directly
    static void encode_fraction(const Fraction& frac, auto& sink) requires BitSink<decltype(sink)> {
        Fraction simplified = frac;
        simplified.simplify();

        NumeratorCodec::encode(simplified.numerator, sink);
        DenominatorCodec::encode(simplified.denominator, sink);
    }

    static Fraction decode_fraction(auto& source) requires BitSource<decltype(source)> {
        int64_t num = NumeratorCodec::template decode<int64_t>(source);
        uint64_t den = DenominatorCodec::template decode<uint64_t>(source);
        return {num, den};
    }
};

// ============================================================
//  Scientific Notation Codec
// ============================================================

template<typename MantissaCodec = SignedGamma, typename ExponentCodec = SignedGamma>
struct Scientific {
    struct SciNumber {
        double mantissa;   // Typically in [-10, 10)
        int32_t exponent;

        double to_double() const {
            return mantissa * std::pow(10.0, exponent);
        }

        static SciNumber from_double(double value) {
            if (value == 0.0) return {0.0, 0};

            int exp = 0;
            double mant = value;

            // Normalize to [-10, 10)
            while (std::abs(mant) >= 10.0) {
                mant /= 10.0;
                exp++;
            }
            while (std::abs(mant) < 1.0 && mant != 0.0) {
                mant *= 10.0;
                exp--;
            }

            return {mant, exp};
        }
    };

    static void encode(double value, auto& sink) requires BitSink<decltype(sink)> {
        SciNumber sci = SciNumber::from_double(value);

        // Encode mantissa as fixed decimal (3 digits precision)
        int32_t mant_fixed = static_cast<int32_t>(std::round(sci.mantissa * 1000));
        MantissaCodec::encode(mant_fixed, sink);

        // Encode exponent
        ExponentCodec::encode(sci.exponent, sink);
    }

    static double decode(auto& source) requires BitSource<decltype(source)> {
        int32_t mant_fixed = MantissaCodec::template decode<int32_t>(source);
        int32_t exp = ExponentCodec::template decode<int32_t>(source);

        double mantissa = mant_fixed / 1000.0;
        return mantissa * std::pow(10.0, exp);
    }
};

// ============================================================
//  Complex Number Codec
// ============================================================

template<typename RealCodec = Float32, typename ImagCodec = Float32>
struct Complex {
    struct ComplexNumber {
        double real;
        double imag;

        double magnitude() const {
            return std::sqrt(real * real + imag * imag);
        }

        double phase() const {
            return std::atan2(imag, real);
        }
    };

    static void encode(const ComplexNumber& c, auto& sink) requires BitSink<decltype(sink)> {
        RealCodec::encode(c.real, sink);
        ImagCodec::encode(c.imag, sink);
    }

    static ComplexNumber decode(auto& source) requires BitSource<decltype(source)> {
        double real = RealCodec::decode(source);
        double imag = ImagCodec::decode(source);
        return {real, imag};
    }

    // Polar form encoding
    template<typename MagCodec = Float32, typename PhaseCodec = Float32>
    static void encode_polar(const ComplexNumber& c, auto& sink) requires BitSink<decltype(sink)> {
        MagCodec::encode(c.magnitude(), sink);
        PhaseCodec::encode(c.phase(), sink);
    }

    template<typename MagCodec = Float32, typename PhaseCodec = Float32>
    static ComplexNumber decode_polar(auto& source) requires BitSource<decltype(source)> {
        double mag = MagCodec::decode(source);
        double phase = PhaseCodec::decode(source);
        return {mag * std::cos(phase), mag * std::sin(phase)};
    }
};

// ============================================================
//  Interval Codec - For interval arithmetic
// ============================================================

template<typename BoundCodec = Float32>
struct Interval {
    struct Range {
        double lower;
        double upper;

        bool contains(double x) const {
            return x >= lower && x <= upper;
        }

        double width() const {
            return upper - lower;
        }

        double center() const {
            return (lower + upper) / 2;
        }
    };

    static void encode(const Range& r, auto& sink) requires BitSink<decltype(sink)> {
        // Encode as center and radius for better compression
        double center = r.center();
        double radius = r.width() / 2;

        BoundCodec::encode(center, sink);
        BoundCodec::encode(radius, sink);
    }

    static Range decode(auto& source) requires BitSource<decltype(source)> {
        double center = BoundCodec::decode(source);
        double radius = BoundCodec::decode(source);

        return {center - radius, center + radius};
    }
};

} // namespace pfc::codecs