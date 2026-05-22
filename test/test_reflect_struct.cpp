#include "test_harness.hpp"
#include "../simple_json.hpp"

struct Point { int x; int y; };

struct Named { std::string name; int value; };

struct Inner { int a; };
struct Outer { std::string label; Inner child; };

struct WithOptional { int x; std::optional<int> y; };

struct WithVector { std::string name; std::vector<int> ids; };

struct WithMap { std::map<std::string, double> scores; };

// -- to_json --

TEST(to_json_flat_struct) {
    Point p{10, 20};
    sj::json j = sj::to_json(p);
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["x"].as_integer(), 10);
    EXPECT_EQ(j["y"].as_integer(), 20);
}

TEST(to_json_struct_with_string) {
    Named n{"hello", 42};
    sj::json j = sj::to_json(n);
    EXPECT_EQ(j["name"].as_string(), std::string("hello"));
    EXPECT_EQ(j["value"].as_integer(), 42);
}

TEST(to_json_nested_struct) {
    Outer o{"top", {42}};
    sj::json j = sj::to_json(o);
    EXPECT_EQ(j["label"].as_string(), std::string("top"));
    EXPECT_TRUE(j["child"].is_object());
    EXPECT_EQ(j["child"]["a"].as_integer(), 42);
}

TEST(to_json_struct_with_vector) {
    WithVector w{"test", {1, 2, 3}};
    sj::json j = sj::to_json(w);
    EXPECT_EQ(j["name"].as_string(), std::string("test"));
    EXPECT_TRUE(j["ids"].is_array());
    EXPECT_EQ(j["ids"].size(), std::size_t{3});
}

TEST(to_json_struct_with_map) {
    WithMap w{.scores = {{"math", 95.5}, {"eng", 88.0}}};
    sj::json j = sj::to_json(w);
    EXPECT_TRUE(j["scores"].is_object());
}

// -- from_json --

TEST(from_json_flat_struct) {
    auto j = sj::parse(R"({"x": 10, "y": 20})");
    EXPECT_OK(j);
    auto r = sj::from_json<Point>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(r->x, 10);
    EXPECT_EQ(r->y, 20);
}

TEST(from_json_nested_struct) {
    auto j = sj::parse(R"({"label": "top", "child": {"a": 42}})");
    EXPECT_OK(j);
    auto r = sj::from_json<Outer>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(r->label, std::string("top"));
    EXPECT_EQ(r->child.a, 42);
}

TEST(from_json_missing_field) {
    auto j = sj::parse(R"({"x": 10})");
    EXPECT_OK(j);
    auto r = sj::from_json<Point>(*j);
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::missing);
}

TEST(from_json_missing_optional_field) {
    auto j = sj::parse(R"({"x": 10})");
    EXPECT_OK(j);
    auto r = sj::from_json<WithOptional>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(r->x, 10);
    EXPECT_FALSE(r->y.has_value());
}

TEST(from_json_optional_field_present) {
    auto j = sj::parse(R"({"x": 10, "y": 20})");
    EXPECT_OK(j);
    auto r = sj::from_json<WithOptional>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(r->x, 10);
    EXPECT_TRUE(r->y.has_value());
    EXPECT_EQ(*r->y, 20);
}

TEST(from_json_type_mismatch) {
    auto j = sj::parse(R"({"x": "not_a_number", "y": 20})");
    EXPECT_OK(j);
    auto r = sj::from_json<Point>(*j);
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::type);
}

TEST(from_json_nested_error_path) {
    auto j = sj::parse(R"({"label": "ok", "child": {"a": "wrong"}})");
    EXPECT_OK(j);
    auto r = sj::from_json<Outer>(*j);
    EXPECT_ERR(r);
    EXPECT_EQ(r.error().path, std::string("child.a"));
}

TEST(from_json_extra_fields_ignored) {
    auto j = sj::parse(R"({"x": 1, "y": 2, "extra": "ignored"})");
    EXPECT_OK(j);
    auto r = sj::from_json<Point>(*j);
    EXPECT_OK(r);
    EXPECT_EQ(r->x, 1);
    EXPECT_EQ(r->y, 2);
}

// -- Round-trip --

TEST(roundtrip_struct) {
    Point original{10, 20};
    sj::json j = sj::to_json(original);
    auto r = sj::from_json<Point>(j);
    EXPECT_OK(r);
    EXPECT_EQ(r->x, original.x);
    EXPECT_EQ(r->y, original.y);
}

TEST(roundtrip_nested_struct) {
    Outer original{"hello", {99}};
    sj::json j = sj::to_json(original);
    auto r = sj::from_json<Outer>(j);
    EXPECT_OK(r);
    EXPECT_EQ(r->label, original.label);
    EXPECT_EQ(r->child.a, original.child.a);
}
