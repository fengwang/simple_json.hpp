# Proposal: Refactor simple_json with C++26 Reflection

## Motivation

The `simple_json` library is a header-only JSON library with two layers: a low-level streaming tokenizer and a high-level `sj::json` variant-based DOM. While functional, it suffers from:

1. **Repetitive boilerplate** — The `sj::json` class has hand-written `is_X()`, `as_X()`, `get<T>()` chains that repeat the same pattern for each variant alternative. Adding a new type means touching many methods.

2. **No struct↔JSON bridging** — Users must manually map between their application types and `sj::json`. There is no automatic serialization or deserialization.

3. **No compile-time JSON** — JSON is always parsed at runtime, even for string literals known at compile time.

4. **Legacy C patterns** — The streaming API (`Reader`, `Value`, raw pointers) carries C-era idioms. SFINAE-based template constraints are verbose compared to C++20 concepts.

5. **Mixed error handling** — The streaming layer uses `std::expected` while `sj::json` throws exceptions.

C++26 reflection (`<meta>`, P2996) enables a fundamentally different approach: the variant type list becomes the single source of truth, struct fields are introspected automatically, and JSON string literals can be parsed into typed aggregates at compile time.

## Specific Changes

1. **Deep redesign of `sj::json`** — Reflection-generated API from variant type list. `std::expected` replaces exceptions. Concepts replace SFINAE. `std::format` replaces `std::ostringstream`.

2. **Concept hierarchy** — `JsonSerializable`, `JsonDeserializable`, `JsonConvertible` concepts that unify `sj::json`, user structs, enums, STL containers, and compile-time aggregates.

3. **Reflection-based `to_json`/`from_json`** — Automatic serialization/deserialization for aggregates (via `nonstatic_data_members_of`), enums (via `enumerators_of`), and recursive types (nested structs, containers, optional, variant, tuple, smart pointers).

4. **Compile-time JSON parser** — A consteval parser that produces statically-typed aggregates from JSON string literals via `define_aggregate`.

5. **Unified `_json` literal** — A single literal that returns a proxy type (`json_literal<R>`) that can be used as a compile-time aggregate or implicitly converted to `sj::json`.

6. **Drop streaming API** — `Reader`, `Value`, `read()`, `iter_array()`, `iter_object()`, `discard_until()`, `location()` are removed from the public API. The parser is rewritten internally.

7. **Custom test harness** — A lightweight `test_harness.hpp` with `TEST()`, `EXPECT_EQ`, `EXPECT_OK`, `EXPECT_ERR` macros. `static_assert` for compile-time features.

## Capabilities

### New Capabilities

| Capability | Description |
|---|---|
| `concept-system` | JsonSerializable/JsonDeserializable/JsonConvertible concepts; container-detection concepts (SequenceSerializable, MappingSerializable, BackInsertable, Insertable, FixedSizeSequence) |
| `reflection-bridge` | `to_json`/`from_json` free functions for aggregates, enums, STL containers, optional, tuple, variant, smart pointers |
| `compile-time-parser` | Consteval JSON parsing into typed aggregates; `json_literal<R>` proxy; unified `_json` template UDL |
| `test-harness` | Custom lightweight test framework with assertion macros and `static_assert`-based compile-time tests |

### Modified Capabilities

| Capability | What Changes |
|---|---|
| `json-core` | `sj::json` internals redesigned: reflection-generated `is<T>`/`as<T>`/`type()`; concepts replacing SFINAE for constructors; `std::format` for serialization; named methods preserved as thin wrappers |
| `error-handling` | Exceptions removed entirely. `parse()`, `get<T>()`, `as<T>()`, `at()`, `from_json<T>()` return `std::expected`. New `sj::error` type with `error_kind`, `message`, and `path` for nested diagnostics |

## Impact

- **API** — Tier 1 methods preserved in name and semantics (except return types changing to `std::expected`). Tier 2 methods may change signatures. Tier 3 (streaming API) removed entirely.
- **Compiler** — Requires g++ 16+ with `-std=c++26 -freflection`. No backward compatibility with earlier standards.
- **Build** — Makefile updated for C++26 flags. CI updated for g++-16.
- **Demos** — All demo files rewritten to use the new API. Old streaming-API demos replaced with reflection feature demos.
- **Dependencies** — None. Standard library only (`<meta>`, `<expected>`, `<format>`, `<concepts>`, `<ranges>`).
