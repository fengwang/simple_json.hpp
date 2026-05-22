# Task Checklist

## 1. Infrastructure Setup

- [ ] 1.1 Update Makefile for C++26 with `-std=c++26 -freflection` flags and `test` target
- [ ] 1.2 Create `test/test_harness.hpp` with TEST, EXPECT_EQ, EXPECT_TRUE, EXPECT_FALSE, EXPECT_OK, EXPECT_ERR, EXPECT_ERR_KIND macros and `run_all()` runner
- [ ] 1.3 Create `test/test_main.cpp` entry point
- [ ] 1.4 Verify harness compiles and runs with a trivial test

## 2. Error Type Foundation

- [ ] 2.1 Define `sj::error_kind` enum and `sj::error` struct with `with_context()`
- [ ] 2.2 Write tests for error type construction and path propagation (`test/test_errors.cpp`)

## 3. Core `sj::json` Redesign

- [ ] 3.1 Rewrite `sj::json` variant definition with type aliases (preserve existing aliases)
- [ ] 3.2 Implement `is<T>()` generic type query and named wrappers (`is_null()`, `is_boolean()`, etc.)
- [ ] 3.3 Implement `as<T>()` returning `std::expected<T const&, error>`
- [ ] 3.4 Implement `get<T>()` returning `std::expected<T, error>` with numeric conversions
- [ ] 3.5 Implement `value_t` enum derived from variant and `type()` method
- [ ] 3.6 Rewrite constructors with concept constraints (replace SFINAE `enable_if_t`)
- [ ] 3.7 Preserve `operator[]` auto-vivify, rewrite `at()` to return `std::expected`
- [ ] 3.8 Preserve container operations: `push_back`, `emplace_back`, `emplace`, `contains`, `size`, `empty`, `clear`, `erase`, `find`, `count`
- [ ] 3.9 Preserve iterator support: `begin`/`end`/`cbegin`/`cend`, `items()` view, structured bindings
- [ ] 3.10 Preserve `value_or()` with current semantics
- [ ] 3.11 Rewrite `dump()` using `std::format` instead of `std::ostringstream`
- [ ] 3.12 Preserve `operator<<` and `operator==`
- [ ] 3.13 Write comprehensive tests for json core (`test/test_json_core.cpp`)
- [ ] 3.14 Write tests for container operations (`test/test_container_ops.cpp`)
- [ ] 3.15 Write tests for serialization round-trips (`test/test_serialize.cpp`)

## 4. Runtime Parser Rewrite

- [ ] 4.1 Rewrite internal parser (drop `Reader`/`Value`/streaming API from public interface)
- [ ] 4.2 Implement `sj::parse(string_view)` returning `std::expected<json, error>`
- [ ] 4.3 Implement `sj::parse(istream&)` returning `std::expected<json, error>`
- [ ] 4.4 Write parse tests: valid JSON, invalid JSON, edge cases (`test/test_parse.cpp`)

## 5. Concept System

- [ ] 5.1 Define `JsonSerializable` concept
- [ ] 5.2 Define `JsonDeserializable` concept
- [ ] 5.3 Define `JsonConvertible` concept
- [ ] 5.4 Define container detection concepts: `SequenceSerializable`, `MappingSerializable`, `BackInsertable`, `Insertable`, `FixedSizeSequence`
- [ ] 5.5 Write concept verification tests with `static_assert` (`test/test_concepts.cpp`)

## 6. Reflection Bridge — Scalars & Identity

- [ ] 6.1 Implement `to_json` / `from_json` for `sj::json` (identity)
- [ ] 6.2 Implement `to_json` / `from_json` for bool, integers, floating-point, string, nullptr

## 7. Reflection Bridge — Aggregates

- [ ] 7.1 Implement `to_json` for aggregates via `nonstatic_data_members_of`
- [ ] 7.2 Implement `from_json` for aggregates with field-name matching and error path propagation
- [ ] 7.3 Write tests: flat structs, nested structs, missing fields, type mismatches, extra fields (`test/test_reflect_struct.cpp`)

## 8. Reflection Bridge — Enums

- [ ] 8.1 Implement `to_json` for enums via `enumerators_of`
- [ ] 8.2 Implement `from_json` for enums with name matching
- [ ] 8.3 Write tests: known enumerators, unknown names, unknown values (`test/test_reflect_enum.cpp`)

## 9. Reflection Bridge — STL Containers

- [ ] 9.1 Implement `to_json` / `from_json` for `std::optional`
- [ ] 9.2 Implement `to_json` / `from_json` for `std::tuple` and `std::pair`
- [ ] 9.3 Implement `to_json` / `from_json` for `std::variant` (tagged format)
- [ ] 9.4 Implement `to_json` / `from_json` for smart pointers (`unique_ptr`, `shared_ptr`)
- [ ] 9.5 Implement `to_json` / `from_json` for sequence containers (concept-detected)
- [ ] 9.6 Implement `to_json` / `from_json` for mapping containers (concept-detected)
- [ ] 9.7 Implement `from_json` for `std::array` (FixedSizeSequence) with size check
- [ ] 9.8 Write tests for all STL type conversions (`test/test_reflect_stl.cpp`)

## 10. Reflection Bridge — Nested & Recursive

- [ ] 10.1 Implement and test deeply nested structs with container members
- [ ] 10.2 Implement and test containers of aggregates (e.g., `vector<MyStruct>`)
- [ ] 10.3 Implement and test structs with optional, enum, and variant members
- [ ] 10.4 Write comprehensive nested tests (`test/test_reflect_nested.cpp`)

## 11. Compile-Time Parser

- [ ] 11.1 Implement consteval JSON tokenizer/parser (object, scalars, nested objects)
- [ ] 11.2 Add compile-time array parsing producing `std::tuple`
- [ ] 11.3 Implement `json_literal<R>` proxy with `.value()`, `operator json()`, `.get()`, and `to_json` friend
- [ ] 11.4 Implement unified `_json` template UDL
- [ ] 11.5 Write compile-time tests with `static_assert` (`test/test_constexpr.cpp`)
- [ ] 11.6 Write runtime conversion tests (`test/test_literal.cpp`)

## 12. Integration & Cleanup

- [ ] 12.1 Remove old streaming API types and functions from public interface
- [ ] 12.2 Remove all `throw` statements
- [ ] 12.3 Rewrite demo files to use new API
- [ ] 12.4 Update `_json` runtime literal (if kept separately from template UDL)
- [ ] 12.5 Update CI workflow for g++-16 and C++26 flags
- [ ] 12.6 Update README.md
- [ ] 12.7 Run full test suite, fix any failures
- [ ] 12.8 Final review: ensure all specs are met
