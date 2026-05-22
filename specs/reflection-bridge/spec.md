# Specification: Reflection Bridge

## ADDED Requirements

### Requirement: to_json for aggregates

`to_json(T const&)` SHALL produce an `sj::json` object where each key is the field name (via `identifier_of`) and each value is the recursive `to_json` of that field.

#### Scenario: Flat aggregate serialization
WHEN `T` is `struct { int x; double y; std::string name; }` with values `{1, 2.5, "hello"}`
THEN `to_json(t)` SHALL produce `{"x": 1, "y": 2.5, "name": "hello"}`

#### Scenario: Nested aggregate serialization
WHEN `T` is `struct Outer { std::string label; struct Inner { int a; } child; }` with values `{"top", {42}}`
THEN `to_json(t)` SHALL produce `{"label": "top", "child": {"a": 42}}`

#### Scenario: Aggregate with container members
WHEN `T` is `struct { std::vector<int> ids; std::map<std::string, double> scores; }`
THEN `to_json(t)` SHALL produce `{"ids": [...], "scores": {...}}` with correctly serialized contents

---

### Requirement: from_json for aggregates

`from_json<T>(sj::json const&)` SHALL return `std::expected<T, error>`. Each JSON object key is matched by name to a struct field. Missing required fields produce an error with the field path.

#### Scenario: Flat aggregate deserialization
WHEN JSON is `{"x": 1, "y": 2.5, "name": "hello"}` and `T` is `struct { int x; double y; std::string name; }`
THEN `from_json<T>(j)` SHALL return the struct with matching values

#### Scenario: Missing required field
WHEN JSON is `{"x": 1}` and `T` is `struct { int x; int y; }`
THEN `from_json<T>(j)` SHALL return an error with `error_kind::missing` and path `"y"`

#### Scenario: Missing optional field
WHEN JSON is `{"x": 1}` and `T` is `struct { int x; std::optional<int> y; }`
THEN `from_json<T>(j)` SHALL succeed with `y == std::nullopt`

#### Scenario: Type mismatch in field
WHEN JSON is `{"x": "not_a_number"}` and `T` is `struct { int x; }`
THEN `from_json<T>(j)` SHALL return an error with `error_kind::type` and path `"x"`

#### Scenario: Nested error path propagation
WHEN JSON is `{"child": {"x": "wrong"}}` and `T` is `struct Outer { struct Inner { int x; } child; }`
THEN `from_json<T>(j)` SHALL return an error with path `"child.x"`

#### Scenario: Extra fields are ignored
WHEN JSON is `{"x": 1, "extra": "ignored"}` and `T` is `struct { int x; }`
THEN `from_json<T>(j)` SHALL succeed, ignoring `"extra"`

---

### Requirement: to_json for enums

`to_json(E)` SHALL serialize an enum value to a JSON string using its enumerator name (via `enumerators_of`).

#### Scenario: Known enumerator
WHEN `E` is `enum class Color { Red, Green, Blue }` and value is `Color::Green`
THEN `to_json(e)` SHALL produce `"Green"`

#### Scenario: Unknown enumerator value
WHEN `E` is `enum class Color { Red = 0 }` and value is `static_cast<Color>(99)`
THEN `to_json(e)` SHALL produce `"unknown(99)"`

---

### Requirement: from_json for enums

`from_json<E>(j)` SHALL deserialize a JSON string to an enum by matching against enumerator names.

#### Scenario: Valid enumerator name
WHEN JSON is `"Green"` and `E` is `enum class Color { Red, Green, Blue }`
THEN `from_json<E>(j)` SHALL return `Color::Green`

#### Scenario: Invalid enumerator name
WHEN JSON is `"Purple"` and `E` is `enum class Color { Red, Green, Blue }`
THEN `from_json<E>(j)` SHALL return an error with `error_kind::conversion`

#### Scenario: Non-string value for enum
WHEN JSON is `42` and `E` is `enum class Color { Red, Green }`
THEN `from_json<E>(j)` SHALL return an error with `error_kind::type`

---

### Requirement: to_json for scalars

Built-in overloads for scalar types.

#### Scenario: bool serialization
WHEN value is `true`
THEN `to_json(true)` SHALL produce a JSON boolean `true`

#### Scenario: integer serialization
WHEN value is `42`
THEN `to_json(42)` SHALL produce a JSON integer `42`

#### Scenario: double serialization
WHEN value is `3.14`
THEN `to_json(3.14)` SHALL produce a JSON number `3.14`

#### Scenario: string serialization
WHEN value is `std::string("hello")`
THEN `to_json(s)` SHALL produce a JSON string `"hello"`

#### Scenario: nullptr serialization
WHEN value is `nullptr`
THEN `to_json(nullptr)` SHALL produce JSON `null`

---

### Requirement: to_json / from_json for std::optional

#### Scenario: optional with value serialization
WHEN value is `std::optional<int>(42)`
THEN `to_json(opt)` SHALL produce JSON integer `42`

#### Scenario: empty optional serialization
WHEN value is `std::optional<int>(std::nullopt)`
THEN `to_json(opt)` SHALL produce JSON `null`

#### Scenario: optional deserialization from value
WHEN JSON is `42` and target is `std::optional<int>`
THEN `from_json` SHALL return `std::optional<int>(42)`

#### Scenario: optional deserialization from null
WHEN JSON is `null` and target is `std::optional<int>`
THEN `from_json` SHALL return `std::nullopt`

---

### Requirement: to_json / from_json for std::variant

#### Scenario: variant serialization
WHEN value is `std::variant<int, std::string>` holding `std::string("hello")`
THEN `to_json(v)` SHALL produce `{"_type": "std::string", "_value": "hello"}`

#### Scenario: variant deserialization
WHEN JSON is `{"_type": "int", "_value": 42}` and target is `std::variant<int, std::string>`
THEN `from_json` SHALL return the variant holding `42`

#### Scenario: variant with unknown type tag
WHEN JSON is `{"_type": "float", "_value": 1.0}` and target is `std::variant<int, std::string>`
THEN `from_json` SHALL return an error with `error_kind::conversion`

---

### Requirement: to_json / from_json for std::tuple and std::pair

#### Scenario: tuple serialization
WHEN value is `std::tuple<int, std::string, bool>(1, "hi", true)`
THEN `to_json(t)` SHALL produce JSON array `[1, "hi", true]`

#### Scenario: tuple deserialization
WHEN JSON is `[1, "hi", true]` and target is `std::tuple<int, std::string, bool>`
THEN `from_json` SHALL return the matching tuple

#### Scenario: tuple size mismatch
WHEN JSON array has 2 elements and target is `std::tuple<int, int, int>`
THEN `from_json` SHALL return an error with `error_kind::conversion`

#### Scenario: pair serialization
WHEN value is `std::pair<int, std::string>(1, "one")`
THEN `to_json(p)` SHALL produce JSON array `[1, "one"]`

---

### Requirement: to_json / from_json for smart pointers

#### Scenario: non-null unique_ptr serialization
WHEN value is `std::unique_ptr<int>` pointing to `42`
THEN `to_json(p)` SHALL produce JSON integer `42`

#### Scenario: null unique_ptr serialization
WHEN value is a null `std::unique_ptr<int>`
THEN `to_json(p)` SHALL produce JSON `null`

#### Scenario: unique_ptr deserialization from value
WHEN JSON is `42` and target is `std::unique_ptr<int>`
THEN `from_json` SHALL return a `unique_ptr` holding `42`

#### Scenario: unique_ptr deserialization from null
WHEN JSON is `null` and target is `std::unique_ptr<int>`
THEN `from_json` SHALL return a null `unique_ptr`

#### Scenario: shared_ptr follows same rules
WHEN value is `std::shared_ptr<std::string>` pointing to `"hello"`
THEN `to_json(p)` SHALL produce `"hello"`

---

### Requirement: to_json / from_json for sequence containers

#### Scenario: vector serialization
WHEN value is `std::vector<int>{1, 2, 3}`
THEN `to_json(v)` SHALL produce JSON array `[1, 2, 3]`

#### Scenario: vector deserialization
WHEN JSON is `[1, 2, 3]` and target is `std::vector<int>`
THEN `from_json` SHALL return `std::vector<int>{1, 2, 3}`

#### Scenario: set serialization
WHEN value is `std::set<std::string>{"a", "b"}`
THEN `to_json(s)` SHALL produce a JSON array containing `"a"` and `"b"`

#### Scenario: set deserialization
WHEN JSON is `["a", "b", "a"]` and target is `std::set<std::string>`
THEN `from_json` SHALL return a set containing `"a"` and `"b"` (deduplicated)

#### Scenario: std::array deserialization with matching size
WHEN JSON is `[1, 2, 3]` and target is `std::array<int, 3>`
THEN `from_json` SHALL succeed with `{1, 2, 3}`

#### Scenario: std::array deserialization with mismatched size
WHEN JSON is `[1, 2]` and target is `std::array<int, 3>`
THEN `from_json` SHALL return an error with `error_kind::conversion`

---

### Requirement: to_json / from_json for mapping containers

#### Scenario: map serialization
WHEN value is `std::map<std::string, int>{{"a", 1}, {"b", 2}}`
THEN `to_json(m)` SHALL produce JSON object `{"a": 1, "b": 2}`

#### Scenario: map deserialization
WHEN JSON is `{"a": 1, "b": 2}` and target is `std::map<std::string, int>`
THEN `from_json` SHALL return the matching map

#### Scenario: unordered_map serialization
WHEN value is `std::unordered_map<std::string, double>{{"x", 1.5}}`
THEN `to_json(m)` SHALL produce a JSON object with key `"x"` and value `1.5`

---

### Requirement: ADL customization point

Users SHALL be able to override `to_json` and `from_json` for their types by providing overloads in their type's namespace, taking priority over the reflection fallback.

#### Scenario: User-provided to_json takes priority
WHEN a user defines `json to_json(MyType const&)` in `MyType`'s namespace
THEN that overload SHALL be called instead of the reflection-generated one

#### Scenario: User-provided from_json takes priority
WHEN a user defines `expected<MyType, error> from_json(json const&)` overloaded for `MyType`'s namespace resolution
THEN that overload SHALL be called instead of the reflection-generated one
