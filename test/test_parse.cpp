#include "test_harness.hpp"
#include "../simple_json.hpp"

TEST(parse_object) {
    auto r = sj::parse(R"({"x": 1, "y": 2})");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_object());
    EXPECT_EQ((*r)["x"].as_integer(), 1);
    EXPECT_EQ((*r)["y"].as_integer(), 2);
}

TEST(parse_array) {
    auto r = sj::parse("[1, 2, 3]");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_array());
    EXPECT_EQ(r->size(), std::size_t{3});
}

TEST(parse_string) {
    auto r = sj::parse(R"("hello")");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_string());
    EXPECT_EQ(r->as_string(), std::string("hello"));
}

TEST(parse_integer) {
    auto r = sj::parse("42");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer(), 42);
}

TEST(parse_negative_integer) {
    auto r = sj::parse("-7");
    EXPECT_OK(r);
    EXPECT_EQ(r->as_integer(), -7);
}

TEST(parse_float) {
    auto r = sj::parse("3.14");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_number());
    EXPECT_FALSE(r->is_integer());
}

TEST(parse_scientific) {
    auto r = sj::parse("1e5");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_number());
}

TEST(parse_true) {
    auto r = sj::parse("true");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_boolean());
    EXPECT_EQ(r->as_boolean(), true);
}

TEST(parse_false) {
    auto r = sj::parse("false");
    EXPECT_OK(r);
    EXPECT_EQ(r->as_boolean(), false);
}

TEST(parse_null) {
    auto r = sj::parse("null");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_null());
}

TEST(parse_nested) {
    auto r = sj::parse(R"({"a": {"b": [1, 2]}})");
    EXPECT_OK(r);
    EXPECT_TRUE((*r)["a"].is_object());
    EXPECT_TRUE((*r)["a"]["b"].is_array());
}

TEST(parse_escape_sequences) {
    auto r = sj::parse(R"("hello\nworld")");
    EXPECT_OK(r);
    EXPECT_EQ(r->as_string(), std::string("hello\nworld"));
}

TEST(parse_unicode_escape) {
    auto r = sj::parse(R"("\u0041")");
    EXPECT_OK(r);
    EXPECT_EQ(r->as_string(), std::string("A"));
}

TEST(parse_empty_object) {
    auto r = sj::parse("{}");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_object());
    EXPECT_TRUE(r->empty());
}

TEST(parse_empty_array) {
    auto r = sj::parse("[]");
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_array());
    EXPECT_TRUE(r->empty());
}

// -- Error cases --

TEST(parse_empty_string_error) {
    auto r = sj::parse("");
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::parse);
}

TEST(parse_unclosed_string) {
    auto r = sj::parse(R"("unclosed)");
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::parse);
}

TEST(parse_unclosed_array) {
    auto r = sj::parse("[1, 2");
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::parse);
}

TEST(parse_unclosed_object) {
    auto r = sj::parse(R"({"x": 1)");
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::parse);
}

TEST(parse_trailing_content) {
    auto r = sj::parse("42 extra");
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::parse);
}

TEST(parse_invalid_token) {
    auto r = sj::parse("undefined");
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::parse);
}

// -- Round-trip --

TEST(parse_dump_roundtrip) {
    auto r = sj::parse(R"({"pi": 3.141, "happy": true, "name": "Niels", "nothing": null, "list": [1, 0, 2]})");
    EXPECT_OK(r);
    std::string dumped = r->dump(2);
    auto r2 = sj::parse(dumped);
    EXPECT_OK(r2);
    EXPECT_TRUE(*r == *r2);
}

TEST(parse_stream) {
    std::istringstream iss(R"({"x": 1})");
    auto r = sj::parse(iss);
    EXPECT_OK(r);
    EXPECT_TRUE(r->is_object());
}

// -- Literal (now goes through compile-time proxy with implicit conversion) --

TEST(json_literal) {
    using namespace sj::literals;
    sj::json j = R"({"a": 1, "b": true})"_json;
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["a"].as_integer(), 1);
    EXPECT_EQ(j["b"].as_boolean(), true);
}

// -- Serialize --

TEST(dump_compact) {
    sj::json j = {{"x", 1}};
    std::string s = j.dump(0);
    EXPECT_TRUE(s.find('\n') == std::string::npos);
}

TEST(stream_operator_roundtrip) {
    sj::json j = {{"a", 1}, {"b", "two"}};
    std::ostringstream oss;
    oss << j;
    auto r = sj::parse(oss.str());
    EXPECT_OK(r);
    EXPECT_TRUE(*r == j);
}
