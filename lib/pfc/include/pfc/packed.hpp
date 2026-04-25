#pragma once
// packed.hpp - Zero-copy packed value types and containers
// Where data compression meets elegant abstraction

#include "core.hpp"
#include "codecs.hpp"
#include <vector>
#include <memory>
#include <tuple>
#include <optional>

namespace pfc {

// ============================================================
//  Packed Value Concept
// ============================================================

template<typename P>
concept PackedValue = requires(const P& p, BitWriter& w, BitReader& r) {
    typename P::value_type;
    { P::encode(p, w) } -> std::same_as<void>;
    { P::decode(r) } -> std::same_as<P>;
    { p.value() } -> std::convertible_to<typename P::value_type>;
};

// ============================================================
//  Basic Packed Type - The fundamental building block
// ============================================================

template<typename T, typename Codec>
class Packed {
public:
    using value_type = T;
    using codec_type = Codec;

private:
    T value_;

public:
    // Constructors
    Packed() = default;
    explicit Packed(const T& val) : value_(val) {}
    explicit Packed(T&& val) : value_(std::move(val)) {}

    // Value access
    [[nodiscard]] const T& value() const noexcept { return value_; }
    [[nodiscard]] T& value() noexcept { return value_; }

    // Implicit conversion for convenience
    operator const T&() const noexcept { return value_; }

    // Serialization
    template<typename Sink>
    static void encode(const Packed& p, Sink& sink) requires BitSink<Sink> {
        Codec::encode(p.value_, sink);
    }

    template<typename Source>
    static Packed decode(Source& source) requires BitSource<Source> {
        if constexpr (requires { Codec::template decode<T>(source); }) {
            // Codec has templated decode method
            return Packed{Codec::template decode<T>(source)};
        } else {
            // Codec has non-templated decode method (returns T directly)
            return Packed{Codec::decode(source)};
        }
    }
};

// Convenience aliases
template<typename Codec = codecs::EliasGamma>
using PackedU32 = Packed<uint32_t, Codec>;

template<typename Codec = codecs::EliasGamma>
using PackedU64 = Packed<uint64_t, Codec>;

template<typename Codec = codecs::SignedGamma>
using PackedI32 = Packed<int32_t, Codec>;

template<typename Codec = codecs::SignedGamma>
using PackedI64 = Packed<int64_t, Codec>;

using PackedBool = Packed<bool, codecs::Boolean>;

// ============================================================
//  Packed Pair - Composing two packed values
// ============================================================

template<PackedValue First, PackedValue Second>
class PackedPair {
public:
    using value_type = std::pair<typename First::value_type,
                                  typename Second::value_type>;
    using first_type = First;
    using second_type = Second;

private:
    First first_;
    Second second_;

public:
    // Constructors
    PackedPair() = default;

    PackedPair(const First& f, const Second& s)
        : first_(f), second_(s) {}

    PackedPair(First&& f, Second&& s)
        : first_(std::move(f)), second_(std::move(s)) {}

    explicit PackedPair(const value_type& p)
        : first_(p.first), second_(p.second) {}

    // Element access
    [[nodiscard]] const First& first() const noexcept { return first_; }
    [[nodiscard]] const Second& second() const noexcept { return second_; }

    [[nodiscard]] First& first() noexcept { return first_; }
    [[nodiscard]] Second& second() noexcept { return second_; }

    // Value access
    [[nodiscard]] value_type value() const {
        return {first_.value(), second_.value()};
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedPair& p, Sink& sink) requires BitSink<Sink> {
        First::encode(p.first_, sink);
        Second::encode(p.second_, sink);
    }

    template<typename Source>
    static PackedPair decode(Source& source) requires BitSource<Source> {
        auto f = First::decode(source);
        auto s = Second::decode(source);
        return PackedPair{std::move(f), std::move(s)};
    }
};

// ============================================================
//  Packed Tuple - N-way composition
// ============================================================

template<PackedValue... Types>
class PackedTuple {
public:
    using value_type = std::tuple<typename Types::value_type...>;

private:
    std::tuple<Types...> elements_;

    // Helper for encoding
    template<size_t... Is, typename Sink>
    static void encode_impl(const std::tuple<Types...>& t,
                            Sink& sink,
                            std::index_sequence<Is...>) {
        (Types::encode(std::get<Is>(t), sink), ...);
    }

    // Helper for decoding
    template<size_t... Is, typename Source>
    static std::tuple<Types...> decode_impl(Source& source,
                                            std::index_sequence<Is...>) {
        return std::tuple<Types...>{Types::decode(source)...};
    }

public:
    // Constructors
    PackedTuple() = default;

    explicit PackedTuple(const Types&... args)
        : elements_(args...) {}

    explicit PackedTuple(Types&&... args)
        : elements_(std::move(args)...) {}

    // Element access
    template<size_t I>
    [[nodiscard]] auto& get() noexcept {
        return std::get<I>(elements_);
    }

    template<size_t I>
    [[nodiscard]] const auto& get() const noexcept {
        return std::get<I>(elements_);
    }

    // Value access
    [[nodiscard]] value_type value() const {
        return std::apply([](const auto&... args) {
            return std::make_tuple(args.value()...);
        }, elements_);
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedTuple& p, Sink& sink) requires BitSink<Sink> {
        encode_impl(p.elements_, sink, std::index_sequence_for<Types...>{});
    }

    template<typename Source>
    static PackedTuple decode(Source& source) requires BitSource<Source> {
        auto elements = decode_impl(source, std::index_sequence_for<Types...>{});
        return std::apply([](auto&&... args) {
            return PackedTuple{std::move(args)...};
        }, std::move(elements));
    }
};

// ============================================================
//  Packed Optional - Maybe there's a value
// ============================================================

template<PackedValue T>
class PackedOptional {
public:
    using value_type = std::optional<typename T::value_type>;

private:
    std::optional<T> opt_;

public:
    // Constructors
    PackedOptional() = default;
    PackedOptional(std::nullopt_t) : opt_(std::nullopt) {}

    explicit PackedOptional(const T& val) : opt_(val) {}
    explicit PackedOptional(T&& val) : opt_(std::move(val)) {}

    explicit PackedOptional(const value_type& val)
        : opt_(val ? std::optional<T>{T{*val}} : std::nullopt) {}

    // Query
    [[nodiscard]] bool has_value() const noexcept { return opt_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    // Access
    [[nodiscard]] const T& operator*() const { return *opt_; }
    [[nodiscard]] T& operator*() { return *opt_; }

    [[nodiscard]] const T* operator->() const { return &*opt_; }
    [[nodiscard]] T* operator->() { return &*opt_; }

    // Value access
    [[nodiscard]] value_type value() const {
        return opt_ ? std::optional{opt_->value()} : std::nullopt;
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedOptional& p, Sink& sink) requires BitSink<Sink> {
        codecs::Boolean::encode(p.has_value(), sink);
        if (p.has_value()) {
            T::encode(*p.opt_, sink);
        }
    }

    template<typename Source>
    static PackedOptional decode(Source& source) requires BitSource<Source> {
        bool has_val = codecs::Boolean::decode(source);
        if (has_val) {
            return PackedOptional{T::decode(source)};
        }
        return PackedOptional{};
    }
};

// ============================================================
//  Packed Vector - Dynamic sequences
// ============================================================

template<PackedValue Element, typename LengthCodec = codecs::EliasGamma>
class PackedVector {
public:
    using value_type = std::vector<typename Element::value_type>;
    using element_type = Element;
    using size_type = size_t;

private:
    std::vector<Element> elements_;

public:
    // Constructors
    PackedVector() = default;

    explicit PackedVector(size_type count) : elements_(count) {}

    explicit PackedVector(const value_type& vals) {
        elements_.reserve(vals.size());
        for (const auto& v : vals) {
            elements_.emplace_back(v);
        }
    }

    // Size operations
    [[nodiscard]] size_type size() const noexcept { return elements_.size(); }
    [[nodiscard]] bool empty() const noexcept { return elements_.empty(); }

    void reserve(size_type new_cap) { elements_.reserve(new_cap); }
    void clear() noexcept { elements_.clear(); }

    // Element access
    [[nodiscard]] const Element& operator[](size_type pos) const {
        return elements_[pos];
    }

    [[nodiscard]] Element& operator[](size_type pos) {
        return elements_[pos];
    }

    // Modifiers
    void push_back(const Element& elem) {
        elements_.push_back(elem);
    }

    void push_back(Element&& elem) {
        elements_.push_back(std::move(elem));
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        elements_.emplace_back(std::forward<Args>(args)...);
    }

    // Iterators
    auto begin() noexcept { return elements_.begin(); }
    auto end() noexcept { return elements_.end(); }
    auto begin() const noexcept { return elements_.begin(); }
    auto end() const noexcept { return elements_.end(); }

    // Value access
    [[nodiscard]] value_type value() const {
        value_type result;
        result.reserve(size());
        for (const auto& elem : elements_) {
            result.push_back(elem.value());
        }
        return result;
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedVector& p, Sink& sink) requires BitSink<Sink> {
        LengthCodec::encode(static_cast<uint32_t>(p.size()), sink);
        for (const auto& elem : p.elements_) {
            Element::encode(elem, sink);
        }
    }

    template<typename Source>
    static PackedVector decode(Source& source) requires BitSource<Source> {
        auto size = LengthCodec::template decode<uint32_t>(source);
        PackedVector result;
        result.reserve(size);
        for (uint32_t i = 0; i < size; ++i) {
            result.push_back(Element::decode(source));
        }
        return result;
    }
};

// ============================================================
//  Packed Buffer - The ultimate container
// ============================================================

class PackedBuffer {
private:
    std::vector<uint8_t> data_;
    size_t bit_size_ = 0;

public:
    PackedBuffer() = default;

    // Reserve space
    void reserve(size_t bytes) { data_.reserve(bytes); }

    // Clear buffer
    void clear() noexcept {
        data_.clear();
        bit_size_ = 0;
    }

    // Size queries
    [[nodiscard]] size_t size_bytes() const noexcept {
        return data_.size();
    }

    [[nodiscard]] size_t size_bits() const noexcept {
        return bit_size_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return data_.empty();
    }

    // Write a packed value
    template<PackedValue P>
    PackedBuffer& operator<<(const P& packed) {
        BitWriter writer(data_.data() + data_.size());
        P::encode(packed, writer);

        // Update size
        size_t bytes_written = writer.position() - (data_.data() + data_.size());
        size_t new_bits = bytes_written * 8;

        if (writer.bytes_written() > 0) {
            data_.push_back(0);  // Add partial byte
            new_bits = (bytes_written - 1) * 8 + (8 - __builtin_clz(data_.back()));
        }

        data_.resize(data_.size() + bytes_written);
        bit_size_ += new_bits;

        return *this;
    }

    // Read a packed value
    template<PackedValue P>
    P extract() const {
        BitReader reader(data_);
        return P::decode(reader);
    }

    // Access raw data
    [[nodiscard]] std::span<const uint8_t> data() const noexcept {
        return data_;
    }

    [[nodiscard]] std::span<uint8_t> data() noexcept {
        return data_;
    }
};

} // namespace pfc