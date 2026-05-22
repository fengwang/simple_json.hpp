#include "test_harness.hpp"
#include "../simple_json.hpp"

using namespace sj::literals;

// -- Compile-time object access --

constexpr auto simple = R"({"port": 8080, "host": "localhost"})"_json;
static_assert(simple.value().port == 8080);
static_assert(std::string_view(simple.value().host) == "localhost");

// -- Nested compile-time objects --

constexpr auto nested = R"({"server": {"host": "0.0.0.0", "port": 443}, "debug": false})"_json;
static_assert(nested.value().server.port == 443);
static_assert(std::string_view(nested.value().server.host) == "0.0.0.0");
static_assert(nested.value().debug == false);

// -- Boolean and null --

constexpr auto bools = R"({"yes": true, "no": false})"_json;
static_assert(bools.value().yes == true);
static_assert(bools.value().no == false);

// -- Negative numbers --

constexpr auto neg = R"({"x": -5})"_json;
static_assert(neg.value().x == -5);

// -- Floating point --

constexpr auto fp = R"({"pi": 3.14})"_json;
static_assert(fp.value().pi > 3.13 && fp.value().pi < 3.15);

// NOTE: Compile-time arrays as std::tuple are not supported due to a
// limitation in define_aggregate — std::tuple is not an aggregate and
// cannot be used as a member in compile-time generated structs.
// Arrays in JSON literals are converted correctly when the literal is
// converted to sj::json at runtime.

// Runtime tests for the proxy conversion

TEST(literal_to_json_conversion) {
    sj::json j = R"({"name": "Alice", "age": 30})"_json;
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["name"].as_string(), std::string("Alice"));
    EXPECT_EQ(j["age"].as_integer(), 30);
}

TEST(literal_get_explicit) {
    auto j = R"({"x": 1})"_json.get();
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["x"].as_integer(), 1);
}

TEST(literal_nested_to_json) {
    sj::json j = R"({"inner": {"val": 99}})"_json;
    EXPECT_TRUE(j["inner"].is_object());
    EXPECT_EQ(j["inner"]["val"].as_integer(), 99);
}

TEST(literal_bool_to_json) {
    sj::json j = R"({"flag": true})"_json;
    EXPECT_EQ(j["flag"].as_boolean(), true);
}

TEST(literal_float_to_json) {
    sj::json j = R"({"val": 3.14})"_json;
    EXPECT_TRUE(j["val"].is_number());
}
