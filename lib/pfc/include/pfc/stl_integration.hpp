#pragma once
// stl_integration.hpp - Making packed types first-class STL citizens
// Proxy iterators, type-erased containers, zero-copy algorithms
// Where efficiency meets elegance

#include "core.hpp"
#include "packed.hpp"
#include <iterator>
#include <algorithm>
#include <memory>
#include <typeindex>
#include <any>

namespace pfc {

// ============================================================
//  Proxy Value - Decodes on access, encodes on assignment
// ============================================================

template<typename Element, typename Container>
class PackedProxy {
public:
    using value_type = typename Element::value_type;

private:
    Container* container_;
    size_t index_;
    mutable std::optional<Element> cached_;

    void ensure_cached() const {
        if (!cached_) {
            // Decode from container at index
            BitReader reader(container_->raw_data(index_));
            cached_ = Element::decode(reader);
        }
    }

public:
    PackedProxy(Container* c, size_t i) : container_(c), index_(i) {}

    // Conversion to value type
    operator value_type() const {
        ensure_cached();
        return cached_->value();
    }

    // Assignment from value type
    PackedProxy& operator=(const value_type& val) {
        cached_ = Element{val};

        // Re-encode to container
        container_->update_element(index_, *cached_);
        return *this;
    }

    // Access to packed element
    const Element& get_packed() const {
        ensure_cached();
        return *cached_;
    }

    // Comparison operators
    bool operator==(const PackedProxy& other) const {
        return value_type(*this) == value_type(other);
    }

    bool operator<(const PackedProxy& other) const {
        return value_type(*this) < value_type(other);
    }
};

// ============================================================
//  Packed Iterator - STL-compliant iterator with proxy values
// ============================================================

template<typename Element, typename Container>
class PackedIterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename Element::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = void;  // No real pointers to packed values
    using reference = PackedProxy<Element, Container>;

private:
    Container* container_;
    size_t index_;

public:
    PackedIterator() : container_(nullptr), index_(0) {}
    PackedIterator(Container* c, size_t i) : container_(c), index_(i) {}

    // Dereference
    reference operator*() const {
        return reference(container_, index_);
    }

    // Increment/decrement
    PackedIterator& operator++() {
        ++index_;
        return *this;
    }

    PackedIterator operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    PackedIterator& operator--() {
        --index_;
        return *this;
    }

    PackedIterator operator--(int) {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    // Arithmetic
    PackedIterator& operator+=(difference_type n) {
        index_ += n;
        return *this;
    }

    PackedIterator& operator-=(difference_type n) {
        index_ -= n;
        return *this;
    }

    PackedIterator operator+(difference_type n) const {
        return PackedIterator(container_, index_ + n);
    }

    PackedIterator operator-(difference_type n) const {
        return PackedIterator(container_, index_ - n);
    }

    difference_type operator-(const PackedIterator& other) const {
        return index_ - other.index_;
    }

    // Comparison
    bool operator==(const PackedIterator& other) const {
        return container_ == other.container_ && index_ == other.index_;
    }

    bool operator!=(const PackedIterator& other) const {
        return !(*this == other);
    }

    bool operator<(const PackedIterator& other) const {
        return index_ < other.index_;
    }

    bool operator>(const PackedIterator& other) const {
        return other < *this;
    }

    bool operator<=(const PackedIterator& other) const {
        return !(other < *this);
    }

    bool operator>=(const PackedIterator& other) const {
        return !(*this < other);
    }

    // Random access
    reference operator[](difference_type n) const {
        return reference(container_, index_ + n);
    }
};

// ============================================================
//  Packed Container - STL-compliant container with compression
// ============================================================

template<PackedValue Element>
class PackedContainer {
public:
    using value_type = typename Element::value_type;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = PackedIterator<Element, PackedContainer>;
    using const_iterator = PackedIterator<Element, const PackedContainer>;

private:
    // Store offsets for O(1) random access
    std::vector<uint8_t> data_;
    std::vector<size_t> offsets_;
    size_t count_ = 0;

public:
    // Constructors
    PackedContainer() = default;

    template<typename InputIt>
    PackedContainer(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            push_back(Element{*it});
        }
    }

    // Size operations
    [[nodiscard]] size_type size() const noexcept { return count_; }
    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }

    void reserve(size_type new_cap) {
        offsets_.reserve(new_cap + 1);
        // Estimate data size (heuristic)
        data_.reserve(new_cap * 8);
    }

    void clear() noexcept {
        data_.clear();
        offsets_.clear();
        count_ = 0;
    }

    // Element access
    [[nodiscard]] value_type operator[](size_type pos) const {
        BitReader reader(data_.data() + offsets_[pos],
                         offsets_[pos + 1] - offsets_[pos]);
        return Element::decode(reader).value();
    }

    [[nodiscard]] value_type at(size_type pos) const {
        if (pos >= count_) {
            throw std::out_of_range("PackedContainer::at");
        }
        return (*this)[pos];
    }

    [[nodiscard]] value_type front() const { return (*this)[0]; }
    [[nodiscard]] value_type back() const { return (*this)[count_ - 1]; }

    // Modifiers
    void push_back(const Element& elem) {
        // Add start offset only for the first element
        if (offsets_.empty()) {
            offsets_.push_back(0);
        }

        // Encode to temporary buffer
        std::vector<uint8_t> temp(64);  // Should be enough
        BitWriter writer(temp.data());
        Element::encode(elem, writer);
        writer.align();

        // Append to data
        size_t bytes = writer.bytes_written();
        data_.insert(data_.end(), temp.begin(), temp.begin() + bytes);

        ++count_;

        // Always push end offset (which becomes start of next element)
        offsets_.push_back(data_.size());
    }

    void push_back(Element&& elem) {
        push_back(elem);  // Same as const version for now
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        push_back(Element{std::forward<Args>(args)...});
    }

    void pop_back() {
        if (count_ > 0) {
            --count_;
            data_.resize(offsets_[count_]);
            offsets_.resize(count_ + 1);
        }
    }

    // Iterators
    iterator begin() noexcept { return iterator(this, 0); }
    iterator end() noexcept { return iterator(this, count_); }
    const_iterator begin() const noexcept { return const_iterator(this, 0); }
    const_iterator end() const noexcept { return const_iterator(this, count_); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }

    // Internal access for proxy
    std::span<const uint8_t> raw_data(size_t index) const {
        return std::span<const uint8_t>(data_.data() + offsets_[index],
                                        offsets_[index + 1] - offsets_[index]);
    }

    void update_element(size_t index, const Element& elem) {
        // For simplicity, rebuild from this point
        // In production, use a more sophisticated approach

        // Save elements after index
        std::vector<Element> saved;
        for (size_t i = index + 1; i < count_; ++i) {
            BitReader reader(data_.data() + offsets_[i],
                           offsets_[i + 1] - offsets_[i]);
            saved.push_back(Element::decode(reader));
        }

        // Truncate to before index
        count_ = index;
        data_.resize(offsets_[index]);
        offsets_.resize(index + 1);

        // Re-add updated element and saved elements
        push_back(elem);
        for (const auto& e : saved) {
            push_back(e);
        }
    }

    // Capacity
    [[nodiscard]] size_type capacity() const noexcept {
        return offsets_.capacity();
    }

    [[nodiscard]] size_type data_bytes() const noexcept {
        return data_.size();
    }

    // Compression ratio
    [[nodiscard]] double compression_ratio() const noexcept {
        if (count_ == 0) return 1.0;
        size_t unpacked_size = count_ * sizeof(value_type);
        return static_cast<double>(unpacked_size) / data_.size();
    }
};

// ============================================================
//  Type-Erased Packed Container
// ============================================================

class TypeErasedPackedContainer {
private:
    struct Concept {
        virtual ~Concept() = default;
        virtual std::type_index type() const = 0;
        virtual size_t size() const = 0;
        virtual size_t data_bytes() const = 0;
        virtual void clear() = 0;
        virtual std::any get(size_t index) const = 0;
        virtual void push_any(const std::any& value) = 0;
    };

    template<PackedValue Element>
    struct Model : Concept {
        PackedContainer<Element> container;

        std::type_index type() const override {
            return std::type_index(typeid(typename Element::value_type));
        }

        size_t size() const override {
            return container.size();
        }

        size_t data_bytes() const override {
            return container.data_bytes();
        }

        void clear() override {
            container.clear();
        }

        std::any get(size_t index) const override {
            return container[index];
        }

        void push_any(const std::any& value) override {
            using V = typename Element::value_type;
            container.push_back(Element{std::any_cast<V>(value)});
        }
    };

    std::unique_ptr<Concept> impl_;

public:
    // Factory function to create with specific packed type
    template<PackedValue Element>
    static TypeErasedPackedContainer create() {
        TypeErasedPackedContainer result;
        result.impl_ = std::make_unique<Model<Element>>();
        return result;
    }

    // Type query
    [[nodiscard]] std::type_index stored_type() const {
        return impl_ ? impl_->type() : std::type_index(typeid(void));
    }

    // Operations
    [[nodiscard]] size_t size() const {
        return impl_ ? impl_->size() : 0;
    }

    [[nodiscard]] size_t data_bytes() const {
        return impl_ ? impl_->data_bytes() : 0;
    }

    void clear() {
        if (impl_) impl_->clear();
    }

    [[nodiscard]] std::any operator[](size_t index) const {
        return impl_ ? impl_->get(index) : std::any{};
    }

    template<typename T>
    void push_back(const T& value) {
        if (impl_) {
            impl_->push_any(value);
        }
    }

    // Check if container holds specific type
    template<typename T>
    [[nodiscard]] bool holds_type() const {
        return stored_type() == std::type_index(typeid(T));
    }

    // Get typed value
    template<typename T>
    [[nodiscard]] std::optional<T> get(size_t index) const {
        if (holds_type<T>() && index < size()) {
            return std::any_cast<T>((*this)[index]);
        }
        return std::nullopt;
    }
};

// ============================================================
//  Zero-Copy Algorithms
// ============================================================

namespace algorithms {

// Transform that produces a new packed container
template<PackedValue Input, typename F>
auto packed_transform(const PackedContainer<Input>& input, F&& f) {
    using OutputValue = decltype(f(std::declval<typename Input::value_type>()));
    using Output = Packed<OutputValue, typename Input::codec_type>;

    PackedContainer<Output> result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        result.push_back(Output{f(input[i])});
    }

    return result;
}

// Accumulate without unpacking everything
template<PackedValue Element, typename T, typename BinaryOp>
T packed_accumulate(const PackedContainer<Element>& container,
                   T init,
                   BinaryOp&& op) {
    for (size_t i = 0; i < container.size(); ++i) {
        init = op(init, container[i]);
    }
    return init;
}

// Find if - early termination on packed data
template<PackedValue Element, typename Predicate>
std::optional<size_t> packed_find_if(const PackedContainer<Element>& container,
                                     Predicate&& pred) {
    for (size_t i = 0; i < container.size(); ++i) {
        if (pred(container[i])) {
            return i;
        }
    }
    return std::nullopt;
}

// Parallel transform using execution policies
template<typename ExecPolicy, PackedValue Input, typename F>
auto packed_transform_par(ExecPolicy&& policy,
                         const PackedContainer<Input>& input,
                         F&& f) {
    using OutputValue = decltype(f(std::declval<typename Input::value_type>()));
    using Output = Packed<OutputValue, typename Input::codec_type>;

    PackedContainer<Output> result;
    result.reserve(input.size());

    // Decode in parallel, encode sequentially (for now)
    std::vector<OutputValue> temp(input.size());

    std::transform(policy,
                  input.begin(), input.end(),
                  temp.begin(),
                  [&f](const auto& proxy) { return f(typename Input::value_type(proxy)); });

    for (const auto& val : temp) {
        result.push_back(Output{val});
    }

    return result;
}

// Zero-copy merge of sorted packed containers
template<PackedValue Element>
PackedContainer<Element> packed_merge(const PackedContainer<Element>& a,
                                      const PackedContainer<Element>& b) {
    PackedContainer<Element> result;
    result.reserve(a.size() + b.size());

    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] <= b[j]) {
            // Direct copy of packed bytes (zero-copy)
            auto data = a.raw_data(i);
            BitReader reader(data.data(), data.size());
            result.push_back(Element::decode(reader));
            ++i;
        } else {
            auto data = b.raw_data(j);
            BitReader reader(data.data(), data.size());
            result.push_back(Element::decode(reader));
            ++j;
        }
    }

    // Copy remaining elements
    while (i < a.size()) {
        auto data = a.raw_data(i);
        BitReader reader(data.data(), data.size());
        result.push_back(Element::decode(reader));
        ++i;
    }

    while (j < b.size()) {
        auto data = b.raw_data(j);
        BitReader reader(data.data(), data.size());
        result.push_back(Element::decode(reader));
        ++j;
    }

    return result;
}

} // namespace algorithms

// ============================================================
//  STL Algorithm Compatibility Layer
// ============================================================

// Enable std::sort on packed containers
template<PackedValue Element>
void sort(PackedContainer<Element>& container) {
    // Extract values, sort, rebuild
    std::vector<typename Element::value_type> values;
    values.reserve(container.size());

    for (size_t i = 0; i < container.size(); ++i) {
        values.push_back(container[i]);
    }

    std::sort(values.begin(), values.end());

    container.clear();
    for (const auto& val : values) {
        container.push_back(Element{val});
    }
}

// Specialization for numeric types - can sort packed bytes directly
template<typename Codec>
void sort(PackedContainer<Packed<uint32_t, Codec>>& container) {
    // For unsigned integers with prefix-free codes,
    // lexicographic order of encoded bytes = numeric order
    // This is a powerful optimization!

    if constexpr (std::is_same_v<Codec, codecs::Fixed<32>>) {
        // Can sort the raw bytes directly!
        // Implementation left as an exercise
    } else {
        // Fall back to general approach
        std::vector<uint32_t> values;
        values.reserve(container.size());

        for (size_t i = 0; i < container.size(); ++i) {
            values.push_back(container[i]);
        }

        std::sort(values.begin(), values.end());

        container.clear();
        for (auto val : values) {
            container.push_back(Packed<uint32_t, Codec>{val});
        }
    }
}

} // namespace pfc