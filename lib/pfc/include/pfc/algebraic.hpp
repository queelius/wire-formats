#pragma once
// algebraic.hpp - Full algebraic type system for packed values
// Sum types, recursive types, and complete algebraic structures
// Following the mathematical foundations of type theory

#include "core.hpp"
#include "packed.hpp"
#include <variant>
#include <memory>

namespace pfc {

// ============================================================
//  Unit Type - The identity of product types
// ============================================================

struct Unit {
    using value_type = std::monostate;

    [[nodiscard]] constexpr value_type value() const noexcept {
        return std::monostate{};
    }

    template<typename Sink>
    static void encode(const Unit&, Sink&) requires BitSink<Sink> {
        // Unit carries no information - encode nothing
    }

    template<typename Source>
    static Unit decode(Source&) requires BitSource<Source> {
        return Unit{};
    }

    // Unit is the identity element for products
    friend constexpr bool operator==(const Unit&, const Unit&) noexcept {
        return true;
    }
};

// ============================================================
//  Bottom Type - The impossible value (for error handling)
// ============================================================

template<typename T>
class Bottom {
public:
    using value_type = T;

    // Bottom can never be constructed with a value
    Bottom() = delete;

    // But we need a way to signal errors
    [[noreturn]] static Bottom error(const char* msg) {
        throw std::runtime_error(msg);
    }

    // These should never be called
    [[noreturn]] value_type value() const {
        std::terminate();
    }

    template<typename Sink>
    [[noreturn]] static void encode(const Bottom&, Sink&) {
        std::terminate();
    }

    template<typename Source>
    [[noreturn]] static Bottom decode(Source&) {
        std::terminate();
    }
};

// ============================================================
//  Sum Types - Discriminated unions with perfect type safety
// ============================================================

template<PackedValue... Types>
class PackedVariant {
public:
    using value_type = std::variant<typename Types::value_type...>;
    static constexpr size_t type_count = sizeof...(Types);

private:
    std::variant<Types...> data_;

    // Helper to encode the variant
    template<typename Sink>
    struct EncodeVisitor {
        Sink& sink;

        template<typename T>
        void operator()(const T& value) const {
            T::encode(value, sink);
        }
    };

    // Helper to get index bits needed
    static constexpr size_t index_bits() {
        if (type_count <= 2) return 1;
        if (type_count <= 4) return 2;
        if (type_count <= 8) return 3;
        if (type_count <= 16) return 4;
        if (type_count <= 32) return 5;
        if (type_count <= 64) return 6;
        return 7;  // Up to 128 variants
    }

public:
    // Constructors
    PackedVariant() = default;

    template<typename T>
    explicit PackedVariant(T&& value) : data_(std::forward<T>(value)) {}

    // Type-safe construction from value_type
    explicit PackedVariant(const value_type& val) {
        std::visit([this](const auto& v) {
            using V = std::decay_t<decltype(v)>;
            // Find matching packed type
            set_variant<0, V>(v);
        }, val);
    }

private:
    template<size_t I, typename V>
    void set_variant(const V& v) {
        if constexpr (I < sizeof...(Types)) {
            using T = std::tuple_element_t<I, std::tuple<Types...>>;
            if constexpr (std::is_same_v<typename T::value_type, V>) {
                data_.template emplace<T>(v);
            } else {
                set_variant<I + 1, V>(v);
            }
        }
    }

public:
    // Visitor interface
    template<typename Visitor>
    auto visit(Visitor&& vis) const {
        return std::visit(std::forward<Visitor>(vis), data_);
    }

    template<typename Visitor>
    auto visit(Visitor&& vis) {
        return std::visit(std::forward<Visitor>(vis), data_);
    }

    // Query which type is active
    [[nodiscard]] size_t index() const noexcept {
        return data_.index();
    }

    // Try to get a specific type
    template<typename T>
    [[nodiscard]] const T* get_if() const noexcept {
        return std::get_if<T>(&data_);
    }

    template<typename T>
    [[nodiscard]] T* get_if() noexcept {
        return std::get_if<T>(&data_);
    }

    // Value access
    [[nodiscard]] value_type value() const {
        return visit([](const auto& packed) -> value_type {
            return packed.value();
        });
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedVariant& var, Sink& sink) requires BitSink<Sink> {
        // Encode type index using minimal bits
        size_t idx = var.index();
        for (size_t i = 0; i < index_bits(); ++i) {
            sink.write((idx >> i) & 1);
        }

        // Encode the value
        std::visit(EncodeVisitor<Sink>{sink}, var.data_);
    }

    template<typename Source>
    static PackedVariant decode(Source& source) requires BitSource<Source> {
        // Decode type index
        size_t idx = 0;
        for (size_t i = 0; i < index_bits(); ++i) {
            idx |= static_cast<size_t>(source.read()) << i;
        }

        // Decode the appropriate type
        return decode_index<0>(idx, source);
    }

private:
    template<size_t I, typename Source>
    static PackedVariant decode_index(size_t idx, Source& source) {
        if constexpr (I < sizeof...(Types)) {
            if (I == idx) {
                using T = std::tuple_element_t<I, std::tuple<Types...>>;
                return PackedVariant{T::decode(source)};
            } else {
                return decode_index<I + 1>(idx, source);
            }
        } else {
            throw std::runtime_error("Invalid variant index");
        }
    }
};

// ============================================================
//  Packed Shared Pointer - For recursive structures
// ============================================================

template<typename T>
class PackedSharedPtr {
public:
    using value_type = std::shared_ptr<T>;

private:
    value_type ptr_;

public:
    PackedSharedPtr() = default;
    explicit PackedSharedPtr(const value_type& p) : ptr_(p) {}
    explicit PackedSharedPtr(value_type&& p) : ptr_(std::move(p)) {}

    [[nodiscard]] const value_type& value() const noexcept { return ptr_; }
    [[nodiscard]] value_type& value() noexcept { return ptr_; }

    // Serialization - we serialize the pointed-to object if it exists
    template<typename Sink>
    static void encode(const PackedSharedPtr& p, Sink& sink) requires BitSink<Sink> {
        codecs::Boolean::encode(p.ptr_ != nullptr, sink);
        if (p.ptr_) {
            T::encode(*p.ptr_, sink);
        }
    }

    template<typename Source>
    static PackedSharedPtr decode(Source& source) requires BitSource<Source> {
        bool has_value = codecs::Boolean::decode(source);
        if (has_value) {
            return PackedSharedPtr{std::make_shared<T>(T::decode(source))};
        }
        return PackedSharedPtr{};
    }
};

// ============================================================
//  Recursive Types - Lists and Trees
// ============================================================

// Forward declaration for recursive list
template<PackedValue Element>
class PackedList;

// List node - either empty or cons cell
template<PackedValue Element>
using PackedListNode = PackedVariant<
    Unit,  // Nil
    PackedPair<Element, PackedSharedPtr<PackedList<Element>>>  // Cons
>;

// Packed list - a recursive algebraic data type
template<PackedValue Element>
class PackedList {
public:
    using value_type = std::vector<typename Element::value_type>;
    using element_type = Element;

private:
    PackedListNode<Element> node_;

public:
    // Constructors
    PackedList() : node_(Unit{}) {}  // Empty list

    PackedList(const Element& head, std::shared_ptr<PackedList> tail)
        : node_(PackedPair<Element, PackedSharedPtr<PackedList>>{
            head, PackedSharedPtr<PackedList>{tail}}) {}

    // Build from vector
    explicit PackedList(const value_type& vec) {
        if (vec.empty()) {
            node_ = PackedListNode<Element>{Unit{}};
        } else {
            // Build list from back to front
            std::shared_ptr<PackedList> tail = std::make_shared<PackedList>();
            for (auto it = vec.rbegin(); it != vec.rend(); ++it) {
                auto new_list = std::make_shared<PackedList>();
                new_list->node_ = PackedListNode<Element>{
                    PackedPair<Element, PackedSharedPtr<PackedList>>{
                        Element{*it},
                        PackedSharedPtr<PackedList>{tail}
                    }
                };
                tail = new_list;
            }
            *this = *tail;
        }
    }

    // Query operations
    [[nodiscard]] bool empty() const noexcept {
        return node_.index() == 0;  // Unit is at index 0
    }

    [[nodiscard]] const Element* head() const {
        if (auto* cons = node_.template get_if<PackedPair<Element, PackedSharedPtr<PackedList>>>()) {
            return &cons->first();
        }
        return nullptr;
    }

    [[nodiscard]] std::shared_ptr<PackedList> tail() const {
        if (auto* cons = node_.template get_if<PackedPair<Element, PackedSharedPtr<PackedList>>>()) {
            return cons->second().value();
        }
        return nullptr;
    }

    // Convert to vector
    [[nodiscard]] value_type value() const {
        value_type result;
        const PackedList* current = this;

        while (!current->empty()) {
            if (auto* h = current->head()) {
                result.push_back(h->value());
            }
            if (auto t = current->tail()) {
                current = t.get();
            } else {
                break;
            }
        }

        return result;
    }

    // Functional operations
    template<typename F>
    auto map(F&& f) const {
        using NewElement = Packed<decltype(f(std::declval<typename Element::value_type>())),
                                  typename Element::codec_type>;

        if (empty()) {
            return PackedList<NewElement>{};
        }

        NewElement new_head{f(head()->value())};
        auto new_tail = tail()->map(f);
        return PackedList<NewElement>{new_head, std::make_shared<PackedList<NewElement>>(new_tail)};
    }

    template<typename F>
    void for_each(F&& f) const {
        if (!empty()) {
            f(*head());
            if (auto t = tail()) {
                t->for_each(f);
            }
        }
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedList& list, Sink& sink) requires BitSink<Sink> {
        PackedListNode<Element>::encode(list.node_, sink);
    }

    template<typename Source>
    static PackedList decode(Source& source) requires BitSource<Source> {
        PackedList result;
        result.node_ = PackedListNode<Element>::decode(source);
        return result;
    }
};

// ============================================================
//  Binary Tree - Another recursive type
// ============================================================

template<PackedValue Value>
class PackedTree;

template<PackedValue Value>
using PackedTreeNode = PackedVariant<
    Unit,  // Leaf
    PackedTuple<Value,
                PackedSharedPtr<PackedTree<Value>>,
                PackedSharedPtr<PackedTree<Value>>>  // Branch
>;

template<PackedValue Value>
class PackedTree {
public:
    using value_type = Value;

private:
    PackedTreeNode<Value> node_;

public:
    // Constructors
    PackedTree() : node_(Unit{}) {}  // Empty tree

    PackedTree(const Value& val,
               std::shared_ptr<PackedTree> left,
               std::shared_ptr<PackedTree> right) {
        using Branch = PackedTuple<Value,
                                   PackedSharedPtr<PackedTree>,
                                   PackedSharedPtr<PackedTree>>;
        node_ = PackedTreeNode<Value>{Branch{val,
                      PackedSharedPtr<PackedTree>{left},
                      PackedSharedPtr<PackedTree>{right}}};
    }

    // Query operations
    [[nodiscard]] bool empty() const noexcept {
        return node_.index() == 0;
    }

    [[nodiscard]] const Value* value() const {
        using Branch = PackedTuple<Value,
                                   PackedSharedPtr<PackedTree>,
                                   PackedSharedPtr<PackedTree>>;
        if (auto* branch = node_.template get_if<Branch>()) {
            return &branch->template get<0>();
        }
        return nullptr;
    }

    [[nodiscard]] std::shared_ptr<PackedTree> left() const {
        using Branch = PackedTuple<Value,
                                   PackedSharedPtr<PackedTree>,
                                   PackedSharedPtr<PackedTree>>;
        if (auto* branch = node_.template get_if<Branch>()) {
            return branch->template get<1>().value();
        }
        return nullptr;
    }

    [[nodiscard]] std::shared_ptr<PackedTree> right() const {
        using Branch = PackedTuple<Value,
                                   PackedSharedPtr<PackedTree>,
                                   PackedSharedPtr<PackedTree>>;
        if (auto* branch = node_.template get_if<Branch>()) {
            return branch->template get<2>().value();
        }
        return nullptr;
    }

    // Tree traversals
    template<typename F>
    void inorder(F&& f) const {
        if (!empty()) {
            if (auto l = left()) l->inorder(f);
            f(*value());
            if (auto r = right()) r->inorder(f);
        }
    }

    template<typename F>
    void preorder(F&& f) const {
        if (!empty()) {
            f(*value());
            if (auto l = left()) l->preorder(f);
            if (auto r = right()) r->preorder(f);
        }
    }

    // Serialization
    template<typename Sink>
    static void encode(const PackedTree& tree, Sink& sink) requires BitSink<Sink> {
        PackedTreeNode<Value>::encode(tree.node_, sink);
    }

    template<typename Source>
    static PackedTree decode(Source& source) requires BitSource<Source> {
        PackedTree result;
        result.node_ = PackedTreeNode<Value>::decode(source);
        return result;
    }
};

// ============================================================
//  Type Aliases for Common Algebraic Types
// ============================================================

// Either type (Result/Error handling)
template<PackedValue Left, PackedValue Right>
using PackedEither = PackedVariant<Left, Right>;

// Maybe type (Option)
template<PackedValue T>
using PackedMaybe = PackedVariant<Unit, T>;

// Result type for error handling
template<PackedValue T, PackedValue E = Packed<std::string, codecs::EliasGamma>>
using PackedResult = PackedEither<E, T>;  // Error on left, value on right

} // namespace pfc