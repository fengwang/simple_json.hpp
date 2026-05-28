# simple_json.hpp

A tiny, header-only JSON library for modern C++26. It combines a runtime JSON value type (`sj::json`) with compile-time reflection to provide automatic struct↔JSON conversion, enum serialization, and consteval JSON parsing — all in a single header with zero dependencies beyond the standard library.

## Building

Requirements:
- GCC 16+ with `-std=c++26 -freflection`

Build tests and demos from the project root:

```bash
make          # builds tests + demos, runs tests
make test     # tests only
make demos    # demos only
```

This produces:
- `run_tests`         – the full test suite (149 tests)
- `demo_basic`        – parsing, access, serialization
- `demo_reflection`   – automatic struct↔JSON via reflection
- `demo_compile_time` – consteval JSON parsing with `static_assert`
- `demo_containers`   – STL container round-trips

## Features & Examples

### User this library via the `sj` namespace:

```cpp
#include "simple_json.hpp"
using namespace sj;
```

### The `sj::json` data type

`sj::json` represents any JSON value: null, boolean, integer (64-bit), floating-point (double), string, array, or object.

### Struct ↔ JSON via reflection

Any aggregate struct is automatically serializable and deserializable — no macros, no registration, no boilerplate:

```cpp
struct Point { int x; int y; };

Point p{10, 20};
sj::json j = sj::to_json(p);           // {"x": 10, "y": 20}

auto result = sj::from_json<Point>(j);  // expected<Point, error>
if (result)
    std::cout << result->x << "\n";     // 10
```

Nested structs, containers, optionals — all handled recursively:

```cpp
struct Address { std::string city; int zip; };
struct Person {
    std::string name;
    int age;
    Address address;
    std::vector<std::string> tags;
    std::optional<double> rating;
};

Person alice{"Alice", 30, {"NYC", 10001}, {"dev", "lead"}, 4.8};
sj::json j = sj::to_json(alice);
auto p = sj::from_json<Person>(j);  // round-trips perfectly
```

Missing required fields produce errors with a JSON-path trace:

```cpp
auto bad = sj::parse(R"({"name": "Bob", "address": {"city": "LA"}})");
auto r = sj::from_json<Person>(*bad);
// r.error().path == "address.zip"
// r.error().message == "missing field 'zip'"
```

Missing `std::optional` fields are silently set to `std::nullopt`.

### Enum serialization via reflection

Scoped enums are serialized by name, deserialized by name matching:

```cpp
enum class Color { Red, Green, Blue };

sj::json j = sj::to_json(Color::Green);   // "Green"

auto c = sj::from_json<Color>(j);          // Color::Green
```

### Parsing JSON

All parsing returns `std::expected<json, error>` — no exceptions.

From a string:

```cpp
auto result = sj::parse(R"({ "pi": 3.141, "happy": true })");
if (!result) {
    std::cerr << result.error().message << "\n";
    return;
}
sj::json data = *result;
```

From an input stream:

```cpp
std::ifstream f("example.json");
auto data = sj::parse(f);
```

### Constructing JSON in C++

Using initializer lists:

```cpp
sj::json j = {
    {"pi", 3.141},
    {"happy", true},
    {"list", sj::json::array_type{1, 0, 2}},
};
```

Using assignment via `operator[]`:

```cpp
sj::json j;          // null
j["pi"] = 3.141;     // j becomes an object
j["happy"] = true;
```

From STL containers (concept-detected — any iterable container works):

```cpp
std::vector<int> v{1, 2, 3};
sj::json j(v);                           // [1, 2, 3]

std::map<std::string, int> m{{"a", 1}};
sj::json j2(m);                          // {"a": 1}
```

### Accessing data

Type queries:

```cpp
j.is_null();     j.is_boolean();   j.is_integer();
j.is_number();   j.is_string();    j.is_array();
j.is_object();
```

Direct accessors (use after a type guard):

```cpp
double pi     = j["pi"].as_number();
bool happy    = j["happy"].as_boolean();
std::string s = j["name"].as_string();
int64_t n     = j["answer"].as_integer();
```

Safe access with `get<T>()` returning `std::expected`:

```cpp
auto name = j["name"].get<std::string>();  // expected<string, error>
if (name)
    std::cout << *name << "\n";
```

Safe lookup with `at()` returning `std::expected`:

```cpp
auto val = j.at("key");      // expected<json const&, error>
auto el  = j.at(std::size_t{0});   // expected<json const&, error>
```

Safe conversions with defaults:

```cpp
double pi     = j.value_or("pi", 0.0);
int answer    = j["answer"].value_or("everything", -1);
std::string s = j.value_or<std::string>("title", "untitled");
```

### Container-like operations

Array operations:

```cpp
sj::json j;              // null
j.push_back("foo");      // j becomes ["foo"]
j.push_back(1);
j.emplace_back(3.14);

j.size();     // 3
j.empty();    // false
j.clear();    // empties the array
```

Object operations:

```cpp
sj::json o;
o["foo"] = 23;
o.emplace("bar", "hello");

o.contains("foo");              // true
o.find("foo") != o.end();      // true
o.count("foo");                 // 1
o.erase("foo");
```

Iteration:

```cpp
// range-for over array or object values
for (auto const& el : j)
    std::cout << el << "\n";

// key/value iteration via items()
for (auto [key, value] : o.items())
    std::cout << key << ": " << value << "\n";

// iterator with key()/value()
for (auto it = o.begin(); it != o.end(); ++it)
    std::cout << it.key() << ": " << it.value() << "\n";
```

### Serialization

```cpp
std::string pretty  = j.dump(2);   // indented
std::string compact = j.dump(0);   // one line

std::cout << j << "\n";            // compact via operator<<
```

Both `dump()` and `operator<<` produce valid JSON that round-trips through `sj::parse`.

### STL type support

Containers are detected by capability (concepts), not by name — any container that satisfies the right interface works automatically, including `std::inplace_vector`, `std::hive`, `std::flat_map`, etc.

| C++ type | JSON representation |
|---|---|
| `std::vector<T>`, `std::deque<T>`, `std::list<T>`, `std::set<T>`, ... | JSON array |
| `std::map<string, T>`, `std::unordered_map<string, T>`, ... | JSON object |
| `std::array<T, N>` | JSON array (size-checked on deserialization) |
| `std::optional<T>` | `null` or the contained value |
| `std::tuple<Ts...>`, `std::pair<A, B>` | JSON array (positional) |
| `std::variant<Ts...>` | `{"_type": "...", "_value": ...}` (tagged) |
| `std::unique_ptr<T>`, `std::shared_ptr<T>` | `null` or the pointed-to value |

### Compile-time JSON parsing

JSON string literals can be parsed at compile time into statically-typed C++ aggregates:

```cpp
using namespace sj::literals;

constexpr auto cfg = R"({
    "host": "0.0.0.0",
    "port": 8080,
    "debug": false
})"_json;

static_assert(cfg.value().port == 8080);
static_assert(cfg.value().debug == false);

// Nested objects work too
constexpr auto nested = R"({
    "server": {"host": "localhost", "port": 443}
})"_json;
static_assert(nested.value().server.port == 443);
```

The same `_json` literal implicitly converts to `sj::json` at runtime:

```cpp
sj::json j = R"({"x": 1, "y": 2})"_json;  // runtime sj::json
```

### Custom serialization (ADL)

Override `to_json`/`from_json` for any type by providing overloads in your type's namespace:

```cpp
namespace mylib {
    struct Special { int data; };

    sj::json to_json(Special const& s) {
        return sj::json(s.data * 2);  // custom encoding
    }
}
```

User-provided overloads take priority over the reflection-based fallback.

### Error handling

All fallible operations return `std::expected`. No exceptions are thrown.

```cpp
auto r = sj::parse(input)
    .and_then([](sj::json const& j) { return sj::from_json<Config>(j); })
    .transform([](Config const& c) { return c.port; });

if (r) listen(*r);
else   log(r.error().message);
```

Errors carry structured diagnostics:

```cpp
struct sj::error {
    error_kind kind;     // parse, type, missing, conversion, overflow
    std::string message; // human-readable description
    std::string path;    // JSON-path breadcrumb, e.g. "server.host"
};
```

## Compatibility

| Requirement | Value |
|---|---|
| Standard | C++26 with P2996 reflection |
| Compiler | GCC 16+ with `-freflection` |
| Dependencies | None (standard library only) |
| Headers | Single header: `simple_json.hpp` |

This library targets the bleeding edge. No backward compatibility with C++23 or earlier is provided.
