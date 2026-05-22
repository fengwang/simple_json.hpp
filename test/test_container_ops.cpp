#include "test_harness.hpp"
#include "../simple_json.hpp"

// -- Array operations --

TEST(push_back_auto_array) {
    sj::json j;
    j.push_back(1);
    j.push_back("two");
    j.push_back(true);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), std::size_t{3});
}

TEST(emplace_back) {
    sj::json j;
    j.emplace_back(42);
    j.emplace_back(3.14);
    EXPECT_EQ(j.size(), std::size_t{2});
    EXPECT_EQ(j[std::size_t{0}].as_integer(), 42);
}

TEST(operator_bracket_index) {
    sj::json j(sj::json::array_type{sj::json(10), sj::json(20)});
    j[std::size_t{0}] = 99;
    EXPECT_EQ(j[std::size_t{0}].as_integer(), 99);
}

// -- Object operations --

TEST(operator_bracket_key_autovivify) {
    sj::json j;
    j["key"] = 42;
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["key"].as_integer(), 42);
}

TEST(emplace_key_value) {
    sj::json j;
    j.emplace("weather", "sunny");
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["weather"].as_string(), std::string("sunny"));
}

TEST(contains_key) {
    sj::json j = {{"x", 1}};
    EXPECT_TRUE(j.contains("x"));
    EXPECT_FALSE(j.contains("y"));
}

TEST(contains_index) {
    sj::json j(sj::json::array_type{sj::json(1), sj::json(2)});
    EXPECT_TRUE(j.contains(std::size_t{0}));
    EXPECT_FALSE(j.contains(std::size_t{5}));
}

TEST(size_empty_null) {
    sj::json j;
    EXPECT_EQ(j.size(), std::size_t{0});
    EXPECT_TRUE(j.empty());
}

TEST(size_scalar) {
    sj::json j(42);
    EXPECT_EQ(j.size(), std::size_t{1});
    EXPECT_FALSE(j.empty());
}

TEST(clear_array) {
    sj::json j(sj::json::array_type{sj::json(1), sj::json(2)});
    j.clear();
    EXPECT_TRUE(j.is_array());
    EXPECT_TRUE(j.empty());
}

TEST(clear_scalar) {
    sj::json j(42);
    j.clear();
    EXPECT_TRUE(j.is_null());
}

TEST(erase_key) {
    sj::json j = {{"a", 1}, {"b", 2}};
    j.erase("a");
    EXPECT_FALSE(j.contains("a"));
    EXPECT_TRUE(j.contains("b"));
}

TEST(find_key) {
    sj::json j = {{"x", 1}};
    auto it = j.find("x");
    EXPECT_TRUE(it != j.end());
    EXPECT_EQ(it.key(), std::string("x"));
    EXPECT_EQ(it.value().as_integer(), 1);
}

TEST(find_missing) {
    sj::json j = {{"x", 1}};
    auto it = j.find("y");
    EXPECT_TRUE(it == j.end());
}

TEST(count_key) {
    sj::json j = {{"x", 1}};
    EXPECT_EQ(j.count("x"), std::size_t{1});
    EXPECT_EQ(j.count("y"), std::size_t{0});
}

// -- Iterator tests --

TEST(range_for_array) {
    sj::json j(sj::json::array_type{sj::json(1), sj::json(2), sj::json(3)});
    int sum = 0;
    for (auto const& el : j) {
        sum += el.as_integer();
    }
    EXPECT_EQ(sum, 6);
}

TEST(range_for_object_values) {
    sj::json j = {{"a", 1}, {"b", 2}};
    int sum = 0;
    for (auto const& el : j) {
        sum += el.as_integer();
    }
    EXPECT_EQ(sum, 3);
}

TEST(items_view) {
    sj::json j = {{"x", 10}, {"y", 20}};
    int sum = 0;
    for (auto item : j.items()) {
        sum += item.value().as_integer();
    }
    EXPECT_EQ(sum, 30);
}

TEST(items_structured_bindings) {
    sj::json j = {{"a", 1}};
    for (auto [key, value] : j.items()) {
        EXPECT_EQ(key, std::string("a"));
        EXPECT_EQ(value.as_integer(), 1);
    }
}

// -- STL container constructors --

TEST(construct_from_vector) {
    std::vector<int> v{1, 2, 3};
    sj::json j(v);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), std::size_t{3});
}

TEST(construct_from_map) {
    std::map<std::string, int> m{{"a", 1}, {"b", 2}};
    sj::json j(m);
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j.size(), std::size_t{2});
}
