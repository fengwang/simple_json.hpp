#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>

#include "../simple_json.hpp"

int main() {
    using sj::json;
    using namespace sj::literals;

    // Complex JSON text used for parsing tests.
    const char* text = R"(
{
  "pi": 3.141,
  "happy": true,
  "name": "Niels",
  "nothing": null,
  "answer": {
    "everything": 42
  },
  "list": [1, 0, 2],
  "object": {
    "currency": "USD",
    "value": 42.99
  }
}
)";

    // 1. Parse from a std::istream.
    std::istringstream iss{text};
    json from_stream = sj::parse(iss);

    // 2. Parse from a raw string literal.
    json from_string = sj::parse(text);

    // 3. Parse using the _json user-defined literal.
    json from_literal = R"(
{
  "pi": 3.141,
  "happy": true,
  "name": "Niels",
  "nothing": null,
  "answer": {
    "everything": 42
  },
  "list": [1, 0, 2],
  "object": {
    "currency": "USD",
    "value": 42.99
  }
}
)"_json;

    // 4. Build the same structure using initializer lists and assignments.
    json expected = {
        {"pi", 3.141},
        {"happy", true},
        {"name", "Niels"},
        {"nothing", nullptr},
        {"answer", {{"everything", 42}}},
        {"list", json::array_type{1, 0, 2}},
        {"object", {{"currency", "USD"}, {"value", 42.99}}}
    };

    // Verify that all constructed JSON values are equal.
    assert(from_stream == expected);
    assert(from_string == expected);
    assert(from_literal == expected);

    // Spot-check some fields via the object/array accessors.
    assert(from_stream.is_object());
    assert(from_stream["happy"].is_boolean());
    assert(from_stream["happy"].as_boolean() == true);
    assert(from_stream["pi"].is_number());
    assert(!from_stream["pi"].is_integer());
    assert(from_stream["pi"].as_number() == 3.141);
    assert(from_stream["answer"]["everything"].is_integer());
    assert(from_stream["answer"]["everything"].as_integer() == 42);
    assert(from_stream["answer"]["everything"].as_number() == 42.0);
    assert(from_stream["list"].is_array());
    assert(from_stream["list"].as_array().size() == 3);
    assert(from_stream["list"].as_array()[0].is_integer());
    assert(from_stream["list"].as_array()[0].as_integer() == 1);
    assert(from_stream["object"]["currency"].as_string() == "USD");

    // contains()
    assert(from_stream.contains("pi"));
    assert(!from_stream.contains("missing"));

    // value_or() for different types.
    double pi = from_stream.value_or("pi", 0.0);
    assert(pi == 3.141);
    int everything = from_stream["answer"].value_or("everything", -1);
    assert(everything == 42);
    std::string name = from_stream.value_or<std::string>("name", "none");
    assert(name == "Niels");
    bool missing_flag = from_stream.value_or("missing_flag", false);
    assert(missing_flag == false);

    // pretty-printing round trip.
    std::string pretty = expected.dump(2);
    json round_trip = sj::parse(pretty);
    assert(round_trip == expected);

    std::cout << "json_test: all checks passed\n";
    return 0;
}
