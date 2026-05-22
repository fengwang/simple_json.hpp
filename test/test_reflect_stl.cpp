#include "test_harness.hpp"
#include "../simple_json.hpp"
#include <set>
#include <array>

// -- optional --

TEST(to_json_optional_with_value) {
    std::optional<int> opt(42);
    sj::json j = sj::to_json(opt);
    EXPECT_TRUE(j.is_integer());
    EXPECT_EQ(j.as_integer(), 42);
}

TEST(to_json_optional_empty) {
    std::optional<int> opt;
    sj::json j = sj::to_json(opt);
    EXPECT_TRUE(j.is_null());
}

TEST(from_json_optional_value) {
    auto j = sj::parse("42");
    EXPECT_OK(j);
    auto r = sj::from_json<std::optional<int>>(*j);
    EXPECT_OK(r);
    EXPECT_TRUE(r->has_value());
    EXPECT_EQ(**r, 42);
}

TEST(from_json_optional_null) {
    auto j = sj::parse("null");
    EXPECT_OK(j);
    auto r = sj::from_json<std::optional<int>>(*j);
    EXPECT_OK(r);
    EXPECT_FALSE(r->has_value());
}

// -- tuple --

TEST(to_json_tuple) {
    auto t = std::tuple{1, std::string("hi"), true};
    sj::json j = sj::to_json(t);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), std::size_t{3});
    EXPECT_EQ(j[std::size_t{0}].as_integer(), 1);
    EXPECT_EQ(j[std::size_t{1}].as_string(), std::string("hi"));
    EXPECT_EQ(j[std::size_t{2}].as_boolean(), true);
}

TEST(from_json_tuple) {
    auto j = sj::parse(R"([1, "hi", true])");
    EXPECT_OK(j);
    auto r = sj::from_json<std::tuple<int, std::string, bool>>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(std::get<0>(*r), 1);
    EXPECT_EQ(std::get<1>(*r), std::string("hi"));
    EXPECT_EQ(std::get<2>(*r), true);
}

TEST(from_json_tuple_size_mismatch) {
    auto j = sj::parse("[1, 2]");
    EXPECT_OK(j);
    auto r = sj::from_json<std::tuple<int, int, int>>(*j);
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::conversion);
}

// -- pair --

TEST(to_json_pair) {
    auto p = std::pair{1, std::string("one")};
    sj::json j = sj::to_json(p);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), std::size_t{2});
}

TEST(from_json_pair) {
    auto j = sj::parse(R"([1, "one"])");
    EXPECT_OK(j);
    auto r = sj::from_json<std::pair<int, std::string>>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(r->first, 1);
    EXPECT_EQ(r->second, std::string("one"));
}

// -- variant --

TEST(to_json_variant_int) {
    std::variant<int, std::string> v = 42;
    sj::json j = sj::to_json(v);
    EXPECT_TRUE(j.is_object());
    EXPECT_TRUE(j.contains("_type"));
    EXPECT_TRUE(j.contains("_value"));
    EXPECT_EQ(j["_value"].as_integer(), 42);
}

TEST(to_json_variant_string) {
    std::variant<int, std::string> v = std::string("hello");
    sj::json j = sj::to_json(v);
    EXPECT_EQ(j["_value"].as_string(), std::string("hello"));
}

TEST(from_json_variant) {
    std::variant<int, std::string> original = 42;
    sj::json j = sj::to_json(original);
    auto r = sj::from_json<std::variant<int, std::string>>(j);
    EXPECT_OK(r);
    EXPECT_TRUE(std::holds_alternative<int>(*r));
    EXPECT_EQ(std::get<int>(*r), 42);
}

// -- smart pointers --

TEST(to_json_unique_ptr_value) {
    auto p = std::make_unique<int>(42);
    sj::json j = sj::to_json(p);
    EXPECT_TRUE(j.is_integer());
    EXPECT_EQ(j.as_integer(), 42);
}

TEST(to_json_unique_ptr_null) {
    std::unique_ptr<int> p;
    sj::json j = sj::to_json(p);
    EXPECT_TRUE(j.is_null());
}

TEST(from_json_unique_ptr) {
    auto j = sj::parse("42");
    EXPECT_OK(j);
    auto r = sj::from_json<std::unique_ptr<int>>(*j);
    EXPECT_OK(r);
    EXPECT_TRUE(*r != nullptr);
    EXPECT_EQ(**r, 42);
}

TEST(from_json_unique_ptr_null) {
    auto j = sj::parse("null");
    EXPECT_OK(j);
    auto r = sj::from_json<std::unique_ptr<int>>(*j);
    EXPECT_OK(r);
    EXPECT_TRUE(*r == nullptr);
}

TEST(to_json_shared_ptr) {
    auto p = std::make_shared<std::string>("hello");
    sj::json j = sj::to_json(p);
    EXPECT_EQ(j.as_string(), std::string("hello"));
}

// -- vector --

TEST(to_json_vector) {
    std::vector<int> v{1, 2, 3};
    sj::json j = sj::to_json(v);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), std::size_t{3});
}

TEST(from_json_vector) {
    auto j = sj::parse("[1, 2, 3]");
    EXPECT_OK(j);
    auto r = sj::from_json<std::vector<int>>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(r->size(), std::size_t{3});
    EXPECT_EQ((*r)[0], 1);
    EXPECT_EQ((*r)[2], 3);
}

// -- set --

TEST(to_json_set) {
    std::set<std::string> s{"a", "b", "c"};
    sj::json j = sj::to_json(s);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), std::size_t{3});
}

TEST(from_json_set) {
    auto j = sj::parse(R"(["a", "b", "a"])");
    EXPECT_OK(j);
    auto r = sj::from_json<std::set<std::string>>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(r->size(), std::size_t{2}); // deduplicated
}

// -- std::array --

TEST(to_json_stdarray) {
    std::array<int, 3> a{10, 20, 30};
    sj::json j = sj::to_json(a);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), std::size_t{3});
}

TEST(from_json_stdarray_ok) {
    auto j = sj::parse("[1, 2, 3]");
    EXPECT_OK(j);
    auto r = sj::from_json<std::array<int, 3>>(*j);
    EXPECT_OK(r);
    EXPECT_EQ((*r)[0], 1);
    EXPECT_EQ((*r)[2], 3);
}

TEST(from_json_stdarray_size_mismatch) {
    auto j = sj::parse("[1, 2]");
    EXPECT_OK(j);
    auto r = sj::from_json<std::array<int, 3>>(*j);
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::conversion);
}

// -- map --

TEST(to_json_map) {
    std::map<std::string, int> m{{"a", 1}, {"b", 2}};
    sj::json j = sj::to_json(m);
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["a"].as_integer(), 1);
}

TEST(from_json_map) {
    auto j = sj::parse(R"({"a": 1, "b": 2})");
    EXPECT_OK(j);
    auto r = sj::from_json<std::map<std::string, int>>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(r->size(), std::size_t{2});
    EXPECT_EQ((*r)["a"], 1);
}
