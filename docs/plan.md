# Implementation Plan

TDD-style micro-task plan. Each section maps to a task group from `tasks.md`. 
Each micro-step follows: RED (write failing test) → GREEN (implement) → REFACTOR.

Compiler: `g++ -std=c++26 -freflection -Wall -Wextra -O2`

---

## Phase 1: Infrastructure Setup (Tasks 1.1–1.4)

### Step 1.1: Update Makefile

**File**: `Makefile`

```makefile
CC := g++
CFLAGS := -std=c++26 -freflection -O2 -Wall -Wextra

# Test build
TEST_SRCS := $(wildcard test/*.cpp)
TEST_OBJS := $(TEST_SRCS:.cpp=.o)

test: run_tests
	./run_tests

run_tests: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test/%.o: test/%.cpp simple_json.hpp test/test_harness.hpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f run_tests test/*.o $(DEMOS)
```

**Verify**: `make test` compiles (will fail until tests exist).

### Step 1.2: Create test harness

**File**: `test/test_harness.hpp`

Implement:
- `sj::test::test_case` struct (name, source_location, function pointer)
- `sj::test::registry()` — global vector of test cases
- `sj::test::registrar` — RAII registration
- `TEST(name)` macro
- `EXPECT_EQ(a, b)` — uses `std::format` for output, `std::source_location`
- `EXPECT_TRUE(expr)`, `EXPECT_FALSE(expr)`
- `EXPECT_OK(expr)`, `EXPECT_ERR(expr)`, `EXPECT_ERR_KIND(expr, k)`
- `sj::test::run_all()` — iterate registry, catch failures, print summary

### Step 1.3: Create test entry point

**File**: `test/test_main.cpp`
```cpp
#include "test_harness.hpp"
int main() { return sj::test::run_all(); }
```

### Step 1.4: Trivial verification test

**File**: `test/test_smoke.cpp`
```cpp
#include "test_harness.hpp"
TEST(smoke_true)  { EXPECT_TRUE(true); }
TEST(smoke_eq)    { EXPECT_EQ(1 + 1, 2); }
TEST(smoke_false) { EXPECT_FALSE(false); }
```

**Verify**: `make test` → `3/3 tests passed`

**COMMIT**: `"add test harness and build infrastructure"`

---

## Phase 2: Error Type Foundation (Tasks 2.1–2.2)

### Step 2.1: Define error types

**File**: `simple_json.hpp` (top of `namespace sj`)

```cpp
enum class error_kind { parse, type, missing, conversion, overflow };

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
```

### Step 2.2: RED — Write error tests

**File**: `test/test_errors.cpp`

Tests:
- Error construction with each `error_kind`
- `with_context` on empty path → sets path
- `with_context` on non-empty path → prepends
- Chain of 3 `with_context` calls → `"a.b.c"`

### Step 2.2: GREEN — Error type already implemented in 2.1

**Verify**: `make test` → all error tests pass

**COMMIT**: `"add sj::error type with path propagation"`

---

## Phase 3: Core `sj::json` Redesign (Tasks 3.1–3.15)

This is the largest phase. Break into sub-phases.

### Step 3.1–3.2: Variant + type queries

**RED**: `test/test_json_core.cpp` — tests for `is<T>()`, `is_null()`, `is_boolean()`, etc.

**GREEN**: Rewrite the json class variant definition. Implement `is<T>()` and named wrappers.

Key code:
```cpp
template <typename T>
bool is() const { return std::holds_alternative<T>(value_); }

bool is_null()    const { return is<null_type>(); }
bool is_boolean() const { return is<boolean_type>(); }
// ... etc
bool is_number()  const { return is<integer_type>() || is<number_type>(); }
```

### Step 3.3–3.4: as<T>() and get<T>()

**RED**: Tests for `as<T>()` success/failure, `get<T>()` with numeric conversions, type mismatches.

**GREEN**: Implement returning `std::expected`.

```cpp
template <typename T>
std::expected<T const&, error> as() const {
    if (auto* p = std::get_if<T>(&value_))
        return *p;
    return std::unexpected(error{error_kind::type, "type mismatch", ""});
}
```

### Step 3.5: value_t enum and type()

**RED**: Test `type()` returns correct enum for each variant alternative.

**GREEN**:
```cpp
enum class value_t { null, boolean, integer, number, string, array, object };
value_t type() const { return static_cast<value_t>(value_.index()); }
```

### Step 3.6: Constructors with concepts

**RED**: Tests for constructing from scalars, STL containers, initializer lists.

**GREEN**: Replace `enable_if_t` with `requires` clauses:
```cpp
template <typename Container>
    requires SequenceSerializable<std::decay_t<Container>>
         && (!std::is_same_v<std::decay_t<Container>, json>)
         && (!std::is_same_v<std::decay_t<Container>, array_type>)
json(Container const& c) : value_(array_type{}) { ... }
```

### Step 3.7: operator[] and at()

**RED**: Tests for auto-vivify, `at()` returning expected for valid/missing keys and indices.

**GREEN**: `operator[]` unchanged. `at()` rewritten:
```cpp
std::expected<json const&, error> at(std::string const& key) const {
    if (!is_object())
        return std::unexpected(error{error_kind::type, "not an object", ""});
    auto const& obj = std::get<object_type>(value_);
    auto it = obj.find(key);
    if (it == obj.end())
        return std::unexpected(error{error_kind::missing, 
            std::format("key '{}' not found", key), ""});
    return it->second;
}
```

### Step 3.8–3.10: Container ops, iterators, value_or

**RED**: Tests for push_back, emplace, contains, size, empty, clear, erase, find, count, begin/end, items(), value_or.

**GREEN**: Preserve existing implementations, modernize where needed.

### Step 3.11–3.12: dump() and operators

**RED**: Tests for dump(2), dump(0), round-trip, operator<<.

**GREEN**: Replace `std::ostringstream` with `std::format_to`:
```cpp
// Instead of: oss << as_number(); out += oss.str();
std::format_to(std::back_inserter(out), "{}", as_integer());
```

**Verify**: `make test` → all json core tests pass

**COMMIT**: `"redesign sj::json core with reflection and std::expected"`

---

## Phase 4: Runtime Parser Rewrite (Tasks 4.1–4.4)

### Step 4.1–4.3: Parser implementation

**RED**: `test/test_parse.cpp` — valid objects, arrays, nested, strings with escapes, numbers (int/float), booleans, null, invalid JSON, empty input, unclosed strings, stray brackets.

**GREEN**: Rewrite parser internally (can reuse streaming logic as private implementation detail, just not public). Return `std::expected<json, error>` from `parse()`.

```cpp
inline std::expected<json, error> parse(std::string_view sv) {
    // Internal parser implementation
    // Returns unexpected on any parse error
}

inline std::expected<json, error> parse(std::istream& is) {
    std::ostringstream oss;
    oss << is.rdbuf();
    return parse(oss.str());
}
```

**Verify**: `make test` → all parse tests pass

**COMMIT**: `"rewrite parser with std::expected, drop public streaming API"`

---

## Phase 5: Concept System (Tasks 5.1–5.5)

### Step 5.1–5.4: Define concepts

**RED**: `test/test_concepts.cpp` — static_assert battery:
```cpp
static_assert(sj::JsonSerializable<int>);
static_assert(sj::JsonSerializable<std::vector<int>>);
static_assert(sj::JsonSerializable<std::map<std::string, double>>);
static_assert(!sj::MappingSerializable<std::map<int, int>>);
// ... etc
```

**GREEN**: Define concepts in `simple_json.hpp`:
```cpp
template <typename T>
concept JsonSerializable = requires(T const& v) {
    { to_json(v) } -> std::same_as<json>;
};
```

Note: Concepts and `to_json`/`from_json` overloads are mutually dependent. Define forward declarations and concepts together, then implementations.

**Verify**: `make test` → concept static_asserts compile

**COMMIT**: `"add concept system: JsonSerializable, JsonDeserializable, container detection"`

---

## Phase 6: Reflection Bridge — Scalars (Tasks 6.1–6.2)

### Step 6.1–6.2: Scalar overloads

**RED**: Tests for `to_json(42)`, `to_json("hello")`, `to_json(true)`, `to_json(nullptr)`, `to_json(3.14)` and their `from_json` counterparts.

**GREEN**: Implement straightforward overloads:
```cpp
inline json to_json(std::nullptr_t)           { return json(nullptr); }
inline json to_json(bool b)                   { return json(b); }
inline json to_json(std::integral auto n)     { return json(static_cast<json::integer_type>(n)); }
inline json to_json(std::floating_point auto n) { return json(static_cast<json::number_type>(n)); }
inline json to_json(std::string const& s)     { return json(s); }
inline json to_json(char const* s)            { return json(s); }
inline json to_json(json const& j)            { return j; }
```

**COMMIT**: `"add to_json/from_json for scalars and json identity"`

---

## Phase 7: Reflection Bridge — Aggregates (Tasks 7.1–7.3)

### Step 7.1–7.2: Aggregate serialization/deserialization

**RED**: `test/test_reflect_struct.cpp` — flat struct, nested struct, missing field, type mismatch, extra fields, optional fields.

**GREEN**: Implement using reflection:
```cpp
template <typename T>
    requires std::is_aggregate_v<T>
json to_json(T const& obj) {
    json::object_type result;
    static constexpr auto members = define_static_array(
        std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
    template for (constexpr auto m : members) {
        result.emplace(std::string(std::meta::identifier_of(m)), to_json(obj.[:m:]));
    }
    return json(std::move(result));
}
```

**Verify**: `make test` → struct tests pass

**COMMIT**: `"add reflection-based to_json/from_json for aggregates"`

---

## Phase 8: Reflection Bridge — Enums (Tasks 8.1–8.3)

### Step 8.1–8.2: Enum serialization/deserialization

**RED**: `test/test_reflect_enum.cpp` — known enumerator, unknown name, unknown value, non-string input.

**GREEN**: Implement using `enumerators_of`:
```cpp
template <typename E> requires std::is_enum_v<E>
json to_json(E e) {
    static constexpr auto enums = define_static_array(std::meta::enumerators_of(^^E));
    template for (constexpr auto ev : enums) {
        if (e == [:ev:]) return json(std::string(std::meta::identifier_of(ev)));
    }
    return json(std::format("unknown({})", static_cast<std::underlying_type_t<E>>(e)));
}
```

**COMMIT**: `"add reflection-based to_json/from_json for enums"`

---

## Phase 9: Reflection Bridge — STL Types (Tasks 9.1–9.8)

### Step 9.1: optional

**RED/GREEN**: `to_json(optional)` → null or value. `from_json<optional>` → nullopt or parse.

### Step 9.2: tuple and pair

**RED/GREEN**: Serialize as JSON array. Deserialize with positional matching and size check.

### Step 9.3: variant

**RED/GREEN**: Serialize as `{"_type": "...", "_value": ...}`. Deserialize by matching type tag.

### Step 9.4: smart pointers

**RED/GREEN**: null pointer → null, non-null → serialize pointee.

### Step 9.5–9.6: Sequence and mapping containers

**RED/GREEN**: Concept-detected. Sequences → JSON array. Mappings → JSON object.

### Step 9.7: std::array

**RED/GREEN**: Fixed-size deserialization with size check.

### Step 9.8: Write tests

**File**: `test/test_reflect_stl.cpp` — one test per type per direction.

**COMMIT**: `"add to_json/from_json for optional, tuple, variant, containers, smart pointers"`

---

## Phase 10: Nested & Recursive (Tasks 10.1–10.4)

### Step 10.1–10.4: Integration tests

**RED**: `test/test_reflect_nested.cpp` — structs with vector members, maps of structs, optional nested structs, enum fields, variant fields.

**GREEN**: Should work already from phases 7–9 (recursive dispatch). Fix any edge cases.

**COMMIT**: `"add nested/recursive reflection bridge tests"`

---

## Phase 11: Compile-Time Parser (Tasks 11.1–11.6)

### Step 11.1: Consteval parser

Adapt the reference code (`references_code/reflection.cpp`) pattern:
- Consteval string parser → token tree
- Type inference → `data_member_spec`
- `define_aggregate` → anonymous struct
- Add bool and null support beyond the reference code

### Step 11.2: Array parsing

Extend parser to handle `[...]` → `std::tuple<Ts...>` via compile-time type list building.

### Step 11.3: json_literal proxy

```cpp
template <auto Data>
struct json_literal {
    static constexpr auto const& data = Data;
    consteval auto const& value() const { return data; }
    operator json() const { return to_json(data); }
    json get() const { return to_json(data); }
    friend json to_json(json_literal const& lit) { return to_json(lit.data); }
};
```

### Step 11.4: Unified _json UDL

```cpp
struct json_string {
    std::meta::info rep;
    consteval json_string(char const* s) : rep{parse_json_ct(s)} {}
};

template <json_string JS>
consteval auto operator""_json() {
    return json_literal<[:JS.rep:]>{};
}
```

### Step 11.5–11.6: Tests

**File**: `test/test_constexpr.cpp` — `static_assert` tests for compile-time access
**File**: `test/test_literal.cpp` — runtime conversion tests

**COMMIT**: `"add compile-time JSON parser and unified _json literal"`

---

## Phase 12: Integration & Cleanup (Tasks 12.1–12.8)

### Step 12.1: Remove old streaming API

Remove `Reader`, `Value`, `Type` enum, `read()`, `iter_array()`, `iter_object()`, `discard_until()`, `location()`, `is_number_cont()`, `is_string()` from the public interface.

### Step 12.2: Remove all throw statements

Search for `throw` — replace any remaining with `std::expected` returns or `std::unreachable()` for truly impossible paths.

### Step 12.3: Rewrite demos

Replace streaming-API demos with new API demos:
- `demo/basic.cc` — parse, access, serialize
- `demo/reflection.cc` — struct to/from json
- `demo/compile_time.cc` — consteval parsing
- `demo/containers.cc` — STL container round-trips

### Step 12.4–12.6: CI, README

Update `.github/workflows/ci.yml` for g++-16. Update `ReadMe.md` with new API.

### Step 12.7–12.8: Final validation

Run `make test`. Verify all specs from `specs/` are met.

**COMMIT**: `"remove streaming API, rewrite demos, update docs"`

---

## Commit Summary

| Phase | Commit message |
|---|---|
| 1 | `add test harness and build infrastructure` |
| 2 | `add sj::error type with path propagation` |
| 3 | `redesign sj::json core with reflection and std::expected` |
| 4 | `rewrite parser with std::expected, drop public streaming API` |
| 5 | `add concept system: JsonSerializable, JsonDeserializable, container detection` |
| 6 | `add to_json/from_json for scalars and json identity` |
| 7 | `add reflection-based to_json/from_json for aggregates` |
| 8 | `add reflection-based to_json/from_json for enums` |
| 9 | `add to_json/from_json for optional, tuple, variant, containers, smart pointers` |
| 10 | `add nested/recursive reflection bridge tests` |
| 11 | `add compile-time JSON parser and unified _json literal` |
| 12 | `remove streaming API, rewrite demos, update docs` |
