# Specification: Concept System

## ADDED Requirements

### Requirement: JsonSerializable concept

A type `T` satisfies `JsonSerializable` if a valid `to_json(T const&) -> sj::json` overload is found via ADL or in `namespace sj`.

#### Scenario: Scalar types satisfy JsonSerializable
WHEN `T` is `bool`, `int`, `std::int64_t`, `double`, `std::string`, `char const*`, or `std::nullptr_t`
THEN `sj::JsonSerializable<T>` SHALL be `true`

#### Scenario: sj::json satisfies JsonSerializable
WHEN `T` is `sj::json`
THEN `sj::JsonSerializable<T>` SHALL be `true`

#### Scenario: Aggregate types satisfy JsonSerializable
WHEN `T` is an aggregate type whose every nonstatic data member type is itself `JsonSerializable`
THEN `sj::JsonSerializable<T>` SHALL be `true`

#### Scenario: Enum types satisfy JsonSerializable
WHEN `T` is a scoped or unscoped enum type
THEN `sj::JsonSerializable<T>` SHALL be `true`

#### Scenario: STL containers with serializable elements satisfy JsonSerializable
WHEN `T` is a `std::ranges::input_range` whose `range_value_t` is `JsonSerializable` (for sequences) or whose value_type is a pair-like with string-convertible key and `JsonSerializable` mapped type (for mappings)
THEN `sj::JsonSerializable<T>` SHALL be `true`

#### Scenario: std::optional satisfies JsonSerializable
WHEN `T` is `std::optional<U>` and `U` is `JsonSerializable`
THEN `sj::JsonSerializable<T>` SHALL be `true`

#### Scenario: std::tuple and std::pair satisfy JsonSerializable
WHEN `T` is `std::tuple<Ts...>` or `std::pair<U,V>` and every element type is `JsonSerializable`
THEN `sj::JsonSerializable<T>` SHALL be `true`

#### Scenario: std::variant satisfies JsonSerializable
WHEN `T` is `std::variant<Ts...>` and every alternative type is `JsonSerializable`
THEN `sj::JsonSerializable<T>` SHALL be `true`

#### Scenario: Smart pointers satisfy JsonSerializable
WHEN `T` is `std::unique_ptr<U>` or `std::shared_ptr<U>` and `U` is `JsonSerializable`
THEN `sj::JsonSerializable<T>` SHALL be `true`

---

### Requirement: JsonDeserializable concept

A type `T` satisfies `JsonDeserializable` if a valid `from_json<T>(sj::json const&) -> std::expected<T, sj::error>` overload is found via ADL or in `namespace sj`.

#### Scenario: Scalar types satisfy JsonDeserializable
WHEN `T` is `bool`, `int`, `std::int64_t`, `double`, or `std::string`
THEN `sj::JsonDeserializable<T>` SHALL be `true`

#### Scenario: Aggregate types satisfy JsonDeserializable
WHEN `T` is an aggregate type whose every nonstatic data member type is itself `JsonDeserializable`
THEN `sj::JsonDeserializable<T>` SHALL be `true`

#### Scenario: Enum types satisfy JsonDeserializable
WHEN `T` is a scoped or unscoped enum type
THEN `sj::JsonDeserializable<T>` SHALL be `true`

#### Scenario: BackInsertable containers satisfy JsonDeserializable
WHEN `T` has `push_back()` or `emplace_back()`, is a range, and its `value_type` is `JsonDeserializable`
THEN `sj::JsonDeserializable<T>` SHALL be `true`

#### Scenario: Insertable containers satisfy JsonDeserializable
WHEN `T` has `insert()`, is a range, and its `value_type` is `JsonDeserializable`
THEN `sj::JsonDeserializable<T>` SHALL be `true`

#### Scenario: FixedSizeSequence (std::array) satisfies JsonDeserializable
WHEN `T` is `std::array<U, N>` and `U` is `JsonDeserializable`
THEN `sj::JsonDeserializable<T>` SHALL be `true`

#### Scenario: Mapping containers satisfy JsonDeserializable
WHEN `T` is a range with pair-like `value_type`, string-constructible key, `JsonDeserializable` mapped type, and an `emplace()` or `insert()` method
THEN `sj::JsonDeserializable<T>` SHALL be `true`

#### Scenario: std::optional satisfies JsonDeserializable
WHEN `T` is `std::optional<U>` and `U` is `JsonDeserializable`
THEN `sj::JsonDeserializable<T>` SHALL be `true`

#### Scenario: std::tuple and std::pair satisfy JsonDeserializable
WHEN `T` is `std::tuple<Ts...>` or `std::pair<U,V>` and every element type is `JsonDeserializable`
THEN `sj::JsonDeserializable<T>` SHALL be `true`

#### Scenario: std::variant satisfies JsonDeserializable
WHEN `T` is `std::variant<Ts...>` and every alternative type is `JsonDeserializable`
THEN `sj::JsonDeserializable<T>` SHALL be `true`

---

### Requirement: JsonConvertible concept

A type `T` satisfies `JsonConvertible` if it satisfies both `JsonSerializable` and `JsonDeserializable`.

#### Scenario: Common types satisfy JsonConvertible
WHEN `T` is `int`, `double`, `bool`, `std::string`, `sj::json`, a user aggregate with convertible members, or an enum type
THEN `sj::JsonConvertible<T>` SHALL be `true`

---

### Requirement: SequenceSerializable concept

Detects sequence containers eligible for JSON array serialization.

#### Scenario: Vector of ints is SequenceSerializable
WHEN `T` is `std::vector<int>`
THEN `sj::SequenceSerializable<T>` SHALL be `true`

#### Scenario: Map is not SequenceSerializable
WHEN `T` is `std::map<std::string, int>`
THEN `sj::SequenceSerializable<T>` SHALL be `false`

#### Scenario: sj::json is not SequenceSerializable
WHEN `T` is `sj::json`
THEN `sj::SequenceSerializable<T>` SHALL be `false`

---

### Requirement: MappingSerializable concept

Detects mapping containers eligible for JSON object serialization.

#### Scenario: Map with string keys is MappingSerializable
WHEN `T` is `std::map<std::string, int>`
THEN `sj::MappingSerializable<T>` SHALL be `true`

#### Scenario: Map with non-string keys is not MappingSerializable
WHEN `T` is `std::map<int, int>`
THEN `sj::MappingSerializable<T>` SHALL be `false`

---

### Requirement: BackInsertable concept

Detects containers that support `push_back` or `emplace_back`.

#### Scenario: Vector is BackInsertable
WHEN `T` is `std::vector<int>`
THEN `sj::BackInsertable<T>` SHALL be `true`

#### Scenario: Set is not BackInsertable
WHEN `T` is `std::set<int>`
THEN `sj::BackInsertable<T>` SHALL be `false`

---

### Requirement: Insertable concept

Detects containers that support `insert`.

#### Scenario: Set is Insertable
WHEN `T` is `std::set<int>`
THEN `sj::Insertable<T>` SHALL be `true`

---

### Requirement: FixedSizeSequence concept

Detects fixed-size sequence containers.

#### Scenario: std::array is FixedSizeSequence
WHEN `T` is `std::array<int, 3>`
THEN `sj::FixedSizeSequence<T>` SHALL be `true`

#### Scenario: Deserialization enforces size match
WHEN deserializing a JSON array of 2 elements into `std::array<int, 3>`
THEN `from_json` SHALL return an error with `error_kind::conversion`
