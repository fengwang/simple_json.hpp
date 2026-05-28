# Specification: Error Handling

## ADDED Requirements

### Requirement: error_kind enum

`sj::error_kind` SHALL enumerate all error categories.

#### Scenario: Parse errors
WHEN JSON text is malformed
THEN the error kind SHALL be `error_kind::parse`

#### Scenario: Type mismatch errors
WHEN a value is accessed as the wrong type (e.g., `get<int>` on a string)
THEN the error kind SHALL be `error_kind::type`

#### Scenario: Missing field errors
WHEN a required field is absent during `from_json` or `at()` on a missing key
THEN the error kind SHALL be `error_kind::missing`

#### Scenario: Conversion errors
WHEN a value cannot be converted (e.g., unknown enum name, array size mismatch)
THEN the error kind SHALL be `error_kind::conversion`

#### Scenario: Overflow errors
WHEN a container capacity is exceeded (e.g., too many elements for `inplace_vector`)
THEN the error kind SHALL be `error_kind::overflow`

---

### Requirement: sj::error type

`sj::error` SHALL contain `kind` (`error_kind`), `message` (`std::string`), and `path` (`std::string`).

#### Scenario: Error construction
WHEN creating `sj::error{error_kind::type, "expected integer", ""}`
THEN `e.kind` SHALL be `error_kind::type`, `e.message` SHALL be `"expected integer"`, and `e.path` SHALL be `""`

#### Scenario: with_context prepends path
WHEN calling `e.with_context("child")` on an error with path `"x"`
THEN the resulting error's path SHALL be `"child.x"`

#### Scenario: with_context on empty path
WHEN calling `e.with_context("field")` on an error with path `""`
THEN the resulting error's path SHALL be `"field"`

#### Scenario: Deeply nested path
WHEN errors propagate through 3 levels: field `"a"` contains field `"b"` contains field `"c"` where the error originates
THEN the final error path SHALL be `"a.b.c"`

---

## MODIFIED Requirements

### Requirement: parse returns std::expected

`sj::parse(string_view)` and `sj::parse(istream&)` SHALL return `std::expected<json, error>` instead of throwing on invalid input.

#### Scenario: Valid JSON parsing
WHEN input is `{"x": 1}`
THEN `sj::parse(input)` SHALL return a json object

#### Scenario: Invalid JSON parsing
WHEN input is `{"broken": }`
THEN `sj::parse(input)` SHALL return `unexpected` with `error_kind::parse`

#### Scenario: Empty input
WHEN input is `""`
THEN `sj::parse(input)` SHALL return `unexpected` with `error_kind::parse`

#### Scenario: Unclosed string
WHEN input is `{"key": "unclosed}`
THEN `sj::parse(input)` SHALL return `unexpected` with `error_kind::parse`

---

## REMOVED Requirements

### Requirement: Exception throwing

All `throw std::runtime_error(...)` calls SHALL be removed from the library.

**Reason**: Replaced by `std::expected` return types throughout.

**Migration**: All call sites that previously caught exceptions must check `.has_value()` or use monadic operations (`.and_then()`, `.transform()`, `.or_else()`).

#### Scenario: No exceptions thrown
WHEN any library function is called with any input
THEN it SHALL NOT throw any exception
