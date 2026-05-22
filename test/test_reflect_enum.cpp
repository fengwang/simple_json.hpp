#include "test_harness.hpp"
#include "../simple_json.hpp"

enum class Color { Red, Green, Blue };
enum class Priority { Low = 0, Medium = 1, High = 2 };

TEST(to_json_enum) {
    sj::json j = sj::to_json(Color::Green);
    EXPECT_TRUE(j.is_string());
    EXPECT_EQ(j.as_string(), std::string("Green"));
}

TEST(to_json_enum_all_values) {
    sj::json r = sj::to_json(Color::Red);
    sj::json g = sj::to_json(Color::Green);
    sj::json b = sj::to_json(Color::Blue);
    EXPECT_EQ(r.as_string(), std::string("Red"));
    EXPECT_EQ(g.as_string(), std::string("Green"));
    EXPECT_EQ(b.as_string(), std::string("Blue"));
}

TEST(to_json_enum_with_values) {
    sj::json j = sj::to_json(Priority::High);
    EXPECT_EQ(j.as_string(), std::string("High"));
}

TEST(from_json_enum_valid) {
    auto j = sj::parse(R"("Green")");
    EXPECT_OK(j);
    auto r = sj::from_json<Color>(*j);
    EXPECT_OK(r);
    EXPECT_TRUE(*r == Color::Green);
}

TEST(from_json_enum_invalid_name) {
    auto j = sj::parse(R"("Purple")");
    EXPECT_OK(j);
    auto r = sj::from_json<Color>(*j);
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::conversion);
}

TEST(from_json_enum_non_string) {
    auto j = sj::parse("42");
    EXPECT_OK(j);
    auto r = sj::from_json<Color>(*j);
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::type);
}

TEST(enum_roundtrip) {
    Color c = Color::Blue;
    sj::json j = sj::to_json(c);
    auto r = sj::from_json<Color>(j);
    EXPECT_OK(r);
    EXPECT_TRUE(*r == Color::Blue);
}

// Enum in a struct
struct Config {
    std::string name;
    Color color;
};

TEST(struct_with_enum_roundtrip) {
    Config cfg{"item", Color::Red};
    sj::json j = sj::to_json(cfg);
    EXPECT_EQ(j["color"].as_string(), std::string("Red"));

    auto r = sj::from_json<Config>(j);
    EXPECT_OK(r);
    EXPECT_EQ(r->name, std::string("item"));
    EXPECT_TRUE(r->color == Color::Red);
}
