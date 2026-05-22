#ifndef SIMPLE_JSON_HPP
#define SIMPLE_JSON_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <expected>
#include <variant>
#include <vector>
#include <map>
#include <istream>
#include <ostream>
#include <sstream>
#include <format>
#include <type_traits>
#include <concepts>
#include <ranges>
#include <iterator>
#include <tuple>
#include <utility>
#include <optional>
#include <memory>
#include <meta>

namespace sj {

// ============================================================================
// Error types
// ============================================================================

enum class error_kind {
    parse,
    type,
    missing,
    conversion,
    overflow,
};

struct error {
    error_kind kind;
    std::string message;
    std::string path;

    error with_context(std::string_view field) const {
        return error{kind, message,
            path.empty() ? std::string(field)
                         : std::format("{}.{}", field, path)};
    }
};

// ============================================================================
// value_t enum
// ============================================================================

enum class value_t {
    null,       // index 0
    boolean,    // index 1
    integer,    // index 2
    number,     // index 3
    string,     // index 4
    array,      // index 5
    object,     // index 6
};

// ============================================================================
// Forward declarations
// ============================================================================

class json;

// Forward-declare to_json / from_json so concepts can reference them
inline json to_json(json const& j);
inline json to_json(std::nullptr_t);
inline json to_json(bool b);
inline json to_json(std::string const& s);
inline json to_json(std::string_view sv);
inline json to_json(char const* s);

template <std::integral T> json to_json(T n);
template <std::floating_point T> json to_json(T n);

template <typename T>
    requires std::is_aggregate_v<T>
          && (!std::is_array_v<T>)
          && (!std::ranges::input_range<T>)
          && (!std::is_same_v<std::decay_t<T>, json>)
json to_json(T const& obj);

template <typename E>
    requires std::is_enum_v<E>
json to_json(E e);

template <typename T> json to_json(std::optional<T> const& opt);
template <typename T> json to_json(std::unique_ptr<T> const& p);
template <typename T> json to_json(std::shared_ptr<T> const& p);
template <typename... Ts> json to_json(std::tuple<Ts...> const& t);
template <typename A, typename B> json to_json(std::pair<A, B> const& p);
template <typename... Ts> json to_json(std::variant<Ts...> const& v);

inline std::expected<json, error> from_json_impl(json const& j, std::type_identity<json>);
inline std::expected<bool, error> from_json_impl(json const& j, std::type_identity<bool>);
inline std::expected<std::string, error> from_json_impl(json const& j, std::type_identity<std::string>);
inline std::expected<std::nullptr_t, error> from_json_impl(json const& j, std::type_identity<std::nullptr_t>);

template <std::integral T>
std::expected<T, error> from_json_impl(json const& j, std::type_identity<T>);

template <std::floating_point T>
std::expected<T, error> from_json_impl(json const& j, std::type_identity<T>);

template <typename T>
    requires std::is_aggregate_v<T>
          && (!std::is_array_v<T>)
          && (!std::ranges::input_range<T>)
          && (!std::is_same_v<std::decay_t<T>, json>)
std::expected<T, error> from_json_impl(json const& j, std::type_identity<T>);

template <typename E>
    requires std::is_enum_v<E>
std::expected<E, error> from_json_impl(json const& j, std::type_identity<E>);

template <typename T>
std::expected<std::optional<T>, error> from_json_impl(json const& j, std::type_identity<std::optional<T>>);

template <typename T>
std::expected<std::unique_ptr<T>, error> from_json_impl(json const& j, std::type_identity<std::unique_ptr<T>>);

template <typename T>
std::expected<std::shared_ptr<T>, error> from_json_impl(json const& j, std::type_identity<std::shared_ptr<T>>);

template <typename... Ts>
std::expected<std::tuple<Ts...>, error> from_json_impl(json const& j, std::type_identity<std::tuple<Ts...>>);

template <typename A, typename B>
std::expected<std::pair<A, B>, error> from_json_impl(json const& j, std::type_identity<std::pair<A, B>>);

template <typename... Ts>
std::expected<std::variant<Ts...>, error> from_json_impl(json const& j, std::type_identity<std::variant<Ts...>>);

// Unified from_json entry point
template <typename T>
std::expected<T, error> from_json(json const& j) {
    return from_json_impl(j, std::type_identity<T>{});
}

// ============================================================================
// Detail helpers
// ============================================================================

namespace detail {

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_optional_v = is_optional<T>::value;

template <typename T, typename = void>
struct is_pair_like : std::false_type {};

template <typename T>
struct is_pair_like<T, std::void_t<typename T::first_type, typename T::second_type>>
    : std::true_type {};

template <typename K>
concept StringConvertible = std::constructible_from<std::string, K>;

} // namespace detail

// ============================================================================
// Concept system
// ============================================================================

// Container detection concepts
template <typename C>
concept SequenceSerializable =
    std::ranges::input_range<C> &&
    !std::is_same_v<std::decay_t<C>, json> &&
    !std::is_same_v<std::decay_t<C>, std::string> &&
    !detail::is_pair_like<std::ranges::range_value_t<C>>::value;

template <typename C>
concept MappingSerializable =
    std::ranges::input_range<C> &&
    !std::is_same_v<std::decay_t<C>, json> &&
    detail::is_pair_like<std::ranges::range_value_t<C>>::value &&
    detail::StringConvertible<typename std::ranges::range_value_t<C>::first_type>;

template <typename C>
concept BackInsertable = requires(C& c, std::ranges::range_value_t<C> v) {
    c.push_back(std::move(v));
};

template <typename C>
concept Insertable = requires(C& c, std::ranges::range_value_t<C> v) {
    c.insert(std::move(v));
};

template <typename C>
concept FixedSizeSequence = requires { std::tuple_size<C>::value; }
    && std::ranges::input_range<C>;

// Serializable sequence containers for to_json
template <SequenceSerializable C> json to_json(C const& seq);
template <MappingSerializable C> json to_json(C const& m);

// Deserializable containers via from_json_impl
template <typename C>
    requires SequenceSerializable<C> && BackInsertable<C>
             && (!FixedSizeSequence<C>)
std::expected<C, error> from_json_impl(json const& j, std::type_identity<C>);

template <typename C>
    requires SequenceSerializable<C> && Insertable<C>
             && (!BackInsertable<C>) && (!FixedSizeSequence<C>)
std::expected<C, error> from_json_impl(json const& j, std::type_identity<C>);

template <typename C>
    requires MappingSerializable<C>
std::expected<C, error> from_json_impl(json const& j, std::type_identity<C>);

template <typename T, std::size_t N>
std::expected<std::array<T, N>, error> from_json_impl(json const& j, std::type_identity<std::array<T, N>>);

// ============================================================================
// json class
// ============================================================================

class json {
public:
    using null_type    = std::nullptr_t;
    using boolean_type = bool;
    using integer_type = std::int64_t;
    using number_type  = double;
    using string_type  = std::string;
    using array_type   = std::vector<json>;
    using object_type  = std::map<std::string, json>;
    using value_type   = std::variant<
        null_type,
        boolean_type,
        integer_type,
        number_type,
        string_type,
        array_type,
        object_type>;

    // -- Iterators ----------------------------------------------------------

    class iterator {
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = json;
        using pointer           = json*;
        using reference         = json&;
        using iterator_category = std::bidirectional_iterator_tag;

        iterator() = default;

        reference operator*() const {
            if (std::holds_alternative<array_iter>(it_))
                return *std::get<array_iter>(it_);
            return std::get<object_iter>(it_)->second;
        }
        pointer operator->() const { return &**this; }

        iterator& operator++() {
            if (std::holds_alternative<array_iter>(it_))
                ++std::get<array_iter>(it_);
            else if (std::holds_alternative<object_iter>(it_))
                ++std::get<object_iter>(it_);
            return *this;
        }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }

        iterator& operator--() {
            if (std::holds_alternative<array_iter>(it_))
                --std::get<array_iter>(it_);
            else if (std::holds_alternative<object_iter>(it_))
                --std::get<object_iter>(it_);
            return *this;
        }
        iterator operator--(int) { auto t = *this; --(*this); return t; }

        friend bool operator==(iterator const& a, iterator const& b) = default;

        std::string const& key() const {
            return std::get<object_iter>(it_)->first;
        }
        reference value() const {
            return std::get<object_iter>(it_)->second;
        }

    private:
        friend class json;
        using array_iter  = array_type::iterator;
        using object_iter = object_type::iterator;
        std::variant<std::monostate, array_iter, object_iter> it_;
        explicit iterator(array_iter it)  : it_(it) {}
        explicit iterator(object_iter it) : it_(it) {}
    };

    class const_iterator {
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = json;
        using pointer           = json const*;
        using reference         = json const&;
        using iterator_category = std::bidirectional_iterator_tag;

        const_iterator() = default;

        reference operator*() const {
            if (std::holds_alternative<array_iter>(it_))
                return *std::get<array_iter>(it_);
            return std::get<object_iter>(it_)->second;
        }
        pointer operator->() const { return &**this; }

        const_iterator& operator++() {
            if (std::holds_alternative<array_iter>(it_))
                ++std::get<array_iter>(it_);
            else if (std::holds_alternative<object_iter>(it_))
                ++std::get<object_iter>(it_);
            return *this;
        }
        const_iterator operator++(int) { auto t = *this; ++(*this); return t; }

        const_iterator& operator--() {
            if (std::holds_alternative<array_iter>(it_))
                --std::get<array_iter>(it_);
            else if (std::holds_alternative<object_iter>(it_))
                --std::get<object_iter>(it_);
            return *this;
        }
        const_iterator operator--(int) { auto t = *this; --(*this); return t; }

        friend bool operator==(const_iterator const& a, const_iterator const& b) = default;

        std::string const& key() const {
            return std::get<object_iter>(it_)->first;
        }
        reference value() const {
            return std::get<object_iter>(it_)->second;
        }

    private:
        friend class json;
        using array_iter  = array_type::const_iterator;
        using object_iter = object_type::const_iterator;
        std::variant<std::monostate, array_iter, object_iter> it_;
        explicit const_iterator(array_iter it)  : it_(it) {}
        explicit const_iterator(object_iter it) : it_(it) {}
    };

    // -- items_view ---------------------------------------------------------

    struct item_type {
        std::string const* key_ptr;
        json* value_ptr;
        std::string const& key() const { return *key_ptr; }
        json& value() const { return *value_ptr; }
    };

    class items_view {
    public:
        struct iterator {
            using difference_type   = std::ptrdiff_t;
            using value_type        = item_type;
            using pointer           = void;
            using reference         = item_type;
            using iterator_category = std::forward_iterator_tag;

            iterator() = default;
            explicit iterator(object_type::iterator it) : it_(it) {}

            item_type operator*() const {
                return item_type{&it_->first, &it_->second};
            }
            iterator& operator++() { ++it_; return *this; }
            iterator operator++(int) { auto t = *this; ++(*this); return t; }
            friend bool operator==(iterator const& a, iterator const& b) = default;

        private:
            object_type::iterator it_{};
        };

        explicit items_view(object_type& obj) : obj_(&obj) {}
        iterator begin() { return iterator(obj_->begin()); }
        iterator end()   { return iterator(obj_->end()); }

    private:
        object_type* obj_;
    };

    // -- Constructors -------------------------------------------------------

    json() : value_(nullptr) {}
    json(std::nullptr_t) : value_(nullptr) {}
    json(boolean_type b) : value_(b) {}
    json(integer_type n) : value_(n) {}
    json(int n) : value_(static_cast<integer_type>(n)) {}
    json(number_type n) : value_(n) {}
    json(string_type const& s) : value_(s) {}
    json(string_type&& s) : value_(std::move(s)) {}
    json(char const* s) : value_(string_type{s}) {}
    json(array_type const& a) : value_(a) {}
    json(array_type&& a) : value_(std::move(a)) {}
    json(object_type const& o) : value_(o) {}
    json(object_type&& o) : value_(std::move(o)) {}

    json(std::initializer_list<std::pair<const std::string, json>> init)
        : value_(object_type{}) {
        auto& obj = std::get<object_type>(value_);
        for (auto const& kv : init)
            obj.emplace(kv.first, kv.second);
    }

    // Construct from sequence-like containers
    template <typename Container>
        requires SequenceSerializable<std::decay_t<Container>>
              && (!std::is_same_v<std::decay_t<Container>, array_type>)
    json(Container const& c) : value_(array_type{}) {
        auto& arr = std::get<array_type>(value_);
        for (auto const& el : c) {
            using E = std::decay_t<decltype(el)>;
            if constexpr (std::is_same_v<E, boolean_type>)
                arr.emplace_back(json(el));
            else if constexpr (std::is_integral_v<E>)
                arr.emplace_back(json(static_cast<integer_type>(el)));
            else
                arr.emplace_back(json(el));
        }
    }

    // Construct from mapping containers
    template <typename Container>
        requires MappingSerializable<std::decay_t<Container>>
              && (!std::is_same_v<std::decay_t<Container>, object_type>)
    json(Container const& c) : value_(object_type{}) {
        auto& obj = std::get<object_type>(value_);
        for (auto const& kv : c) {
            std::string key(kv.first);
            using M = std::decay_t<decltype(kv.second)>;
            if constexpr (std::is_same_v<M, boolean_type>)
                obj.emplace(std::move(key), json(kv.second));
            else if constexpr (std::is_integral_v<M>)
                obj.emplace(std::move(key), json(static_cast<integer_type>(kv.second)));
            else
                obj.emplace(std::move(key), json(kv.second));
        }
    }

    json(json const&) = default;
    json(json&&) noexcept = default;
    json& operator=(json const&) = default;
    json& operator=(json&&) noexcept = default;

    // -- Type queries -------------------------------------------------------

    template <typename T>
    bool is() const { return std::holds_alternative<T>(value_); }

    bool is_null()    const { return is<null_type>(); }
    bool is_boolean() const { return is<boolean_type>(); }
    bool is_integer() const { return is<integer_type>(); }
    bool is_number()  const { return is<integer_type>() || is<number_type>(); }
    bool is_string()  const { return is<string_type>(); }
    bool is_array()   const { return is<array_type>(); }
    bool is_object()  const { return is<object_type>(); }

    value_t type() const { return static_cast<value_t>(value_.index()); }

    // -- Accessors (std::expected) ------------------------------------------

    template <typename T>
    std::expected<std::reference_wrapper<T const>, error> as() const {
        if (auto* p = std::get_if<T>(&value_))
            return std::cref(*p);
        return std::unexpected(error{error_kind::type,
            std::format("expected {}", typeid(T).name()), ""});
    }

    // Convenience direct accessors (unchecked, for use after is_X() guard)
    boolean_type as_boolean() const { return std::get<boolean_type>(value_); }
    integer_type as_integer() const { return std::get<integer_type>(value_); }
    number_type as_number() const {
        if (std::holds_alternative<number_type>(value_))
            return std::get<number_type>(value_);
        return static_cast<number_type>(std::get<integer_type>(value_));
    }
    string_type const& as_string() const { return std::get<string_type>(value_); }
    array_type const& as_array() const { return std::get<array_type>(value_); }
    object_type const& as_object() const { return std::get<object_type>(value_); }

    // -- get<T>() returning expected ----------------------------------------

    template <typename T>
    std::expected<T, error> get() const {
        if constexpr (std::is_same_v<T, json>) {
            return *this;
        } else if constexpr (std::is_same_v<T, boolean_type>) {
            if (!is_boolean())
                return std::unexpected(error{error_kind::type, "not a boolean", ""});
            return as_boolean();
        } else if constexpr (std::is_same_v<T, integer_type>) {
            if (is_integer()) return as_integer();
            if (std::holds_alternative<number_type>(value_))
                return static_cast<integer_type>(std::get<number_type>(value_));
            return std::unexpected(error{error_kind::type, "not a number", ""});
        } else if constexpr (std::is_same_v<T, int>) {
            if (is_integer()) return static_cast<int>(as_integer());
            if (std::holds_alternative<number_type>(value_))
                return static_cast<int>(std::get<number_type>(value_));
            return std::unexpected(error{error_kind::type, "not a number", ""});
        } else if constexpr (std::is_same_v<T, number_type>) {
            if (!is_number())
                return std::unexpected(error{error_kind::type, "not a number", ""});
            return as_number();
        } else if constexpr (std::is_same_v<T, string_type>) {
            if (!is_string())
                return std::unexpected(error{error_kind::type, "not a string", ""});
            return as_string();
        } else if constexpr (std::is_same_v<T, array_type>) {
            if (!is_array())
                return std::unexpected(error{error_kind::type, "not an array", ""});
            return as_array();
        } else if constexpr (std::is_same_v<T, object_type>) {
            if (!is_object())
                return std::unexpected(error{error_kind::type, "not an object", ""});
            return as_object();
        } else {
            static_assert(std::is_same_v<T, void>, "unsupported type for get<T>()");
        }
    }

    // -- Convenience accessors ----------------------------------------------

    number_type to_double(number_type default_value = 0.0) const {
        return is_number() ? as_number() : default_value;
    }

    int to_int(int default_value = 0) const {
        if (is_integer()) return static_cast<int>(as_integer());
        if (is_number())  return static_cast<int>(as_number());
        return default_value;
    }

    // -- operator[] ---------------------------------------------------------

    json& operator[](std::string const& key) {
        if (!is_object()) value_ = object_type{};
        return std::get<object_type>(value_)[key];
    }

    json& operator[](char const* key) {
        return (*this)[std::string(key)];
    }

    json& operator[](std::size_t idx) {
        return std::get<array_type>(value_)[idx];
    }

    // -- at() returning expected --------------------------------------------

    std::expected<std::reference_wrapper<json const>, error> at(std::string const& key) const {
        if (!is_object())
            return std::unexpected(error{error_kind::type, "not an object", ""});
        auto const& obj = std::get<object_type>(value_);
        auto it = obj.find(key);
        if (it == obj.end())
            return std::unexpected(error{error_kind::missing,
                std::format("key '{}' not found", key), ""});
        return std::cref(it->second);
    }

    std::expected<std::reference_wrapper<json>, error> at(std::string const& key) {
        if (!is_object())
            return std::unexpected(error{error_kind::type, "not an object", ""});
        auto& obj = std::get<object_type>(value_);
        auto it = obj.find(key);
        if (it == obj.end())
            return std::unexpected(error{error_kind::missing,
                std::format("key '{}' not found", key), ""});
        return std::ref(it->second);
    }

    std::expected<std::reference_wrapper<json const>, error> at(std::size_t idx) const {
        if (!is_array())
            return std::unexpected(error{error_kind::type, "not an array", ""});
        auto const& arr = std::get<array_type>(value_);
        if (idx >= arr.size())
            return std::unexpected(error{error_kind::missing,
                std::format("index {} out of range (size {})", idx, arr.size()), ""});
        return std::cref(arr[idx]);
    }

    std::expected<std::reference_wrapper<json>, error> at(std::size_t idx) {
        if (!is_array())
            return std::unexpected(error{error_kind::type, "not an array", ""});
        auto& arr = std::get<array_type>(value_);
        if (idx >= arr.size())
            return std::unexpected(error{error_kind::missing,
                std::format("index {} out of range (size {})", idx, arr.size()), ""});
        return std::ref(arr[idx]);
    }

    // -- Container operations -----------------------------------------------

    void push_back(json const& v) {
        ensure_array();
        std::get<array_type>(value_).push_back(v);
    }

    void push_back(json&& v) {
        ensure_array();
        std::get<array_type>(value_).push_back(std::move(v));
    }

    template <typename T>
    void push_back(T&& value) {
        emplace_back(std::forward<T>(value));
    }

    template <typename... Args>
    json& emplace_back(Args&&... args) {
        ensure_array();
        auto& arr = std::get<array_type>(value_);
        arr.emplace_back(std::forward<Args>(args)...);
        return arr.back();
    }

    template <typename T>
    json& emplace(std::string key, T&& value) {
        ensure_object();
        auto& obj = std::get<object_type>(value_);
        auto [it, inserted] = obj.emplace(std::move(key), json(std::forward<T>(value)));
        (void)inserted;
        return it->second;
    }

    template <typename T>
    json& emplace(char const* key, T&& value) {
        return emplace(std::string(key), std::forward<T>(value));
    }

    bool contains(std::string const& key) const {
        if (!is_object()) return false;
        auto const& obj = std::get<object_type>(value_);
        return obj.find(key) != obj.end();
    }

    bool contains(char const* key) const {
        return contains(std::string(key));
    }

    bool contains(std::size_t idx) const {
        if (!is_array()) return false;
        return idx < std::get<array_type>(value_).size();
    }

    std::size_t size() const {
        if (is_null())   return 0;
        if (is_array())  return std::get<array_type>(value_).size();
        if (is_object()) return std::get<object_type>(value_).size();
        return 1;
    }

    bool empty() const {
        if (is_null())   return true;
        if (is_array())  return std::get<array_type>(value_).empty();
        if (is_object()) return std::get<object_type>(value_).empty();
        return false;
    }

    void clear() {
        if (is_array())       std::get<array_type>(value_).clear();
        else if (is_object()) std::get<object_type>(value_).clear();
        else                  value_ = nullptr;
    }

    // -- Iterators ----------------------------------------------------------

    iterator begin() {
        if (is_array())  return iterator(std::get<array_type>(value_).begin());
        if (is_object()) return iterator(std::get<object_type>(value_).begin());
        return iterator{};
    }
    iterator end() {
        if (is_array())  return iterator(std::get<array_type>(value_).end());
        if (is_object()) return iterator(std::get<object_type>(value_).end());
        return iterator{};
    }
    const_iterator begin() const {
        if (is_array())  return const_iterator(std::get<array_type>(value_).begin());
        if (is_object()) return const_iterator(std::get<object_type>(value_).begin());
        return const_iterator{};
    }
    const_iterator end() const {
        if (is_array())  return const_iterator(std::get<array_type>(value_).end());
        if (is_object()) return const_iterator(std::get<object_type>(value_).end());
        return const_iterator{};
    }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend()   const { return end(); }

    items_view items() {
        return items_view(std::get<object_type>(value_));
    }

    iterator find(std::string const& key) {
        if (!is_object()) return end();
        return iterator(std::get<object_type>(value_).find(key));
    }
    iterator find(char const* key) { return find(std::string(key)); }

    const_iterator find(std::string const& key) const {
        if (!is_object()) return end();
        return const_iterator(std::get<object_type>(value_).find(key));
    }
    const_iterator find(char const* key) const { return find(std::string(key)); }

    std::size_t count(std::string const& key) const {
        return contains(key) ? 1u : 0u;
    }
    std::size_t count(char const* key) const { return count(std::string(key)); }

    void erase(std::string const& key) {
        if (is_object()) std::get<object_type>(value_).erase(key);
    }
    void erase(char const* key) { erase(std::string(key)); }

    // -- value_or -----------------------------------------------------------

    template <typename T>
    T value_or(std::string const& key, T default_value) const {
        if (!is_object()) return default_value;
        auto const& obj = std::get<object_type>(value_);
        auto it = obj.find(key);
        if (it == obj.end()) return default_value;
        json const& v = it->second;

        if constexpr (std::is_same_v<T, json>) {
            return v;
        } else if constexpr (std::is_same_v<T, boolean_type>) {
            return v.is_boolean() ? v.as_boolean() : default_value;
        } else if constexpr (std::is_same_v<T, integer_type>) {
            return v.is_integer() ? v.as_integer() : default_value;
        } else if constexpr (std::is_same_v<T, int>) {
            if (v.is_integer()) return static_cast<int>(v.as_integer());
            if (v.is_number())  return static_cast<int>(v.as_number());
            return default_value;
        } else if constexpr (std::is_same_v<T, number_type>) {
            return v.is_number() ? v.as_number() : default_value;
        } else if constexpr (std::is_same_v<T, string_type>) {
            return v.is_string() ? v.as_string() : default_value;
        } else if constexpr (std::is_same_v<T, array_type>) {
            return v.is_array() ? v.as_array() : default_value;
        } else if constexpr (std::is_same_v<T, object_type>) {
            return v.is_object() ? v.as_object() : default_value;
        } else {
            static_assert(std::is_same_v<T, void>, "unsupported type");
        }
    }

    template <typename T>
    T value_or(char const* key, T default_value) const {
        return value_or(std::string(key), std::move(default_value));
    }

    // -- Serialization ------------------------------------------------------

    std::string dump(int indent = 2) const {
        std::string out;
        dump_impl(out, indent, 0);
        return out;
    }

    friend bool operator==(json const&, json const&) = default;

private:
    void ensure_array() {
        if (is_null()) value_ = array_type{};
    }

    void ensure_object() {
        if (is_null()) value_ = object_type{};
    }

    static void dump_string(std::string& out, std::string const& s) {
        out.push_back('"');
        for (unsigned char c : s) {
            switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    static char const* hex = "0123456789ABCDEF";
                    out += "\\u00";
                    out.push_back(hex[(c >> 4) & 0xF]);
                    out.push_back(hex[c & 0xF]);
                } else {
                    out.push_back(static_cast<char>(c));
                }
                break;
            }
        }
        out.push_back('"');
    }

    void dump_impl(std::string& out, int indent, int depth) const {
        auto write_indent = [&](int d) {
            if (indent > 0)
                out.append(static_cast<std::size_t>(d * indent), ' ');
        };

        if (is_null()) {
            out += "null";
        } else if (is_boolean()) {
            out += as_boolean() ? "true" : "false";
        } else if (is_number()) {
            if (is_integer()) {
                std::format_to(std::back_inserter(out), "{}", as_integer());
            } else {
                std::format_to(std::back_inserter(out), "{}", std::get<number_type>(value_));
            }
        } else if (is_string()) {
            dump_string(out, as_string());
        } else if (is_array()) {
            auto const& arr = std::get<array_type>(value_);
            out.push_back('[');
            if (!arr.empty()) {
                bool first = true;
                for (auto const& el : arr) {
                    if (first) first = false;
                    else out.push_back(',');
                    if (indent > 0) { out.push_back('\n'); write_indent(depth + 1); }
                    else out.push_back(' ');
                    el.dump_impl(out, indent, depth + 1);
                }
                if (indent > 0) { out.push_back('\n'); write_indent(depth); }
            }
            out.push_back(']');
        } else if (is_object()) {
            auto const& obj = std::get<object_type>(value_);
            out.push_back('{');
            if (!obj.empty()) {
                bool first = true;
                for (auto const& kv : obj) {
                    if (first) first = false;
                    else out.push_back(',');
                    if (indent > 0) { out.push_back('\n'); write_indent(depth + 1); }
                    else out.push_back(' ');
                    dump_string(out, kv.first);
                    out.push_back(':');
                    if (indent > 0) out.push_back(' ');
                    kv.second.dump_impl(out, indent, depth + 1);
                }
                if (indent > 0) { out.push_back('\n'); write_indent(depth); }
            }
            out.push_back('}');
        }
    }

    value_type value_;
};

// ============================================================================
// Structured bindings support for item_type
// ============================================================================

} // namespace sj

namespace std {

template<>
struct tuple_size<sj::json::item_type> : std::integral_constant<std::size_t, 2> {};

template<>
struct tuple_element<0, sj::json::item_type> {
    using type = std::string const;
};

template<>
struct tuple_element<1, sj::json::item_type> {
    using type = sj::json;
};

} // namespace std

namespace sj {

template <std::size_t I>
auto get(json::item_type item) {
    if constexpr (I == 0) return item.key();
    else if constexpr (I == 1) return item.value();
}

// ============================================================================
// Runtime parser
// ============================================================================

namespace detail {

struct parser {
    char const* cur;
    char const* end;

    char peek() const { return cur < end ? *cur : '\0'; }
    bool at_end() const { return cur >= end; }
    char advance() { return *cur++; }

    void skip_ws() {
        while (cur < end && (*cur == ' ' || *cur == '\n' || *cur == '\r' || *cur == '\t'))
            ++cur;
    }

    std::expected<char, error> expect(char c) {
        skip_ws();
        if (at_end() || *cur != c)
            return std::unexpected(error{error_kind::parse,
                std::format("expected '{}', got '{}'", c,
                            at_end() ? "EOF" : std::string(1, *cur)), ""});
        return advance();
    }

    std::expected<json, error> parse_value() {
        skip_ws();
        if (at_end())
            return std::unexpected(error{error_kind::parse, "unexpected end of input", ""});

        switch (*cur) {
        case '"': return parse_string();
        case '{': return parse_object();
        case '[': return parse_array();
        case 't': case 'f': return parse_bool();
        case 'n': return parse_null();
        default:
            if (*cur == '-' || (*cur >= '0' && *cur <= '9'))
                return parse_number();
            return std::unexpected(error{error_kind::parse,
                std::format("unexpected character '{}'", *cur), ""});
        }
    }

    std::expected<json, error> parse_string() {
        auto r = parse_string_raw();
        if (!r) return std::unexpected(r.error());
        return json(std::move(*r));
    }

    std::expected<std::string, error> parse_string_raw() {
        if (auto e = expect('"'); !e) return std::unexpected(e.error());
        std::string s;
        while (!at_end() && *cur != '"') {
            if (*cur == '\\') {
                ++cur;
                if (at_end())
                    return std::unexpected(error{error_kind::parse, "unexpected end in escape", ""});
                switch (*cur) {
                case '"':  s.push_back('"');  break;
                case '\\': s.push_back('\\'); break;
                case '/':  s.push_back('/');  break;
                case 'b':  s.push_back('\b'); break;
                case 'f':  s.push_back('\f'); break;
                case 'n':  s.push_back('\n'); break;
                case 'r':  s.push_back('\r'); break;
                case 't':  s.push_back('\t'); break;
                case 'u': {
                    ++cur;
                    if (end - cur < 4)
                        return std::unexpected(error{error_kind::parse, "incomplete unicode escape", ""});
                    // Simple: just store the raw hex as-is for basic ASCII range
                    unsigned val = 0;
                    for (int i = 0; i < 4; ++i) {
                        val <<= 4;
                        char h = cur[i];
                        if (h >= '0' && h <= '9') val += h - '0';
                        else if (h >= 'a' && h <= 'f') val += 10 + h - 'a';
                        else if (h >= 'A' && h <= 'F') val += 10 + h - 'A';
                        else return std::unexpected(error{error_kind::parse, "invalid hex in unicode escape", ""});
                    }
                    cur += 4;
                    if (val < 0x80) {
                        s.push_back(static_cast<char>(val));
                    } else if (val < 0x800) {
                        s.push_back(static_cast<char>(0xC0 | (val >> 6)));
                        s.push_back(static_cast<char>(0x80 | (val & 0x3F)));
                    } else {
                        s.push_back(static_cast<char>(0xE0 | (val >> 12)));
                        s.push_back(static_cast<char>(0x80 | ((val >> 6) & 0x3F)));
                        s.push_back(static_cast<char>(0x80 | (val & 0x3F)));
                    }
                    continue; // don't advance again
                }
                default:
                    return std::unexpected(error{error_kind::parse,
                        std::format("invalid escape '\\{}'", *cur), ""});
                }
                ++cur;
            } else {
                s.push_back(*cur++);
            }
        }
        if (at_end())
            return std::unexpected(error{error_kind::parse, "unterminated string", ""});
        ++cur; // skip closing "
        return s;
    }

    std::expected<json, error> parse_number() {
        char const* start = cur;
        bool is_float = false;

        if (*cur == '-') ++cur;
        if (at_end() || !(*cur >= '0' && *cur <= '9'))
            return std::unexpected(error{error_kind::parse, "invalid number", ""});

        while (!at_end() && *cur >= '0' && *cur <= '9') ++cur;

        if (!at_end() && *cur == '.') {
            is_float = true;
            ++cur;
            while (!at_end() && *cur >= '0' && *cur <= '9') ++cur;
        }

        if (!at_end() && (*cur == 'e' || *cur == 'E')) {
            is_float = true;
            ++cur;
            if (!at_end() && (*cur == '+' || *cur == '-')) ++cur;
            while (!at_end() && *cur >= '0' && *cur <= '9') ++cur;
        }

        std::string num_str(start, cur);
        char* end_ptr = nullptr;

        if (is_float) {
            double dv = std::strtod(num_str.c_str(), &end_ptr);
            if (end_ptr != num_str.c_str() + num_str.size())
                return std::unexpected(error{error_kind::parse, "invalid number literal", ""});
            return json(dv);
        } else {
            long long iv = std::strtoll(num_str.c_str(), &end_ptr, 10);
            if (end_ptr != num_str.c_str() + num_str.size())
                return std::unexpected(error{error_kind::parse, "invalid integer literal", ""});
            return json(static_cast<json::integer_type>(iv));
        }
    }

    std::expected<json, error> parse_bool() {
        if (end - cur >= 4 && cur[0] == 't' && cur[1] == 'r' && cur[2] == 'u' && cur[3] == 'e') {
            cur += 4;
            return json(true);
        }
        if (end - cur >= 5 && cur[0] == 'f' && cur[1] == 'a' && cur[2] == 'l' && cur[3] == 's' && cur[4] == 'e') {
            cur += 5;
            return json(false);
        }
        return std::unexpected(error{error_kind::parse, "invalid boolean", ""});
    }

    std::expected<json, error> parse_null() {
        if (end - cur >= 4 && cur[0] == 'n' && cur[1] == 'u' && cur[2] == 'l' && cur[3] == 'l') {
            cur += 4;
            return json(nullptr);
        }
        return std::unexpected(error{error_kind::parse, "expected null", ""});
    }

    std::expected<json, error> parse_array() {
        if (auto e = expect('['); !e) return std::unexpected(e.error());
        json::array_type arr;
        skip_ws();
        if (!at_end() && *cur == ']') { ++cur; return json(std::move(arr)); }

        while (true) {
            auto val = parse_value();
            if (!val) return std::unexpected(val.error());
            arr.push_back(std::move(*val));

            skip_ws();
            if (at_end())
                return std::unexpected(error{error_kind::parse, "unterminated array", ""});
            if (*cur == ']') { ++cur; return json(std::move(arr)); }
            if (*cur == ',') { ++cur; continue; }
            return std::unexpected(error{error_kind::parse,
                std::format("expected ',' or ']', got '{}'", *cur), ""});
        }
    }

    std::expected<json, error> parse_object() {
        if (auto e = expect('{'); !e) return std::unexpected(e.error());
        json::object_type obj;
        skip_ws();
        if (!at_end() && *cur == '}') { ++cur; return json(std::move(obj)); }

        while (true) {
            skip_ws();
            if (at_end() || *cur != '"')
                return std::unexpected(error{error_kind::parse, "expected string key", ""});
            auto key = parse_string_raw();
            if (!key) return std::unexpected(key.error());

            if (auto e = expect(':'); !e) return std::unexpected(e.error());

            auto val = parse_value();
            if (!val) return std::unexpected(val.error());
            obj.emplace(std::move(*key), std::move(*val));

            skip_ws();
            if (at_end())
                return std::unexpected(error{error_kind::parse, "unterminated object", ""});
            if (*cur == '}') { ++cur; return json(std::move(obj)); }
            if (*cur == ',') { ++cur; continue; }
            return std::unexpected(error{error_kind::parse,
                std::format("expected ',' or '}}', got '{}'", *cur), ""});
        }
    }
};

} // namespace detail

inline std::expected<json, error> parse(std::string_view sv) {
    detail::parser p{sv.data(), sv.data() + sv.size()};
    auto result = p.parse_value();
    if (!result) return result;
    p.skip_ws();
    if (!p.at_end())
        return std::unexpected(error{error_kind::parse, "trailing content", ""});
    return result;
}

inline std::expected<json, error> parse(std::istream& is) {
    std::ostringstream oss;
    oss << is.rdbuf();
    return parse(oss.str());
}

// ============================================================================
// to_json implementations
// ============================================================================

inline json to_json(json const& j) { return j; }
inline json to_json(std::nullptr_t) { return json(nullptr); }
inline json to_json(bool b) { return json(b); }
inline json to_json(std::string const& s) { return json(s); }
inline json to_json(std::string_view sv) { return json(std::string(sv)); }
inline json to_json(char const* s) { return json(s); }

template <std::integral T>
json to_json(T n) { return json(static_cast<json::integer_type>(n)); }

template <std::floating_point T>
json to_json(T n) { return json(static_cast<json::number_type>(n)); }

// Enum serialization via reflection
template <typename E>
    requires std::is_enum_v<E>
json to_json(E e) {
    static constexpr auto enums = define_static_array(
        std::meta::enumerators_of(^^E));
    template for (constexpr auto ev : enums) {
        if (e == [:ev:])
            return json(std::string(std::meta::identifier_of(ev)));
    }
    return json(std::format("unknown({})", static_cast<std::underlying_type_t<E>>(e)));
}

// Optional
template <typename T>
json to_json(std::optional<T> const& opt) {
    if (!opt) return json(nullptr);
    return to_json(*opt);
}

// Smart pointers
template <typename T>
json to_json(std::unique_ptr<T> const& p) {
    if (!p) return json(nullptr);
    return to_json(*p);
}

template <typename T>
json to_json(std::shared_ptr<T> const& p) {
    if (!p) return json(nullptr);
    return to_json(*p);
}

// Tuple
template <typename... Ts>
json to_json(std::tuple<Ts...> const& t) {
    json::array_type arr;
    std::apply([&](auto const&... args) {
        (arr.push_back(to_json(args)), ...);
    }, t);
    return json(std::move(arr));
}

// Pair
template <typename A, typename B>
json to_json(std::pair<A, B> const& p) {
    return json(json::array_type{to_json(p.first), to_json(p.second)});
}

// Variant — tagged format
template <typename... Ts>
json to_json(std::variant<Ts...> const& v) {
    json result;
    std::visit([&](auto const& val) {
        using VT = std::decay_t<decltype(val)>;
        result = json(json::object_type{
            {"_type", json(std::string(std::meta::display_string_of(std::meta::dealias(^^VT))))},
            {"_value", to_json(val)}
        });
    }, v);
    return result;
}

// Sequence containers (concept-detected)
template <SequenceSerializable C>
json to_json(C const& seq) {
    json::array_type arr;
    for (auto const& el : seq)
        arr.push_back(to_json(el));
    return json(std::move(arr));
}

// Mapping containers (concept-detected)
template <MappingSerializable C>
json to_json(C const& m) {
    json::object_type obj;
    for (auto const& kv : m)
        obj.emplace(std::string(kv.first), to_json(kv.second));
    return json(std::move(obj));
}

// Aggregate types via reflection (lowest priority fallback)
// Excludes C arrays, ranges (std::array etc.), and json itself
template <typename T>
    requires std::is_aggregate_v<T>
          && (!std::is_array_v<T>)
          && (!std::ranges::input_range<T>)
          && (!std::is_same_v<std::decay_t<T>, json>)
json to_json(T const& obj) {
    json::object_type result;
    static constexpr auto members = define_static_array(
        std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
    template for (constexpr auto m : members) {
        result.emplace(
            std::string(std::meta::identifier_of(m)),
            to_json(obj.[:m:]));
    }
    return json(std::move(result));
}

// ============================================================================
// from_json implementations
// ============================================================================

// Identity
inline std::expected<json, error> from_json_impl(json const& j, std::type_identity<json>) {
    return j;
}

// Bool
inline std::expected<bool, error> from_json_impl(json const& j, std::type_identity<bool>) {
    if (!j.is_boolean())
        return std::unexpected(error{error_kind::type, "expected boolean", ""});
    return j.as_boolean();
}

// String
inline std::expected<std::string, error> from_json_impl(json const& j, std::type_identity<std::string>) {
    if (!j.is_string())
        return std::unexpected(error{error_kind::type, "expected string", ""});
    return j.as_string();
}

// Nullptr
inline std::expected<std::nullptr_t, error> from_json_impl(json const& j, std::type_identity<std::nullptr_t>) {
    if (!j.is_null())
        return std::unexpected(error{error_kind::type, "expected null", ""});
    return nullptr;
}

// Integers
template <std::integral T>
std::expected<T, error> from_json_impl(json const& j, std::type_identity<T>) {
    if (j.is_integer())
        return static_cast<T>(j.as_integer());
    if (j.is_number())
        return static_cast<T>(j.as_number());
    return std::unexpected(error{error_kind::type, "expected integer", ""});
}

// Floating point
template <std::floating_point T>
std::expected<T, error> from_json_impl(json const& j, std::type_identity<T>) {
    if (!j.is_number())
        return std::unexpected(error{error_kind::type, "expected number", ""});
    return static_cast<T>(j.as_number());
}

// Enum deserialization via reflection
template <typename E>
    requires std::is_enum_v<E>
std::expected<E, error> from_json_impl(json const& j, std::type_identity<E>) {
    if (!j.is_string())
        return std::unexpected(error{error_kind::type, "expected string for enum", ""});
    auto const& s = j.as_string();
    static constexpr auto enums = define_static_array(
        std::meta::enumerators_of(^^E));
    template for (constexpr auto ev : enums) {
        if (s == std::meta::identifier_of(ev))
            return [:ev:];
    }
    return std::unexpected(error{error_kind::conversion,
        std::format("unknown enumerator '{}'", s), ""});
}

// Optional
template <typename T>
std::expected<std::optional<T>, error> from_json_impl(json const& j, std::type_identity<std::optional<T>>) {
    if (j.is_null()) return std::optional<T>(std::nullopt);
    auto r = from_json<T>(j);
    if (!r) return std::unexpected(r.error());
    return std::optional<T>(std::move(*r));
}

// unique_ptr
template <typename T>
std::expected<std::unique_ptr<T>, error> from_json_impl(json const& j, std::type_identity<std::unique_ptr<T>>) {
    if (j.is_null()) return std::unique_ptr<T>(nullptr);
    auto r = from_json<T>(j);
    if (!r) return std::unexpected(r.error());
    return std::make_unique<T>(std::move(*r));
}

// shared_ptr
template <typename T>
std::expected<std::shared_ptr<T>, error> from_json_impl(json const& j, std::type_identity<std::shared_ptr<T>>) {
    if (j.is_null()) return std::shared_ptr<T>(nullptr);
    auto r = from_json<T>(j);
    if (!r) return std::unexpected(r.error());
    return std::make_shared<T>(std::move(*r));
}

// Tuple
template <typename... Ts>
std::expected<std::tuple<Ts...>, error> from_json_impl(json const& j, std::type_identity<std::tuple<Ts...>>) {
    if (!j.is_array())
        return std::unexpected(error{error_kind::type, "expected array for tuple", ""});
    auto const& arr = j.as_array();
    if (arr.size() != sizeof...(Ts))
        return std::unexpected(error{error_kind::conversion,
            std::format("expected {} elements, got {}", sizeof...(Ts), arr.size()), ""});

    std::tuple<Ts...> result;
    std::size_t idx = 0;
    bool had_error = false;
    error first_error{error_kind::conversion, "", ""};

    auto parse_element = [&]<typename U>(U& out) {
        if (had_error) return;
        auto r = from_json<U>(arr[idx]);
        if (!r) {
            had_error = true;
            first_error = r.error().with_context(std::format("[{}]", idx));
        } else {
            out = std::move(*r);
        }
        ++idx;
    };

    std::apply([&](auto&... elems) {
        (parse_element(elems), ...);
    }, result);

    if (had_error) return std::unexpected(first_error);
    return result;
}

// Pair
template <typename A, typename B>
std::expected<std::pair<A, B>, error> from_json_impl(json const& j, std::type_identity<std::pair<A, B>>) {
    auto r = from_json<std::tuple<A, B>>(j);
    if (!r) return std::unexpected(r.error());
    auto& [a, b] = *r;
    return std::pair<A, B>(std::move(a), std::move(b));
}

// Variant — tagged format
template <typename... Ts>
std::expected<std::variant<Ts...>, error> from_json_impl(json const& j, std::type_identity<std::variant<Ts...>>) {
    if (!j.is_object())
        return std::unexpected(error{error_kind::type, "expected object for variant", ""});
    if (!j.contains("_type") || !j.contains("_value"))
        return std::unexpected(error{error_kind::missing,
            "variant requires '_type' and '_value' fields", ""});

    auto type_result = j.at("_type");
    if (!type_result) return std::unexpected(type_result.error());
    json const& type_json = type_result->get();
    if (!type_json.is_string())
        return std::unexpected(error{error_kind::type, "'_type' must be a string", ""});
    auto const& type_name = type_json.as_string();

    auto val_result = j.at("_value");
    if (!val_result) return std::unexpected(val_result.error());
    json const& val_json = val_result->get();

    std::expected<std::variant<Ts...>, error> result =
        std::unexpected(error{error_kind::conversion,
            std::format("unknown variant type '{}'", type_name), ""});

    auto try_type = [&]<typename U>() {
        if (result.has_value()) return;
        if (type_name == std::meta::display_string_of(std::meta::dealias(^^U))) {
            auto r = from_json<U>(val_json);
            if (r) result = std::variant<Ts...>(std::move(*r));
            else result = std::unexpected(r.error().with_context("_value"));
        }
    };

    (try_type.template operator()<Ts>(), ...);
    return result;
}

// Sequence containers — BackInsertable
template <typename C>
    requires SequenceSerializable<C> && BackInsertable<C>
             && (!FixedSizeSequence<C>)
std::expected<C, error> from_json_impl(json const& j, std::type_identity<C>) {
    if (!j.is_array())
        return std::unexpected(error{error_kind::type, "expected array", ""});
    C result{};
    std::size_t idx = 0;
    for (auto const& el : j.as_array()) {
        using V = std::ranges::range_value_t<C>;
        auto r = from_json<V>(el);
        if (!r)
            return std::unexpected(r.error().with_context(std::format("[{}]", idx)));
        result.push_back(std::move(*r));
        ++idx;
    }
    return result;
}

// Sequence containers — Insertable (sets)
template <typename C>
    requires SequenceSerializable<C> && Insertable<C>
             && (!BackInsertable<C>) && (!FixedSizeSequence<C>)
std::expected<C, error> from_json_impl(json const& j, std::type_identity<C>) {
    if (!j.is_array())
        return std::unexpected(error{error_kind::type, "expected array", ""});
    C result{};
    std::size_t idx = 0;
    for (auto const& el : j.as_array()) {
        using V = std::ranges::range_value_t<C>;
        auto r = from_json<V>(el);
        if (!r)
            return std::unexpected(r.error().with_context(std::format("[{}]", idx)));
        result.insert(std::move(*r));
        ++idx;
    }
    return result;
}

// std::array — fixed size
template <typename T, std::size_t N>
std::expected<std::array<T, N>, error> from_json_impl(json const& j, std::type_identity<std::array<T, N>>) {
    if (!j.is_array())
        return std::unexpected(error{error_kind::type, "expected array", ""});
    auto const& arr = j.as_array();
    if (arr.size() != N)
        return std::unexpected(error{error_kind::conversion,
            std::format("expected {} elements, got {}", N, arr.size()), ""});
    std::array<T, N> result{};
    for (std::size_t i = 0; i < N; ++i) {
        auto r = from_json<T>(arr[i]);
        if (!r) return std::unexpected(r.error().with_context(std::format("[{}]", i)));
        result[i] = std::move(*r);
    }
    return result;
}

// Mapping containers
template <typename C>
    requires MappingSerializable<C>
std::expected<C, error> from_json_impl(json const& j, std::type_identity<C>) {
    if (!j.is_object())
        return std::unexpected(error{error_kind::type, "expected object", ""});
    C result{};
    for (auto const& [key, val] : j.as_object()) {
        using V = typename std::ranges::range_value_t<C>::second_type;
        auto r = from_json<V>(val);
        if (!r) return std::unexpected(r.error().with_context(key));
        result.emplace(key, std::move(*r));
    }
    return result;
}

// Aggregate types via reflection
template <typename T>
    requires std::is_aggregate_v<T>
          && (!std::is_array_v<T>)
          && (!std::ranges::input_range<T>)
          && (!std::is_same_v<std::decay_t<T>, json>)
std::expected<T, error> from_json_impl(json const& j, std::type_identity<T>) {
    if (!j.is_object())
        return std::unexpected(error{error_kind::type, "expected object", ""});

    T obj{};
    static constexpr auto members = define_static_array(
        std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
    template for (constexpr auto m : members) {
        using MemberType = typename[:std::meta::type_of(m):];
        constexpr auto name = std::meta::identifier_of(m);

        auto it = j.find(std::string(name));
        if (it == j.end()) {
            if constexpr (detail::is_optional_v<MemberType>) {
                obj.[:m:] = std::nullopt;
            } else {
                return std::unexpected(error{error_kind::missing,
                    std::format("missing field '{}'", name), std::string(name)});
            }
        } else {
            auto parsed = from_json<MemberType>(*it);
            if (!parsed)
                return std::unexpected(parsed.error().with_context(name));
            obj.[:m:] = std::move(*parsed);
        }
    }
    return obj;
}

// ============================================================================
// ============================================================================
// Stream operator
// ============================================================================

inline std::ostream& operator<<(std::ostream& os, json const& j) {
    os << j.dump(0);
    return os;
}

// ============================================================================
// Compile-time JSON parser
// ============================================================================

namespace ct {

template <std::meta::info ...Ms>
struct Outer {
    struct Inner;
    consteval { define_aggregate(^^Inner, {Ms...}); }
};

template <std::meta::info ...Ms>
using Cls = Outer<Ms...>::Inner;

template <typename T, auto ... Vs>
constexpr T construct_from{Vs...};

// Helpers for consteval number parsing
consteval int ct_parse_int(std::string_view in) {
    int out = 0;
    bool neg = false;
    auto it = in.begin();
    if (it != in.end() && *it == '-') { neg = true; ++it; }
    for (; it != in.end(); ++it)
        out = out * 10 + (*it - '0');
    return neg ? -out : out;
}

consteval double ct_parse_double(std::string_view s) {
    double result = 0, frac = 0, div = 1;
    bool neg = false, in_frac = false;
    auto it = s.begin();
    if (it != s.end() && *it == '-') { neg = true; ++it; }
    for (; it != s.end(); ++it) {
        if (*it == '.') { in_frac = true; continue; }
        if (*it == 'e' || *it == 'E') break;
        if (in_frac) { div *= 10; frac += (*it - '0') / div; }
        else { result = result * 10 + (*it - '0'); }
    }
    return neg ? -(result + frac) : (result + frac);
}

consteval bool ct_is_ws(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

consteval bool ct_is_float(std::string_view v) {
    for (char c : v)
        if (c == '.' || c == 'e' || c == 'E') return true;
    return false;
}

// Forward declarations
consteval std::meta::info parse_json_ct(std::string_view json);
consteval std::meta::info parse_array_ct(std::string_view arr_str);

// Classify a single value string and return {type_info, value_info}
consteval std::pair<std::meta::info, std::meta::info> ct_classify_value(std::string_view elem) {
    using std::meta::reflect_constant, std::meta::reflect_constant_string;

    if (elem.size() >= 2 && elem[0] == '"') {
        // String
        auto contents = elem.substr(1, elem.size() - 2);
        return {^^char const*, reflect_constant_string(contents)};
    }
    if (elem == "true")  return {^^bool, reflect_constant(true)};
    if (elem == "false") return {^^bool, reflect_constant(false)};
    if (elem == "null")  return {^^std::nullptr_t, reflect_constant(nullptr)};
    if (elem[0] == '{') {
        auto parsed = parse_json_ct(elem);
        return {std::meta::type_of(parsed), parsed};
    }
    // Note: compile-time arrays as std::tuple are not supported in
    // define_aggregate (std::tuple is not an aggregate). Arrays in
    // JSON literals are handled at runtime via the to_json bridge.
    // Number
    if (ct_is_float(elem)) {
        double dv = ct_parse_double(elem);
        return {^^double, reflect_constant(dv)};
    }
    int iv = ct_parse_int(elem);
    return {^^int, reflect_constant(iv)};
}

// Extract one value token from position, handling strings, objects, arrays, atoms
consteval std::string ct_extract_token(std::string_view src, std::size_t& pos) {
    auto sz = src.size();
    // skip whitespace
    while (pos < sz && ct_is_ws(src[pos])) ++pos;
    if (pos >= sz) throw "unexpected end";

    std::string out;

    // String
    if (src[pos] == '"') {
        out += src[pos++];
        while (pos < sz && src[pos] != '"') {
            if (src[pos] == '\\') out += src[pos++];
            if (pos < sz) out += src[pos++];
        }
        if (pos >= sz) throw "unterminated string";
        out += src[pos++];
        return out;
    }

    // Object or array — track depth
    if (src[pos] == '{' || src[pos] == '[') {
        char open = src[pos], close = (open == '{') ? '}' : ']';
        unsigned depth = 0;
        do {
            if (src[pos] == open) ++depth;
            else if (src[pos] == close) --depth;
            else if (src[pos] == '"') {
                out += src[pos++];
                while (pos < sz && src[pos] != '"') {
                    if (src[pos] == '\\') out += src[pos++];
                    if (pos < sz) out += src[pos++];
                }
                if (pos < sz) out += src[pos++];
                continue;
            }
            out += src[pos++];
        } while (pos < sz && depth > 0);
        return out;
    }

    // Atom (number, true, false, null)
    while (pos < sz && !ct_is_ws(src[pos]) && src[pos] != ',' && src[pos] != '}' && src[pos] != ']')
        out += src[pos++];
    return out;
}

// Parse a JSON array string into a std::tuple via reflection
consteval std::meta::info parse_array_ct(std::string_view arr_str) {
    std::size_t pos = 0;
    auto sz = arr_str.size();

    // skip whitespace and '['
    while (pos < sz && ct_is_ws(arr_str[pos])) ++pos;
    if (pos >= sz || arr_str[pos] != '[') throw "expected [";
    ++pos;

    std::vector<std::meta::info> types;
    std::vector<std::meta::info> values = {^^void}; // placeholder

    while (pos < sz) {
        while (pos < sz && ct_is_ws(arr_str[pos])) ++pos;
        if (pos < sz && arr_str[pos] == ']') break;

        auto elem = ct_extract_token(arr_str, pos);
        if (elem.empty()) throw "expected element";

        auto [t, v] = ct_classify_value(elem);
        types.push_back(t);
        values.push_back(v);

        while (pos < sz && ct_is_ws(arr_str[pos])) ++pos;
        if (pos < sz && arr_str[pos] == ',') ++pos;
    }

    values[0] = substitute(^^std::tuple, types);
    return substitute(^^construct_from, values);
}

// Parse a JSON object (or array) string into a reflected aggregate
consteval std::meta::info parse_json_ct(std::string_view json) {
    std::size_t pos = 0;
    auto sz = json.size();

    while (pos < sz && ct_is_ws(json[pos])) ++pos;
    if (pos >= sz) throw "empty input";

    // Top-level array
    if (json[pos] == '[') return parse_array_ct(json);

    // Top-level object
    if (json[pos] != '{') throw "expected { or [";
    ++pos;

    std::vector<std::meta::info> members;
    std::vector<std::meta::info> values = {^^void};

    using std::meta::reflect_constant, std::meta::data_member_spec;

    while (pos < sz) {
        while (pos < sz && ct_is_ws(json[pos])) ++pos;
        if (pos < sz && json[pos] == '}') break;

        // Parse key string
        while (pos < sz && ct_is_ws(json[pos])) ++pos;
        if (pos >= sz || json[pos] != '"') throw "expected key";
        ++pos;
        std::string field_name;
        while (pos < sz && json[pos] != '"')
            field_name += json[pos++];
        if (pos >= sz) throw "unterminated key";
        ++pos; // closing "

        // Expect colon
        while (pos < sz && ct_is_ws(json[pos])) ++pos;
        if (pos >= sz || json[pos] != ':') throw "expected :";
        ++pos;

        // Extract value token
        auto value = ct_extract_token(json, pos);
        if (value.empty()) throw "expected value";

        // Classify and add
        auto [t, v] = ct_classify_value(value);
        auto dms = data_member_spec(t, {.name = field_name});
        members.push_back(reflect_constant(dms));
        values.push_back(v);

        while (pos < sz && ct_is_ws(json[pos])) ++pos;
        if (pos < sz && json[pos] == ',') ++pos;
    }

    if (pos >= sz) throw "unterminated object";
    // skip }
    ++pos;

    values[0] = substitute(^^Cls, members);
    return substitute(^^construct_from, values);
}

// NTTP wrapper
struct json_string {
    std::meta::info rep;
    consteval json_string(char const* s) : rep{parse_json_ct(s)} {}
};

// json_literal proxy
template <auto Data>
struct json_literal {
    static constexpr auto const& data = Data;
    using aggregate_type = std::remove_cvref_t<decltype(Data)>;

    consteval auto const& value() const { return data; }

    operator json() const { return to_json(data); }
    json get() const { return to_json(data); }

    friend json to_json(json_literal const& lit) {
        return to_json(lit.data);
    }
};

} // namespace ct

namespace literals {

template <ct::json_string JS>
consteval auto operator""_json() {
    return ct::json_literal<([:JS.rep:])>{};
}

} // namespace literals

} // namespace sj

#endif // SIMPLE_JSON_HPP
