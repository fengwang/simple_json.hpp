#ifndef SIMPLE_JSON_HPP
#define SIMPLE_JSON_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <expected>
#include <variant>
#include <vector>
#include <map>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <type_traits>
#include <iterator>
#include <tuple>
#include <utility>

/// Simple streaming JSON reader utilities.
namespace sj {

 /// JSON token type produced by the reader.
 enum class Type { END, ARRAY, OBJECT, NUMBER, STRING, BOOL, NULL_ };

 /// Streaming JSON reader state.
 struct Reader {
     /// Pointer to the beginning of the JSON buffer.
     char const* data;
     /// Current cursor position within the buffer.
     char const* cur;
     /// One‑past‑the‑end pointer of the buffer.
     char const* end;
     /// Current nesting depth (arrays / objects).
     int depth;
 };

 /// View of a single JSON value within the original buffer.
 struct Value {
     /// Type of this JSON value.
     Type type;
     /// Pointer to the first character of the value’s textual representation.
     char const* start;
     /// One‑past‑the‑end pointer of the value’s textual representation.
     char const* end;
     /// Nesting depth at which this value resides.
     int depth;
 };

 /// Create a reader over a JSON buffer `[data, data + len)`.
 inline Reader reader(char const* data, std::size_t len) {
     Reader r;
     r.data = data;
     r.cur = data;
     r.end = data + len;
     r.depth = 0;
     return r;
 }

 /// Return true if the character can appear after the first character
 /// in a JSON number literal.
 inline bool is_number_cont(char c) {
     return (c >= '0' && c <= '9') ||  c == 'e' || c == 'E' || c == '.' || c == '-' || c == '+';
 }

 /// Helper that checks whether the sequence `[cur, end)` starts with `expect`.
 inline bool is_string(char const* cur, char const* end, char const* expect) {
     while (*expect) {
         if (cur == end || *cur != *expect)
             return false;
         expect++, cur++;
     }
     return true;
 }

 /// Read the next JSON token from the stream.
 ///
 /// On success, returns a `Value` whose `start`/`end` point into
 /// the original buffer managed by `Reader`. On failure, returns
 /// an error message describing the problem.
 inline std::expected<Value, std::string> read(Reader* r) {
     Value res{};

     // Process tokens in a loop to avoid goto
     while (true) {
         // Handle EOF condition
         if (r->cur == r->end) { return std::unexpected("unexpected eof"); }

         res.start = r->cur;

         switch (*r->cur) {
         case ' ': case '\n': case '\r': case '\t':
         case ':': case ',':
             r->cur++;
             // Continue to next iteration of the loop
             continue;

         case '-': case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
             res.type = Type::NUMBER;
             while (r->cur != r->end && is_number_cont(*r->cur)) { r->cur++; }
             break;

         case '"':
             res.type = Type::STRING;
             res.start = ++r->cur;
             for (;;) {
                 if ( r->cur == r->end) { return std::unexpected("unclosed string"); }
                 if (*r->cur ==    '"') { break; }
                 if (*r->cur ==   '\\') { r->cur++; }
                 if ( r->cur != r->end) { r->cur++; }
             }
             res.end = r->cur++;
             return res;

         case '{': case '[':
             if (r->depth > 1000000) { return std::unexpected("max depth reached!"); }
             res.type = (*r->cur == '{') ? Type::OBJECT : Type::ARRAY;
             res.depth = ++r->depth;
             r->cur++;
             break;

         case '}': case ']':
             res.type = Type::END;
             if (--r->depth < 0) { return std::unexpected((*r->cur == '}') ? "stray '}'" : "stray ']'"); }
             r->cur++;
             break;

         case 'n': case 't': case 'f':
             res.type = (*r->cur == 'n') ? Type::NULL_ : Type::BOOL;
             if (is_string(r->cur, r->end,  "null")) { r->cur += 4; break; }
             if (is_string(r->cur, r->end,  "true")) { r->cur += 4; break; }
             if (is_string(r->cur, r->end, "false")) { r->cur += 5; break; }
             [[fallthrough]];

         default:
             return std::unexpected("unknown token");
         }

         // If we reach here, it means we've processed a complete token
         res.end = r->cur;
         return res;
     }
 }

 /// Consume values until the reader’s depth returns to the given `depth`.
 ///
 /// This is mainly useful for skipping over unneeded subtrees.
 inline void discard_until(Reader* r, int depth) {
     while (r->depth != depth) {
         auto val = read(r);
         if (!val.has_value()) { break; }
     }
 }

 /// Iterate over the elements of a JSON array.
 ///
 /// `arr` must be a `Value` whose `type` is `Type::ARRAY` and whose
 /// `depth` matches the reader state. On success, stores the next
 /// element into `*val` and returns:
 ///  - `true`  while there are more elements,
 ///  - `false` when the end of the array is reached.
 /// On error, returns an error message.
 inline std::expected<bool, std::string> iter_array(Reader* r, Value arr, Value* val) {
     discard_until(r, arr.depth);
     auto result = read(r);
     if (!result.has_value()) { return std::unexpected(result.error()); }
     *val = result.value();
     if (val->type == Type::END) { return false; }
     return true;
 }

 /// Iterate over the key/value pairs of a JSON object.
 ///
 /// `obj` must be a `Value` whose `type` is `Type::OBJECT` and whose
 /// `depth` matches the reader state. On success, stores the next
 /// key into `*key` and its value into `*val`, and returns:
 ///  - `true`  while there are more pairs,
 ///  - `false` when the end of the object is reached.
 /// On error, returns an error message.
 inline std::expected<bool, std::string> iter_object(Reader* r, Value obj, Value* key, Value* val) {
     discard_until(r, obj.depth);
     auto key_result = read(r);
     if (!key_result.has_value()) { return std::unexpected(key_result.error()); }
     *key = key_result.value();
     if (key->type == Type::END) { return false; }

     auto val_result = read(r);
     if (!val_result.has_value()) { return std::unexpected(val_result.error()); }
     *val = val_result.value();
     if (val->type == Type::END)   { return std::unexpected("unexpected object end"); }
     return true;
 }

 /// Compute the current line and column of the reader cursor.
 ///
 /// Lines and columns are 1‑based and are counted from `Reader::data`
 /// up to, but not including, the current cursor position.
 inline void location(Reader* r, int* line, int* col) {
     int ln = 1, cl = 1;
     for (char const *p = r->data; p != r->cur; p++) {
         if (*p == '\n') { ln++; cl = 0; } cl++;
     }
     *line = ln;
     *col = cl;
 }

 /// High‑level JSON value category used by sj::json::type().
 enum class value_t {
     null,
     boolean,
     integer,
     number,
     string,
     array,
     object
 };

 namespace detail {

 template <typename T, typename = void>
 struct is_pair_like : std::false_type {};

 template <typename T>
 struct is_pair_like<T, std::void_t<typename T::first_type, typename T::second_type>>
     : std::true_type {};

 template <typename K>
 inline std::string key_to_string(K const& k) {
     return std::string(k);
 }

 } // namespace detail

 /// First‑class JSON value type built on top of the streaming reader.
 class json {
 public:
     /// Type used to represent JSON arrays.
     using array_type = std::vector<json>;
     /// Type used to represent JSON objects.
     using object_type = std::map<std::string, json>;
     /// Type used to represent JSON strings.
     using string_type = std::string;
     /// Type used to represent JSON booleans.
     using boolean_type = bool;
     /// Type used to represent integer JSON numbers.
     using integer_type = std::int64_t;
     /// Type used to represent floating‑point JSON numbers.
     using number_type = double;
     /// Underlying variant storage type.
     using value_type = std::variant<
         std::nullptr_t,
         boolean_type,
         integer_type,
         number_type,
         string_type,
         array_type,
         object_type>;

     /// Iterator for arrays and objects.
     class iterator {
     public:
         using difference_type   = std::ptrdiff_t;
         using value_type        = json;
         using pointer           = json*;
         using reference         = json&;
         using iterator_category = std::bidirectional_iterator_tag;

         iterator() = default;

         reference operator*() const {
             if (std::holds_alternative<array_iter>(it_)) {
                 return *std::get<array_iter>(it_);
             }
             return std::get<object_iter>(it_)->second;
         }

         pointer operator->() const { return &**this; }

         iterator& operator++() {
             if (std::holds_alternative<array_iter>(it_)) {
                 ++std::get<array_iter>(it_);
             } else if (std::holds_alternative<object_iter>(it_)) {
                 ++std::get<object_iter>(it_);
             }
             return *this;
         }

         iterator operator++(int) {
             iterator tmp = *this;
             ++(*this);
             return tmp;
         }

         iterator& operator--() {
             if (std::holds_alternative<array_iter>(it_)) {
                 --std::get<array_iter>(it_);
             } else if (std::holds_alternative<object_iter>(it_)) {
                 --std::get<object_iter>(it_);
             }
             return *this;
         }

         iterator operator--(int) {
             iterator tmp = *this;
             --(*this);
             return tmp;
         }

         friend bool operator==(iterator const& a, iterator const& b) {
             return a.it_ == b.it_;
         }

         friend bool operator!=(iterator const& a, iterator const& b) {
             return !(a == b);
         }

         /// Access the key when iterating objects.
         std::string const& key() const {
             return std::get<object_iter>(it_)->first;
         }

         /// Access the value when iterating objects.
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

     /// Const iterator for arrays and objects.
     class const_iterator {
     public:
         using difference_type   = std::ptrdiff_t;
         using value_type        = json;
         using pointer           = json const*;
         using reference         = json const&;
         using iterator_category = std::bidirectional_iterator_tag;

         const_iterator() = default;

         reference operator*() const {
             if (std::holds_alternative<array_iter>(it_)) {
                 return *std::get<array_iter>(it_);
             }
             return std::get<object_iter>(it_)->second;
         }

         pointer operator->() const { return &**this; }

         const_iterator& operator++() {
             if (std::holds_alternative<array_iter>(it_)) {
                 ++std::get<array_iter>(it_);
             } else if (std::holds_alternative<object_iter>(it_)) {
                 ++std::get<object_iter>(it_);
             }
             return *this;
         }

         const_iterator operator++(int) {
             const_iterator tmp = *this;
             ++(*this);
             return tmp;
         }

         const_iterator& operator--() {
             if (std::holds_alternative<array_iter>(it_)) {
                 --std::get<array_iter>(it_);
             } else if (std::holds_alternative<object_iter>(it_)) {
                 --std::get<object_iter>(it_);
             }
             return *this;
         }

         const_iterator operator--(int) {
             const_iterator tmp = *this;
             --(*this);
             return tmp;
         }

         friend bool operator==(const_iterator const& a, const_iterator const& b) {
             return a.it_ == b.it_;
         }

         friend bool operator!=(const_iterator const& a, const_iterator const& b) {
             return !(a == b);
         }

         /// Access the key when iterating objects.
         std::string const& key() const {
             return std::get<object_iter>(it_)->first;
         }

         /// Access the value when iterating objects.
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

     /// Proxy type used when iterating key/value pairs of an object.
     struct item_type {
         std::string const* key_ptr;
         json* value_ptr;

         std::string const& key() const { return *key_ptr; }
         json& value() const { return *value_ptr; }
     };

     /// View over the key/value pairs of an object.
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

             iterator& operator++() {
                 ++it_;
                 return *this;
             }

             iterator operator++(int) {
                 iterator tmp = *this;
                 ++(*this);
                 return tmp;
             }

             friend bool operator==(iterator const& a, iterator const& b) {
                 return a.it_ == b.it_;
             }

             friend bool operator!=(iterator const& a, iterator const& b) {
                 return !(a == b);
             }

         private:
             friend class items_view;
             object_type::iterator it_{};
         };

         explicit items_view(object_type& obj) : obj_(&obj) {}

         iterator begin() { return iterator(obj_->begin()); }
         iterator end()   { return iterator(obj_->end()); }

     private:
         object_type* obj_;
     };

     /// Construct a null JSON value.
     json() : value_(nullptr) {}
     /// Construct a null JSON value.
     json(std::nullptr_t) : value_(nullptr) {}
     /// Construct a boolean JSON value.
     json(boolean_type b) : value_(b) {}
     /// Construct an integer JSON value.
     json(integer_type n) : value_(n) {}
     /// Construct an integer JSON value from a signed int.
     json(int n) : value_(static_cast<integer_type>(n)) {}
     /// Construct a numeric JSON value from a floating‑point number.
     json(number_type n) : value_(n) {}
     /// Construct a string JSON value.
     json(string_type const& s) : value_(s) {}
     /// Construct a string JSON value.
     json(string_type&& s) : value_(std::move(s)) {}
     /// Construct a string JSON value from a null‑terminated C string.
     json(char const* s) : value_(string_type{s}) {}
     /// Construct an array JSON value.
     json(array_type const& a) : value_(a) {}
     /// Construct an array JSON value.
     json(array_type&& a) : value_(std::move(a)) {}
     /// Construct an object JSON value.
     json(object_type const& o) : value_(o) {}
     /// Construct an object JSON value.
     json(object_type&& o) : value_(std::move(o)) {}
     /// Construct an object JSON value from an initializer list of key/value pairs.
     json(std::initializer_list<std::pair<const std::string, json>> init)
         : value_(object_type{}) {
         auto& obj = std::get<object_type>(value_);
         for (auto const& kv : init) {
             obj.emplace(kv.first, kv.second);
         }
     }

     /// Construct a JSON array from a generic container of elements.
     ///
     /// This enables construction from standard sequence and set-like
     /// containers such as `std::vector`, `std::deque`, `std::list`,
     /// `std::forward_list`, `std::array`, `std::set`, `std::multiset`,
     /// `std::unordered_set`, and `std::unordered_multiset`, as long as
     /// the contained value type is itself convertible to `sj::json`.
     template <typename Container,
               typename C = std::decay_t<Container>,
               typename V = typename C::value_type,
               std::enable_if_t<
                   !std::is_same_v<C, json> &&
                   !std::is_same_v<C, array_type> &&
                   !std::is_same_v<C, object_type> &&
                   !detail::is_pair_like<V>::value,
                   int> = 0>
     json(Container const& c)
         : value_(array_type{}) {
         auto& arr = std::get<array_type>(value_);
         for (auto const& el : c) {
             using E = std::decay_t<decltype(el)>;
             if constexpr (std::is_same_v<E, boolean_type>) {
                 arr.emplace_back(json(el));
             } else if constexpr (std::is_integral_v<E>) {
                 arr.emplace_back(json(static_cast<integer_type>(el)));
             } else {
                 arr.emplace_back(json(el));
             }
         }
     }

     /// Construct a JSON object from a generic associative container of
     /// key/value pairs.
     ///
     /// This enables construction from `std::map`, `std::unordered_map`,
     /// `std::multimap`, and `std::unordered_multimap` if the key type
     /// can be used to construct an `std::string` and the mapped type is
     /// convertible to `sj::json`. For multi-maps, only one value per
     /// key is kept in the resulting JSON object; which one depends on
     /// the container’s iteration order.
     template <typename Container,
               typename C = std::decay_t<Container>,
               typename V = typename C::value_type,
               std::enable_if_t<
                   !std::is_same_v<C, json> &&
                   !std::is_same_v<C, array_type> &&
                   !std::is_same_v<C, object_type> &&
                   detail::is_pair_like<V>::value,
                   int> = 0>
     json(Container const& c)
         : value_(object_type{}) {
         auto& obj = std::get<object_type>(value_);
         for (auto const& kv : c) {
             std::string key = detail::key_to_string(kv.first);
             using M = std::decay_t<decltype(kv.second)>;
             if constexpr (std::is_same_v<M, boolean_type>) {
                 obj.emplace(std::move(key), json(kv.second));
             } else if constexpr (std::is_integral_v<M>) {
                 obj.emplace(std::move(key),
                             json(static_cast<integer_type>(kv.second)));
             } else {
                 obj.emplace(std::move(key), json(kv.second));
             }
         }
     }

     /// Defaulted copy constructor.
     json(json const&) = default;
     /// Defaulted move constructor.
     json(json&&) noexcept = default;
     /// Defaulted copy assignment.
     json& operator=(json const&) = default;
     /// Defaulted move assignment.
     json& operator=(json&&) noexcept = default;

     /// Return true if this value is null.
     bool is_null() const { return std::holds_alternative<std::nullptr_t>(value_); }
     /// Return true if this value is a boolean.
     bool is_boolean() const { return std::holds_alternative<boolean_type>(value_); }
     /// Return true if this value is an integer number.
     bool is_integer() const { return std::holds_alternative<integer_type>(value_); }
     /// Return true if this value is a number (integer or floating‑point).
     bool is_number() const {
         return std::holds_alternative<integer_type>(value_) ||
                std::holds_alternative<number_type>(value_);
     }
     /// Return true if this value is a string.
     bool is_string() const { return std::holds_alternative<string_type>(value_); }
     /// Return true if this value is an array.
     bool is_array() const { return std::holds_alternative<array_type>(value_); }
     /// Return true if this value is an object.
     bool is_object() const { return std::holds_alternative<object_type>(value_); }

     /// Return the high‑level type of this value.
     value_t type() const {
         if (is_null())    { return value_t::null; }
         if (is_boolean()) { return value_t::boolean; }
         if (is_integer()) { return value_t::integer; }
         if (std::holds_alternative<number_type>(value_)) { return value_t::number; }
         if (is_string())  { return value_t::string; }
         if (is_array())   { return value_t::array; }
         if (is_object())  { return value_t::object; }
         return value_t::null;
     }

     /// Access the contained boolean. Precondition: `is_boolean()`.
     boolean_type as_boolean() const { return std::get<boolean_type>(value_); }
     /// Access the contained integer. Precondition: `is_integer()`.
     integer_type as_integer() const { return std::get<integer_type>(value_); }
     /// Access the contained number. Precondition: `is_number()`.
     number_type as_number() const {
         if (std::holds_alternative<number_type>(value_)) {
             return std::get<number_type>(value_);
         }
         return static_cast<number_type>(std::get<integer_type>(value_));
     }
     /// Access the contained string. Precondition: `is_string()`.
     string_type const& as_string() const { return std::get<string_type>(value_); }
     /// Access the contained array. Precondition: `is_array()`.
     array_type const& as_array() const { return std::get<array_type>(value_); }
     /// Access the contained object. Precondition: `is_object()`.
     object_type const& as_object() const { return std::get<object_type>(value_); }

     /// Extract the value as type `T`.
     ///
     /// Supported `T` are:
     ///  - `sj::json`
     ///  - `bool`
     ///  - `integer_type` / `int`
     ///  - `number_type`
     ///  - `string_type`
     ///  - `array_type`
     ///  - `object_type`
     /// Throws `std::runtime_error` if the value cannot be converted.
     template <typename T>
     T get() const {
         if constexpr (std::is_same_v<T, json>) {
             return *this;
         } else if constexpr (std::is_same_v<T, boolean_type>) {
             if (!is_boolean()) {
                 throw std::runtime_error("sj::json::get<bool>: not a boolean");
             }
             return as_boolean();
         } else if constexpr (std::is_same_v<T, integer_type>) {
             if (is_integer()) {
                 return as_integer();
             }
             if (std::holds_alternative<number_type>(value_)) {
                 return static_cast<integer_type>(std::get<number_type>(value_));
             }
             throw std::runtime_error("sj::json::get<integer_type>: not a number");
         } else if constexpr (std::is_same_v<T, int>) {
             if (is_integer()) {
                 return static_cast<int>(as_integer());
             }
             if (std::holds_alternative<number_type>(value_)) {
                 return static_cast<int>(std::get<number_type>(value_));
             }
             throw std::runtime_error("sj::json::get<int>: not a number");
         } else if constexpr (std::is_same_v<T, number_type>) {
             if (!is_number()) {
                 throw std::runtime_error("sj::json::get<number_type>: not a number");
             }
             return as_number();
         } else if constexpr (std::is_same_v<T, string_type>) {
             if (!is_string()) {
                 throw std::runtime_error("sj::json::get<string>: not a string");
             }
             return as_string();
         } else if constexpr (std::is_same_v<T, array_type>) {
             if (!is_array()) {
                 throw std::runtime_error("sj::json::get<array_type>: not an array");
             }
             return as_array();
         } else if constexpr (std::is_same_v<T, object_type>) {
             if (!is_object()) {
                 throw std::runtime_error("sj::json::get<object_type>: not an object");
             }
             return as_object();
         } else {
             static_assert(std::is_same_v<T, void>,
                           "sj::json::get: unsupported type");
         }
     }

     /// Convert this value to a double, or return `default_value` if it is
     /// not a number.
     number_type to_double(number_type default_value = 0.0) const {
         return is_number() ? as_number() : default_value;
     }

     /// Convert this value to an int, or return `default_value` if it is
     /// not numeric.
     int to_int(int default_value = 0) const {
         if (is_integer()) {
             return static_cast<int>(as_integer());
         }
         if (is_number()) {
             return static_cast<int>(as_number());
         }
         return default_value;
     }

     /// Access or create an object member by key.
     ///
     /// If this value is null, it is first converted to an empty object.
     /// Precondition (otherwise): `is_object()`.
     json& operator[](std::string const& key) {
         if (!is_object()) {
             value_ = object_type{};
         }
         return std::get<object_type>(value_)[key];
     }

     /// Access or create an object member by key.
     json& operator[](char const* key) {
         return (*this)[std::string(key)];
     }

     /// Access an object member by key (const). Precondition: `is_object()`.
     json const& at(std::string const& key) const {
         return std::get<object_type>(value_).at(key);
     }

     /// Access an object member by key. Precondition: `is_object()`.
     json& at(std::string const& key) {
         return std::get<object_type>(value_).at(key);
     }

     /// Access an array element by index. Precondition: `is_array()`.
     json& operator[](std::size_t idx) {
         return std::get<array_type>(value_)[idx];
     }

     /// Access an array element by index (const). Precondition: `is_array()`.
     json const& at(std::size_t idx) const {
         return std::get<array_type>(value_).at(idx);
     }

     /// Access an array element by index. Precondition: `is_array()`.
     json& at(std::size_t idx) {
         return std::get<array_type>(value_).at(idx);
     }

     /// Append an element to an array, converting null to an empty array.
     void push_back(json const& v) {
         ensure_array();
         std::get<array_type>(value_).push_back(v);
     }

     /// Append an element to an array, converting null to an empty array.
     void push_back(json&& v) {
         ensure_array();
         std::get<array_type>(value_).push_back(std::move(v));
     }

     /// Append an element constructed from `value` to an array.
     template <typename T>
     void push_back(T&& value) {
         emplace_back(std::forward<T>(value));
     }

     /// Emplace an element at the end of an array and return a reference to it.
     template <typename... Args>
     json& emplace_back(Args&&... args) {
         ensure_array();
         auto& arr = std::get<array_type>(value_);
         arr.emplace_back(std::forward<Args>(args)...);
         return arr.back();
     }

     /// Emplace a key/value pair into an object and return a reference to the value.
     template <typename T>
     json& emplace(std::string key, T&& value) {
         ensure_object();
         auto& obj = std::get<object_type>(value_);
         auto [it, inserted] = obj.emplace(std::move(key), json(std::forward<T>(value)));
         (void)inserted;
         return it->second;
     }

     /// Emplace a key/value pair into an object and return a reference to the value.
     template <typename T>
     json& emplace(char const* key, T&& value) {
         return emplace(std::string(key), std::forward<T>(value));
     }

     /// Return true if this is an object and contains the given key.
     bool contains(std::string const& key) const {
         if (!is_object()) {
             return false;
         }
         auto const& obj = std::get<object_type>(value_);
         return obj.find(key) != obj.end();
     }

     /// Return true if this is an object and contains the given key.
     bool contains(char const* key) const {
         return contains(std::string(key));
     }

     /// Return true if this is an array and `idx` is a valid index.
     bool contains(std::size_t idx) const {
         if (!is_array()) {
             return false;
         }
         auto const& arr = std::get<array_type>(value_);
         return idx < arr.size();
     }

     /// Return the number of elements.
     ///
     /// For arrays and objects, this is the element count.
     /// For null, returns 0. For scalar values, returns 1.
     std::size_t size() const {
         if (is_null()) {
             return 0;
         }
         if (is_array()) {
             return std::get<array_type>(value_).size();
         }
         if (is_object()) {
             return std::get<object_type>(value_).size();
         }
         return 1;
     }

     /// Return true if the value is empty.
     ///
     /// Null is empty; arrays and objects are empty if they contain
     /// no elements; scalar values are never empty.
     bool empty() const {
         if (is_null()) {
             return true;
         }
         if (is_array()) {
             return std::get<array_type>(value_).empty();
         }
         if (is_object()) {
             return std::get<object_type>(value_).empty();
         }
         return false;
     }

     /// Remove all elements from the value.
     ///
     /// For arrays and objects, clears their contents. For scalar
     /// values, resets the value to null.
     void clear() {
         if (is_array()) {
             std::get<array_type>(value_).clear();
         } else if (is_object()) {
             std::get<object_type>(value_).clear();
         } else {
             value_ = nullptr;
         }
     }

     /// Iterator to the first element (array element or object value).
     iterator begin() {
         if (is_array()) {
             auto& arr = std::get<array_type>(value_);
             return iterator(arr.begin());
         }
         if (is_object()) {
             auto& obj = std::get<object_type>(value_);
             return iterator(obj.begin());
         }
         return iterator{};
     }

     /// Iterator past the last element (array element or object value).
     iterator end() {
         if (is_array()) {
             auto& arr = std::get<array_type>(value_);
             return iterator(arr.end());
         }
         if (is_object()) {
             auto& obj = std::get<object_type>(value_);
             return iterator(obj.end());
         }
         return iterator{};
     }

     /// Const iterator to the first element.
     const_iterator begin() const {
         if (is_array()) {
             auto const& arr = std::get<array_type>(value_);
             return const_iterator(arr.begin());
         }
         if (is_object()) {
             auto const& obj = std::get<object_type>(value_);
             return const_iterator(obj.begin());
         }
         return const_iterator{};
     }

     /// Const iterator past the last element.
     const_iterator end() const {
         if (is_array()) {
             auto const& arr = std::get<array_type>(value_);
             return const_iterator(arr.end());
         }
         if (is_object()) {
             auto const& obj = std::get<object_type>(value_);
             return const_iterator(obj.end());
         }
         return const_iterator{};
     }

     /// Const iterator to the first element.
     const_iterator cbegin() const { return begin(); }
     /// Const iterator past the last element.
     const_iterator cend() const { return end(); }

     /// Return a view over an object's items (key/value pairs).
     ///
     /// Precondition: `is_object()`.
     items_view items() {
         if (!is_object()) {
             throw std::runtime_error("sj::json::items: not an object");
         }
         auto& obj = std::get<object_type>(value_);
         return items_view(obj);
     }

     /// Find an element in an object by key.
     iterator find(std::string const& key) {
         if (!is_object()) {
             return end();
         }
         auto& obj = std::get<object_type>(value_);
         return iterator(obj.find(key));
     }

     /// Find an element in an object by key.
     iterator find(char const* key) {
         return find(std::string(key));
     }

     /// Find an element in an object by key (const).
     const_iterator find(std::string const& key) const {
         if (!is_object()) {
             return end();
         }
         auto const& obj = std::get<object_type>(value_);
         return const_iterator(obj.find(key));
     }

     /// Find an element in an object by key (const).
     const_iterator find(char const* key) const {
         return find(std::string(key));
     }

     /// Count the number of elements with the given key (0 or 1).
     std::size_t count(std::string const& key) const {
         return contains(key) ? 1u : 0u;
     }

     /// Count the number of elements with the given key (0 or 1).
     std::size_t count(char const* key) const {
         return count(std::string(key));
     }

     /// Erase an element from an object by key. No‑op if not an object.
     void erase(std::string const& key) {
         if (!is_object()) {
             return;
         }
         auto& obj = std::get<object_type>(value_);
         obj.erase(key);
     }

     /// Erase an element from an object by key. No‑op if not an object.
     void erase(char const* key) {
         erase(std::string(key));
     }

     /// Return the value at `key` converted to `T`, or `default_value`
     /// if the key is missing, the value is null, or it cannot be
     /// converted to `T`.
     template <typename T>
     T value_or(std::string const& key, T default_value) const {
         if (!is_object()) {
             return default_value;
         }
         auto const& obj = std::get<object_type>(value_);
         auto it = obj.find(key);
         if (it == obj.end()) {
             return default_value;
         }
         json const& v = it->second;

         if constexpr (std::is_same_v<T, json>) {
             return v;
         } else if constexpr (std::is_same_v<T, boolean_type>) {
             return v.is_boolean() ? v.as_boolean() : default_value;
         } else if constexpr (std::is_same_v<T, integer_type>) {
             return v.is_integer() ? v.as_integer() : default_value;
         } else if constexpr (std::is_same_v<T, int>) {
             if (v.is_integer()) {
                 return static_cast<int>(v.as_integer());
             }
             if (v.is_number()) {
                 return static_cast<int>(v.as_number());
             }
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
             static_assert(std::is_same_v<T, void>,
                           "sj::json::value_or: unsupported type");
         }
     }

     /// Convenience overload taking a C‑string key.
     template <typename T>
     T value_or(char const* key, T default_value) const {
         return value_or(std::string(key), std::move(default_value));
     }

     /// Serialize this JSON value as a string.
     ///
     /// If `indent > 0`, pretty‑prints with the given indentation
     /// width per nesting level. If `indent <= 0`, prints a compact
     /// representation without line breaks.
     std::string dump(int indent = 2) const {
         std::string out;
         dump_impl(out, indent, 0);
         return out;
     }

     /// Compare two JSON values for equality.
     friend bool operator==(json const&, json const&) = default;

 private:
     void ensure_array() {
         if (is_null()) {
             value_ = array_type{};
         } else if (!is_array()) {
             throw std::runtime_error("sj::json: value is not an array");
         }
     }

     void ensure_object() {
         if (is_null()) {
             value_ = object_type{};
         } else if (!is_object()) {
             throw std::runtime_error("sj::json: value is not an object");
         }
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
             if (indent > 0) {
                 out.append(static_cast<std::size_t>(d * indent), ' ');
             }
         };

         if (is_null()) {
             out += "null";
         } else if (is_boolean()) {
             out += as_boolean() ? "true" : "false";
         } else if (is_number()) {
             if (is_integer()) {
                 out += std::to_string(as_integer());
             } else {
                 std::ostringstream oss;
                 oss << as_number();
                 out += oss.str();
             }
         } else if (is_string()) {
             dump_string(out, as_string());
         } else if (is_array()) {
             auto const& arr = std::get<array_type>(value_);
             out.push_back('[');
             if (!arr.empty()) {
                 bool first = true;
                 for (auto const& el : arr) {
                     if (first) {
                         first = false;
                     } else {
                         out.push_back(',');
                     }
                     if (indent > 0) {
                         out.push_back('\n');
                         write_indent(depth + 1);
                     } else {
                         out.push_back(' ');
                     }
                     el.dump_impl(out, indent, depth + 1);
                 }
                 if (indent > 0) {
                     out.push_back('\n');
                     write_indent(depth);
                 }
             }
             out.push_back(']');
         } else if (is_object()) {
             auto const& obj = std::get<object_type>(value_);
             out.push_back('{');
             if (!obj.empty()) {
                 bool first = true;
                 for (auto const& kv : obj) {
                     if (first) {
                         first = false;
                     } else {
                         out.push_back(',');
                     }
                     if (indent > 0) {
                         out.push_back('\n');
                         write_indent(depth + 1);
                     } else {
                         out.push_back(' ');
                     }
                     dump_string(out, kv.first);
                     out.push_back(':');
                     if (indent > 0) {
                         out.push_back(' ');
                     }
                     kv.second.dump_impl(out, indent, depth + 1);
                 }
                 if (indent > 0) {
                     out.push_back('\n');
                     write_indent(depth);
                 }
             }
             out.push_back('}');
         }
     }

     value_type value_;
 };

 namespace detail {

 /// Parse a JSON value from the current token.
 inline json parse_value(Reader& r, Value v);

 /// Parse a JSON array from the current position.
 inline json parse_array(Reader& r, Value arr) {
     json::array_type out;
     Value element{};
     while (true) {
         auto result = iter_array(&r, arr, &element);
         if (!result.has_value()) {
             throw std::runtime_error(result.error());
         }
         if (!result.value()) {
             break;
         }
         out.push_back(parse_value(r, element));
     }
     return json(std::move(out));
 }

 /// Parse a JSON object from the current position.
 inline json parse_object(Reader& r, Value obj) {
     json::object_type out;
     Value key{};
     Value val{};
     while (true) {
         auto result = iter_object(&r, obj, &key, &val);
         if (!result.has_value()) {
             throw std::runtime_error(result.error());
         }
         if (!result.value()) {
             break;
         }
         if (key.type != Type::STRING) {
             throw std::runtime_error("object key is not a string");
         }
         std::string k{key.start, key.end};
         out.emplace(std::move(k), parse_value(r, val));
     }
     return json(std::move(out));
 }

 inline json parse_value(Reader& r, Value v) {
     switch (v.type) {
     case Type::NULL_:
         return json(nullptr);
     case Type::BOOL:
         return json(v.start[0] == 't');
    case Type::NUMBER: {
         std::string num_str{v.start, v.end};
         bool is_integer_literal = true;
         for (char c : num_str) {
             if (c == '.' || c == 'e' || c == 'E') {
                 is_integer_literal = false;
                 break;
             }
         }

         char* end_ptr = nullptr;
         char const* c_str = num_str.c_str();
         if (is_integer_literal) {
             long long iv = std::strtoll(c_str, &end_ptr, 10);
             if (end_ptr != c_str + num_str.size()) {
                 throw std::runtime_error("invalid integer literal");
             }
             return json(static_cast<json::integer_type>(iv));
         } else {
             double dv = std::strtod(c_str, &end_ptr);
             if (end_ptr != c_str + num_str.size()) {
                 throw std::runtime_error("invalid number literal");
             }
             return json(dv);
         }
     }
     case Type::STRING:
         return json(std::string{v.start, v.end});
     case Type::ARRAY:
         return parse_array(r, v);
     case Type::OBJECT:
         return parse_object(r, v);
     case Type::END:
     default:
         throw std::runtime_error("unexpected token");
     }
 }

 } // namespace detail

 /// Parse a JSON document from a character buffer.
 inline json parse(std::string_view sv) {
     Reader r = reader(sv.data(), sv.size());
     auto first = read(&r);
     if (!first.has_value()) {
         throw std::runtime_error(first.error());
     }
     return detail::parse_value(r, first.value());
 }

 /// Parse a JSON document from an input stream.
 inline json parse(std::istream& is) {
     std::ostringstream oss;
     oss << is.rdbuf();
     return parse(oss.str());
 }

 /// User‑defined literal for parsing JSON from string literals.
 namespace literals {

 /// Parse a JSON value from a string literal.
 inline json operator""_json(char const* s, std::size_t n) {
     return parse(std::string_view{s, n});
 }

 } // namespace literals

 /// Stream a JSON value to an output stream using its compact representation.
 inline std::ostream& operator<<(std::ostream& os, json const& j) {
     os << j.dump(0);
     return os;
 }

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
    if constexpr (I == 0) {
        return item.key();
    } else if constexpr (I == 1) {
        return item.value();
    }
}

} // namespace sj

#endif // #ifndef SIMPLE_JSON_HPP
