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
#include <sstream>
#include <stdexcept>
#include <cstdlib>

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
     /// Type used to represent JSON numbers.
     using number_type = double;
     /// Underlying variant storage type.
     using value_type = std::variant<std::nullptr_t, boolean_type, number_type, string_type, array_type, object_type>;

     /// Construct a null JSON value.
     json() : value_(nullptr) {}
     /// Construct a null JSON value.
     json(std::nullptr_t) : value_(nullptr) {}
     /// Construct a boolean JSON value.
     json(boolean_type b) : value_(b) {}
     /// Construct a numeric JSON value from a floating‑point number.
     json(number_type n) : value_(n) {}
     /// Construct a numeric JSON value from an integer.
     json(int n) : value_(static_cast<number_type>(n)) {}
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
     /// Return true if this value is a number.
     bool is_number() const { return std::holds_alternative<number_type>(value_); }
     /// Return true if this value is a string.
     bool is_string() const { return std::holds_alternative<string_type>(value_); }
     /// Return true if this value is an array.
     bool is_array() const { return std::holds_alternative<array_type>(value_); }
     /// Return true if this value is an object.
     bool is_object() const { return std::holds_alternative<object_type>(value_); }

     /// Access the contained boolean. Precondition: `is_boolean()`.
     boolean_type as_boolean() const { return std::get<boolean_type>(value_); }
     /// Access the contained number. Precondition: `is_number()`.
     number_type as_number() const { return std::get<number_type>(value_); }
     /// Access the contained string. Precondition: `is_string()`.
     string_type const& as_string() const { return std::get<string_type>(value_); }
     /// Access the contained array. Precondition: `is_array()`.
     array_type const& as_array() const { return std::get<array_type>(value_); }
     /// Access the contained object. Precondition: `is_object()`.
     object_type const& as_object() const { return std::get<object_type>(value_); }

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

     /// Access an array element by index. Precondition: `is_array()`.
     json& operator[](std::size_t idx) {
         return std::get<array_type>(value_)[idx];
     }

     /// Access an array element by index (const). Precondition: `is_array()`.
     json const& at(std::size_t idx) const {
         return std::get<array_type>(value_).at(idx);
     }

     /// Compare two JSON values for equality.
     friend bool operator==(json const&, json const&) = default;

 private:
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
         char* end_ptr = nullptr;
         char const* c_str = num_str.c_str();
         double value = std::strtod(c_str, &end_ptr);
         if (end_ptr != c_str + num_str.size()) {
             throw std::runtime_error("invalid number literal");
         }
         return json(value);
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

 } // namespace sj

#endif // #ifndef SIMPLE_JSON_HPP
