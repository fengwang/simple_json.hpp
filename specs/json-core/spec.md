# Specification: json Core

## MODIFIED Requirements

### Requirement: sj::json value type

`sj::json` SHALL remain a variant-based value type representing any JSON value: null, boolean, integer, number, string, array, or object. The variant type list is the single source of truth for the type system.

#### Scenario: Null construction
WHEN constructing `sj::json{}` or `sj::json(nullptr)`
THEN the value SHALL be null and `is_null()` SHALL return `true`

#### Scenario: Boolean construction
WHEN constructing `sj::json(true)`
THEN `is_boolean()` SHALL return `true` and `as_boolean()` SHALL return `true`

#### Scenario: Integer construction
WHEN constructing `sj::json(42)` or `sj::json(int64_t{42})`
THEN `is_integer()` SHALL return `true` and `is_number()` SHALL return `true`

#### Scenario: Number construction
WHEN constructing `sj::json(3.14)`
THEN `is_number()` SHALL return `true` and `is_integer()` SHALL return `false`

#### Scenario: String construction
WHEN constructing `sj::json("hello")` or `sj::json(std::string("hello"))`
THEN `is_string()` SHALL return `true`

#### Scenario: Array construction
WHEN constructing `sj::json(sj::json::array_type{1, 2, 3})`
THEN `is_array()` SHALL return `true` and `size()` SHALL return `3`

#### Scenario: Object construction from initializer list
WHEN constructing `sj::json j = {{"key", "value"}}`
THEN `is_object()` SHALL return `true` and `j["key"]` SHALL be `"value"`

#### Scenario: Equality comparison
WHEN two `sj::json` values have the same type and content
THEN `operator==` SHALL return `true`

---

### Requirement: is<T>() generic type query

A new `is<T>()` template method SHALL check if the json value holds type `T`.

#### Scenario: is<T> matches held type
WHEN `sj::json j = 42` and `T` is `sj::json::integer_type`
THEN `j.is<T>()` SHALL return `true`

#### Scenario: is<T> rejects wrong type
WHEN `sj::json j = 42` and `T` is `sj::json::string_type`
THEN `j.is<T>()` SHALL return `false`

---

### Requirement: Named type queries preserved

`is_null()`, `is_boolean()`, `is_integer()`, `is_number()`, `is_string()`, `is_array()`, `is_object()` SHALL remain as named methods, implemented as thin wrappers around `is<T>()`.

#### Scenario: is_number includes integer and double
WHEN `sj::json j = 42`
THEN `j.is_number()` SHALL return `true` (covers both integer_type and number_type)

---

### Requirement: as<T>() returns std::expected

`as<T>()` SHALL return `std::expected<T const&, error>` instead of a bare reference.

#### Scenario: Successful as<T>
WHEN `sj::json j = "hello"` and `T` is `string_type`
THEN `j.as<T>()` SHALL return the string reference

#### Scenario: Failed as<T>
WHEN `sj::json j = 42` and `T` is `string_type`
THEN `j.as<T>()` SHALL return `unexpected` with `error_kind::type`

---

### Requirement: get<T>() returns std::expected

`get<T>()` SHALL return `std::expected<T, error>`. It supports variant alternatives and numeric conversions (int↔double).

#### Scenario: get<int> from integer
WHEN `sj::json j = 42`
THEN `j.get<int>()` SHALL return `42`

#### Scenario: get<double> from integer (numeric conversion)
WHEN `sj::json j = 42`
THEN `j.get<double>()` SHALL return `42.0`

#### Scenario: get<int> from double (numeric conversion)
WHEN `sj::json j = 3.14`
THEN `j.get<int>()` SHALL return `3`

#### Scenario: get<string> from integer (type mismatch)
WHEN `sj::json j = 42`
THEN `j.get<std::string>()` SHALL return `unexpected` with `error_kind::type`

---

### Requirement: value_t enum derived from variant

`value_t` SHALL mirror the variant alternatives. `type()` SHALL return the value's `value_t` using `variant::index()`.

#### Scenario: type() returns correct enum
WHEN `sj::json j = "hello"`
THEN `j.type()` SHALL return `sj::value_t::string`

---

### Requirement: operator[] auto-vivify

`operator[](std::string const&)` SHALL auto-vivify: if the value is null, convert to an empty object and return the reference to the new entry. No change from current behavior.

#### Scenario: Auto-vivify from null
WHEN `sj::json j; j["key"] = 42`
THEN `j` SHALL be an object with `"key"` mapping to `42`

---

### Requirement: at() returns std::expected

`at(key)` and `at(idx)` SHALL return `std::expected<json const&, error>` instead of throwing.

#### Scenario: at() with valid key
WHEN `sj::json j = {{"x", 1}}` and calling `j.at("x")`
THEN it SHALL return a reference to the value `1`

#### Scenario: at() with missing key
WHEN `sj::json j = {{"x", 1}}` and calling `j.at("y")`
THEN it SHALL return `unexpected` with `error_kind::missing`

#### Scenario: at() with valid index
WHEN `sj::json j` is an array `[10, 20]` and calling `j.at(0)`
THEN it SHALL return a reference to `10`

#### Scenario: at() with out-of-range index
WHEN `sj::json j` is an array `[10, 20]` and calling `j.at(5)`
THEN it SHALL return `unexpected` with `error_kind::missing`

---

### Requirement: Container operations preserved

`push_back()`, `emplace_back()`, `emplace()`, `contains()`, `size()`, `empty()`, `clear()`, `erase()`, `find()`, `count()` SHALL be preserved with their current semantics.

#### Scenario: push_back auto-converts null to array
WHEN `sj::json j; j.push_back(1);`
THEN `j` SHALL be an array containing `1`

#### Scenario: contains checks key existence
WHEN `sj::json j = {{"x", 1}}`
THEN `j.contains("x")` SHALL return `true` and `j.contains("y")` SHALL return `false`

#### Scenario: size of array
WHEN `sj::json j` is `[1, 2, 3]`
THEN `j.size()` SHALL return `3`

---

### Requirement: Iterator support preserved

`begin()`, `end()`, `cbegin()`, `cend()` SHALL continue to work for arrays and objects. `items()` SHALL continue to provide structured-binding-compatible iteration over object key/value pairs.

#### Scenario: Range-for over array
WHEN iterating `sj::json j = json::array_type{1, 2, 3}` with range-for
THEN each element SHALL be visited in order

#### Scenario: Structured bindings over items()
WHEN iterating an object with `for (auto [key, value] : j.items())`
THEN `key` SHALL be `std::string const&` and `value` SHALL be `sj::json&`

---

### Requirement: value_or preserved

`value_or(key, default_value)` SHALL return the value at `key` converted to `T`, or `default_value` if the key is missing, the value is null, or conversion fails. Return type is `T`, not `expected`.

#### Scenario: value_or with existing key
WHEN `sj::json j = {{"x", 42}}` and calling `j.value_or("x", 0)`
THEN it SHALL return `42`

#### Scenario: value_or with missing key
WHEN `sj::json j = {{"x", 42}}` and calling `j.value_or("y", -1)`
THEN it SHALL return `-1`

---

### Requirement: dump() serialization

`dump(indent)` SHALL serialize the json value to a string. `std::format` SHALL be used for numeric formatting instead of `std::ostringstream`.

#### Scenario: Pretty-print round trip
WHEN serializing a json value with `dump(2)` and parsing the result
THEN the parsed value SHALL equal the original

#### Scenario: Compact serialization
WHEN serializing with `dump(0)`
THEN the output SHALL contain no newlines

---

### Requirement: operator<< preserved

`operator<<(ostream, json)` SHALL output compact JSON.

#### Scenario: Stream round trip
WHEN streaming a json value and parsing the output
THEN the parsed value SHALL equal the original

---

### Requirement: STL container constructors use concepts

Container constructors SHALL use concept constraints instead of SFINAE `enable_if_t`.

#### Scenario: Vector constructs json array
WHEN constructing `sj::json(std::vector<int>{1, 2, 3})`
THEN the result SHALL be a JSON array `[1, 2, 3]`

#### Scenario: Map constructs json object
WHEN constructing `sj::json(std::map<std::string, int>{{"a", 1}})`
THEN the result SHALL be a JSON object `{"a": 1}`
