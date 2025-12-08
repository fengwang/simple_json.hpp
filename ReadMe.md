# simple_json.hpp

A tiny, header-only JSON helper library with a streaming tokenizer and a modern C++23 value type `sj::json`. It started as a C project and now provides a small, easy-to-embed interface suitable for demos, tools, and tests.

## Building the demos

Requirements:
- C++23-compatible compiler (e.g. `g++` 13+)

Build all demos from the project root:

```bash
make
```

This produces:
- `array`        – simple array iteration demo  
- `object`       – object iteration and skipping fields  
- `printer`      – pretty-print / minify a JSON file  
- `rect`         – load a small struct from JSON  
- `json_test`    – exercises the `sj::json` API  
- `json_container` – demonstrates `sj::json`’s container-like operations

## The `sj::json` data type

The core high-level API is the `sj::json` value type defined in `simple_json.hpp`. It represents any JSON value:

- `null`
- boolean (`true` / `false`)
- number (stored as 64-bit integer or double)
- string
- array of `sj::json`
- object mapping `std::string` keys to `sj::json`

### Parsing JSON

From an input stream:

```cpp
#include <fstream>
#include "simple_json.hpp"

std::ifstream f("example.json");
sj::json data = sj::parse(f);
```

From a string:

```cpp
std::string text = R"({ "pi": 3.141, "happy": true })";
sj::json data = sj::parse(text);
```

Using the user-defined literal:

```cpp
using namespace sj::literals;

sj::json data = R"(
{
  "pi": 3.141,
  "happy": true
}
)"_json;
```

### Constructing JSON in C++

Using initializer lists:

```cpp
sj::json j = {
  {"pi", 3.141},
  {"happy", true},
};
```

Using assignment via `operator[]`:

```cpp
sj::json j;          // null
j["pi"] = 3.141;     // j becomes an object
j["happy"] = true;
```

Arrays use `sj::json::array_type`:

```cpp
sj::json::array_type arr = {1, 0, 2};
sj::json j = {
  {"list", arr}
};
```

### Accessing data

Basic access:

```cpp
double pi = j["pi"].as_number();      // or to_double()
bool happy = j["happy"].as_boolean();
std::string name = j["name"].as_string();
```

Integer vs floating-point:

```cpp
if (j["answer"]["everything"].is_integer()) {
    auto n = j["answer"]["everything"].as_integer(); // 42
}
double n_as_double = j["answer"]["everything"].as_number(); // 42.0
```

Safe conversions with defaults:

```cpp
double pi = j.value_or("pi", 0.0);
int answer = j["answer"].value_or("everything", -1);
std::string title = j.value_or<std::string>("title", "untitled");
bool flag = j.value_or("flag", false);
```

Containment checks:

```cpp
if (j.contains("pi")) {
    // key exists in object
}

if (j["list"].contains(0)) {
    // index is valid in array
}
```

### Container-like operations

`sj::json` behaves similarly to a standard container for arrays and objects.

Array operations:

```cpp
sj::json j;              // null
j.push_back("foo");      // j becomes an array
j.push_back(1);
j.push_back(true);
j.emplace_back(1.78);

for (auto it = j.begin(); it != j.end(); ++it) {
    std::cout << *it << '\n';
}

for (auto& element : j) {
    std::cout << element << '\n';
}

auto s = j[0].get<sj::json::string_type>(); // "foo"
j[1] = 42;
bool flag = j.at(2);                        // implicit get<bool>()

std::size_t n = j.size();   // 4
bool e = j.empty();         // false
sj::value_t t = j.type();   // sj::value_t::array
j.clear();                  // becomes empty array (or null for scalars)
```

Object operations:

```cpp
sj::json o;
o["foo"] = 23;
o["bar"] = false;
o["baz"] = 3.141;
o.emplace("weather", "sunny");

// iterate with key() / value()
for (auto it = o.begin(); it != o.end(); ++it) {
    std::cout << it.key() << " : " << it.value() << "\n";
}

// items() view
for (auto item : o.items()) {
    std::cout << item.key() << " : " << item.value() << "\n";
}

// structured bindings
for (auto [key, value] : o.items()) {
    std::cout << key << " : " << value << "\n";
}

if (o.contains("foo")) { /* key exists */ }
if (o.find("foo") != o.end()) { /* found via iterator */ }

auto count_foo = o.count("foo"); // 1
o.erase("foo");                  // remove entry if present
```

### Pretty-printing and streaming

To get a JSON string:

```cpp
std::string pretty = j.dump(2);  // indented
std::string compact = j.dump(0); // one line
```

`sj::json` also supports streaming:

```cpp
#include <iostream>

std::cout << j << "\n"; // compact JSON
```

Both `dump()` and `operator<<` produce valid JSON that can be parsed again with `sj::parse`.
