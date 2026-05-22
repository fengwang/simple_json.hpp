# Design: simple_json C++26 Reflection Refactoring

## Context

`simple_json` is a ~1300-line header-only JSON library targeting C++23. It provides a streaming tokenizer (Reader/Value) and a variant-based DOM type (`sj::json`). The library runs on g++ 16.1.1 with Arch Linux.

C++26 reflection (P2996, `<meta>`) is now available via `-freflection`. Key facilities confirmed working on our compiler:
- `^^T` (reflect), `[:m:]` (splice)
- `nonstatic_data_members_of`, `enumerators_of`, `type_of`, `identifier_of`
- `define_static_array`, `define_aggregate`, `data_member_spec`
- `reflect_constant`, `reflect_constant_string`
- `template for` (expansion statements)

**Stakeholders**: Library author (solo project).

**Constraints**: Single header. No third-party dependencies. g++ 16.1.1 only. No backward compatibility required.

## Goals

1. Redesign `sj::json` internals so the variant type list is the single source of truth — reflection generates the repetitive API surface.
2. Add reflection-powered `to_json`/`from_json` supporting aggregates, enums, and STL containers recursively.
3. Add compile-time JSON parsing that produces statically-typed aggregates.
4. Unify compile-time and runtime paths through a concept hierarchy (`JsonSerializable`, `JsonDeserializable`, `JsonConvertible`).
5. Replace all exceptions with `std::expected`.
6. Provide a custom test harness with full TDD-style test coverage.

## Non-Goals

- Backward compatibility with C++23 or earlier compilers.
- SAX/streaming parser as a public API.
- Performance benchmarking (correctness first).
- JSON Schema validation.
- JSON Pointer / JSON Patch.

## Decisions

### D1: Concept-Unified Architecture (over simpler alternatives)

**Choice**: Define a concept lattice (`JsonSerializable` / `JsonDeserializable` / `JsonConvertible`) that `sj::json`, user structs, enums, STL containers, and compile-time aggregates all satisfy.

**Alternatives considered**:
- *Modernized Variant + Reflection Add-ons* — Simpler but misses the "deep redesign" goal. Reflection is bolted on rather than foundational.
- *Reflection-Generated API only (no concepts)* — DRY internals but no unifying abstraction across type categories.

**Rationale**: Concepts are the C++26-native way to express "this type can be converted to/from JSON." They make the library extensible (new types automatically participate) and enable generic algorithms over any JSON-like type.

### D2: `std::expected` Everywhere (over exceptions or dual API)

**Choice**: All fallible operations return `std::expected<T, sj::error>`. No exceptions thrown by library code.

**Alternatives considered**:
- *Exceptions only* — Simpler call sites but prevents use in `noexcept` contexts and makes error propagation implicit.
- *Dual API* (`parse` + `try_parse`) — More flexible but doubles the API surface.

**Rationale**: `std::expected` is the modern C++ error handling primitive. It composes with monadic operations (`.and_then()`, `.transform()`), makes error paths explicit, and the `sj::error` type carries structured diagnostics (kind, message, path).

### D3: Concept-Based Container Detection (over enumeration)

**Choice**: Detect container capabilities via structural concepts (`SequenceSerializable`, `MappingSerializable`, `BackInsertable`, `Insertable`, `FixedSizeSequence`) rather than listing specific STL containers.

**Rationale**: Automatically supports all current and future STL containers (`inplace_vector`, `hive`, `flat_map`, etc.) and user-defined containers. Zero maintenance cost for new container types.

### D4: Unified `_json` Literal via Proxy (over separate syntaxes)

**Choice**: The `_json` literal uses a template UDL (NTTP) that returns `json_literal<R>`, a proxy type that:
- Provides `.value()` for compile-time aggregate access
- Implicitly converts to `sj::json` for runtime use
- Satisfies `JsonSerializable`

**Alternatives considered**:
- *Separate `_json` and `_cjson` literals* — Clearer but less seamless.
- *`sj::json_object<"...">`* variable template — Explicit but verbose.

**Rationale**: A single literal that "just works" in both contexts is the best UX. The proxy type handles the dispatch transparently.

### D5: `to_json` Is Infallible, `from_json` Returns Expected (asymmetry by design)

**Choice**: `to_json(T) -> json` (always succeeds), `from_json<T>(json) -> expected<T, error>` (may fail).

**Rationale**: If a type satisfies `JsonSerializable`, its conversion to JSON cannot fail at runtime — the concept constraint proves it at compile time. Deserialization can always fail because the JSON input may not match the expected structure.

### D6: ADL-Based Customization Point (over traits/specialization)

**Choice**: `to_json` and `from_json` are found via ADL. User types get auto-generated overloads via reflection fallback. Users can override by providing their own overloads in their type's namespace.

**Dispatch priority**:
1. User-provided ADL overload
2. `sj::json` identity
3. Scalar built-ins
4. `std::optional`
5. `std::variant` (tagged: `{"_type": ..., "_value": ...}`)
6. `std::tuple` / `std::pair` (positional JSON array)
7. Smart pointers (`unique_ptr`, `shared_ptr`)
8. Mapping containers (concept-detected)
9. Sequence containers (concept-detected)
10. Enum types (via `enumerators_of`)
11. Aggregate types (via reflection, lowest priority fallback)

### D7: Compile-Time Arrays as `std::tuple` (over `std::array`)

**Choice**: JSON arrays in compile-time parsing produce `std::tuple<Ts...>` where each element type is independently inferred.

**Alternatives considered**:
- `std::array<T, N>` for homogeneous arrays — Simpler but can't handle `[1, "hi", true]`.
- Skip arrays in compile-time path — Too limiting.

**Rationale**: `std::tuple` handles both homogeneous and heterogeneous arrays. Compile-time JSON's primary use case is configuration objects where mixed-type arrays are common.

### D8: Drop Streaming API Entirely (over modernize or internalize)

**Choice**: `Reader`, `Value`, `read()`, `iter_array()`, `iter_object()`, `discard_until()`, `location()` are removed from the public API. The parser is rewritten internally.

**Rationale**: The streaming API carries C-era idioms (raw pointers, manual state management) and is only used by demos. The `sj::json` DOM and reflection features cover all use cases. Demos are rewritten.

## Risks / Trade-offs

| Risk | Impact | Mitigation |
|---|---|---|
| g++ 16 reflection bugs or incomplete features | Build failures on edge cases | Validate each reflection pattern with isolated test before integrating. Keep a `references_code/` folder of known-working patterns. |
| `template for` + `define_static_array` heap allocation errors | Compile errors for certain reflection patterns | Use the `define_static_array(...)` pattern consistently (confirmed working). |
| Unified `_json` proxy may confuse overload resolution | Unexpected implicit conversions | Explicit conversion operators where needed. Thorough testing of both paths. |
| `std::expected` everywhere may be ergonomically worse for simple use cases | More verbose call sites | Provide `value_or()` helpers and document monadic chaining. |
| Compile-time parser limited by consteval restrictions | Can't handle all JSON features (e.g., escape sequences, unicode) | Document limitations. Compile-time path is for simple/config JSON. |
| No backward compatibility | Existing users must rewrite | Accepted non-goal. This is a clean-break redesign. |

## Migration Plan

This is a greenfield redesign on the `fea/reflection` branch. No incremental migration needed.

1. Implement and test the new library on `fea/reflection` branch.
2. Rewrite all demos to use the new API.
3. Update README and CI.
4. Merge to master as a breaking release.

Rollback: the `master` branch preserves the original library.

## Open Questions

| Question | Status | Notes |
|---|---|---|
| `std::variant` serialization format | Decided: tagged `{"_type": "...", "_value": ...}` | Using reflection to get type names. May revisit if ambiguous. |
| Compile-time string representation | Tentative: `char const*` | May need `std::string_view` or a constexpr string wrapper for longer strings. |
| Compile-time parser escape sequence support | Open | The reference code doesn't handle `\"`, `\\`, etc. May limit compile-time path to simple strings. |
| `json_literal` member access ergonomics | Open | Without `operator.` forwarding, users must call `.value().field`. Acceptable for now. |
