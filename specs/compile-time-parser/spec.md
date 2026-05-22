# Specification: Compile-Time Parser

## ADDED Requirements

### Requirement: Consteval JSON parsing

A consteval function SHALL parse a JSON string literal into a statically-typed C++ aggregate at compile time using `define_aggregate` and `data_member_spec`.

#### Scenario: Simple object parsing
WHEN JSON string is `{"port": 8080, "host": "localhost"}`
THEN the consteval parser SHALL produce an aggregate with `int port` and `char const* host` members

#### Scenario: Nested object parsing
WHEN JSON string is `{"server": {"host": "0.0.0.0", "port": 443}}`
THEN the consteval parser SHALL produce a nested aggregate where `server` is itself an aggregate with `host` and `port` members

#### Scenario: Boolean value parsing
WHEN JSON string is `{"debug": true, "verbose": false}`
THEN the consteval parser SHALL produce an aggregate with `bool debug` and `bool verbose` members

#### Scenario: Null value parsing
WHEN JSON string is `{"data": null}`
THEN the consteval parser SHALL produce an aggregate with `std::nullptr_t data` member

#### Scenario: Array value parsing as tuple
WHEN JSON string is `{"items": [1, "two", true]}`
THEN the consteval parser SHALL produce an aggregate where `items` is `std::tuple<int, char const*, bool>`

#### Scenario: Homogeneous array parsing
WHEN JSON string is `{"ports": [80, 443, 8080]}`
THEN the consteval parser SHALL produce an aggregate where `ports` is `std::tuple<int, int, int>`

#### Scenario: Invalid JSON at compile time
WHEN JSON string is `{"broken": }`
THEN the consteval parser SHALL produce a compile-time error

---

### Requirement: Compile-time type inference

The consteval parser SHALL infer C++ types from JSON values according to fixed rules.

#### Scenario: Integer inference
WHEN a JSON number has no decimal point or exponent (e.g., `42`, `-7`)
THEN the inferred type SHALL be `int`

#### Scenario: Large integer inference
WHEN a JSON integer exceeds `int` range (e.g., `9999999999`)
THEN the inferred type SHALL be `long long`

#### Scenario: Floating-point inference
WHEN a JSON number has a decimal point or exponent (e.g., `3.14`, `1e5`)
THEN the inferred type SHALL be `double`

#### Scenario: String inference
WHEN a JSON value is a quoted string (e.g., `"hello"`)
THEN the inferred type SHALL be `char const*`

#### Scenario: Boolean inference
WHEN a JSON value is `true` or `false`
THEN the inferred type SHALL be `bool`

#### Scenario: Null inference
WHEN a JSON value is `null`
THEN the inferred type SHALL be `std::nullptr_t`

#### Scenario: Object inference
WHEN a JSON value is `{...}`
THEN the inferred type SHALL be an anonymous aggregate (recursive)

#### Scenario: Array inference
WHEN a JSON value is `[...]`
THEN the inferred type SHALL be `std::tuple<Ts...>` with per-element inference

---

### Requirement: json_literal proxy type

`json_literal<R>` SHALL wrap a compile-time aggregate value and provide:
- `.value()` for direct compile-time aggregate access
- Implicit conversion `operator json()` for runtime `sj::json` conversion
- `.get()` for explicit `sj::json` conversion
- Satisfaction of `JsonSerializable` concept

#### Scenario: Compile-time aggregate access
WHEN `constexpr auto cfg = R"({"port": 8080})"_json`
THEN `cfg.value().port` SHALL equal `8080` and be usable in `static_assert`

#### Scenario: Implicit conversion to sj::json
WHEN `sj::json j = R"({"name": "Alice"})"_json`
THEN `j` SHALL be an `sj::json` object with key `"name"` and string value `"Alice"`

#### Scenario: Explicit get()
WHEN `auto j = R"({"x": 1})"_json.get()`
THEN `j` SHALL be of type `sj::json`

#### Scenario: JsonSerializable satisfaction
WHEN `T` is `json_literal<R>` for any `R`
THEN `sj::JsonSerializable<T>` SHALL be `true`

#### Scenario: Nested compile-time access
WHEN `constexpr auto v = R"({"inner": {"x": 42}})"_json`
THEN `v.value().inner.x` SHALL equal `42`

#### Scenario: Compile-time tuple access
WHEN `constexpr auto v = R"({"items": [1, "hi"]})"_json`
THEN `std::get<0>(v.value().items)` SHALL equal `1`
AND `std::string_view(std::get<1>(v.value().items))` SHALL equal `"hi"`

---

### Requirement: Unified _json literal

A single `_json` template UDL SHALL handle both compile-time aggregate production and runtime `sj::json` conversion, returning `json_literal<R>`.

#### Scenario: Literal used in constexpr context
WHEN `constexpr auto v = "..."_json`
THEN `v` SHALL be a `json_literal<R>` with compile-time accessible members

#### Scenario: Literal assigned to sj::json
WHEN `sj::json j = "..."_json`
THEN `j` SHALL be a valid `sj::json` value produced via implicit conversion

#### Scenario: Literal passed to JsonSerializable parameter
WHEN a function accepts `JsonSerializable auto const&` and receives `"..."_json`
THEN the function call SHALL compile and the literal SHALL be usable as a JsonSerializable value
