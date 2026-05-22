#include "test_harness.hpp"
#include "../simple_json.hpp"

// -- Construction and type queries --

TEST(null_construction) {
    sj::json j;
    EXPECT_TRUE(j.is_null());
    EXPECT_EQ(static_cast<int>(j.type()), static_cast<int>(sj::value_t::null));
}

TEST(nullptr_construction) {
    sj::json j(nullptr);
    EXPECT_TRUE(j.is_null());
}

TEST(bool_construction) {
    sj::json j(true);
    EXPECT_TRUE(j.is_boolean());
    EXPECT_EQ(j.as_boolean(), true);
    EXPECT_EQ(static_cast<int>(j.type()), static_cast<int>(sj::value_t::boolean));
}

TEST(integer_construction) {
    sj::json j(42);
    EXPECT_TRUE(j.is_integer());
    EXPECT_TRUE(j.is_number());
    EXPECT_EQ(j.as_integer(), 42);
    EXPECT_EQ(static_cast<int>(j.type()), static_cast<int>(sj::value_t::integer));
}

TEST(double_construction) {
    sj::json j(3.14);
    EXPECT_TRUE(j.is_number());
    EXPECT_FALSE(j.is_integer());
    EXPECT_EQ(static_cast<int>(j.type()), static_cast<int>(sj::value_t::number));
}

TEST(string_construction) {
    sj::json j("hello");
    EXPECT_TRUE(j.is_string());
    EXPECT_EQ(j.as_string(), std::string("hello"));
}

TEST(string_construction_stdstring) {
    std::string s = "world";
    sj::json j(s);
    EXPECT_TRUE(j.is_string());
    EXPECT_EQ(j.as_string(), std::string("world"));
}

TEST(array_construction) {
    sj::json::array_type arr = {sj::json(1), sj::json(2), sj::json(3)};
    sj::json j(arr);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), std::size_t{3});
}

TEST(object_construction_init_list) {
    sj::json j = {{"key", "value"}, {"num", 42}};
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j.size(), std::size_t{2});
}

// -- is<T>() --

TEST(is_template_match) {
    sj::json j(42);
    EXPECT_TRUE(j.is<sj::json::integer_type>());
    EXPECT_FALSE(j.is<sj::json::string_type>());
}

// -- as<T>() with expected --

TEST(as_success) {
    sj::json j("hello");
    auto r = j.as<sj::json::string_type>();
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(r->get(), std::string("hello"));
}

TEST(as_failure) {
    sj::json j(42);
    auto r = j.as<sj::json::string_type>();
    EXPECT_FALSE(r.has_value());
}

// -- get<T>() with expected --

TEST(get_int_from_integer) {
    sj::json j(42);
    auto r = j.get<int>();
    EXPECT_OK(r);
    EXPECT_EQ(*r, 42);
}

TEST(get_double_from_integer) {
    sj::json j(42);
    auto r = j.get<sj::json::number_type>();
    EXPECT_OK(r);
    EXPECT_EQ(*r, 42.0);
}

TEST(get_int_from_double) {
    sj::json j(3.14);
    auto r = j.get<int>();
    EXPECT_OK(r);
    EXPECT_EQ(*r, 3);
}

TEST(get_string_from_integer_fails) {
    sj::json j(42);
    auto r = j.get<std::string>();
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::type);
}

TEST(get_bool) {
    sj::json j(true);
    auto r = j.get<bool>();
    EXPECT_OK(r);
    EXPECT_EQ(*r, true);
}

// -- value_t type() --

TEST(type_string) {
    sj::json j("test");
    EXPECT_EQ(static_cast<int>(j.type()), static_cast<int>(sj::value_t::string));
}

TEST(type_array) {
    sj::json j(sj::json::array_type{});
    EXPECT_EQ(static_cast<int>(j.type()), static_cast<int>(sj::value_t::array));
}

TEST(type_object) {
    sj::json j(sj::json::object_type{});
    EXPECT_EQ(static_cast<int>(j.type()), static_cast<int>(sj::value_t::object));
}

// -- at() with expected --

TEST(at_key_valid) {
    sj::json j = {{"x", 1}};
    auto r = j.at("x");
    EXPECT_OK(r);
    EXPECT_EQ(r->get().as_integer(), 1);
}

TEST(at_key_missing) {
    sj::json j = {{"x", 1}};
    auto r = j.at("y");
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::missing);
}

TEST(at_index_valid) {
    sj::json j(sj::json::array_type{sj::json(10), sj::json(20)});
    auto r = j.at(std::size_t{0});
    EXPECT_OK(r);
    EXPECT_EQ(r->get().as_integer(), 10);
}

TEST(at_index_out_of_range) {
    sj::json j(sj::json::array_type{sj::json(10)});
    auto r = j.at(std::size_t{5});
    EXPECT_ERR(r);
    EXPECT_ERR_KIND(r, sj::error_kind::missing);
}

// -- value_or --

TEST(value_or_existing) {
    sj::json j = {{"x", 42}};
    EXPECT_EQ(j.value_or("x", 0), 42);
}

TEST(value_or_missing) {
    sj::json j = {{"x", 42}};
    EXPECT_EQ(j.value_or("y", -1), -1);
}

TEST(value_or_string) {
    sj::json j = {{"name", "Alice"}};
    EXPECT_EQ(j.value_or<std::string>("name", "none"), std::string("Alice"));
}

// -- to_double / to_int --

TEST(to_double_number) {
    sj::json j(3.14);
    EXPECT_EQ(j.to_double(), 3.14);
}

TEST(to_double_default) {
    sj::json j("not a number");
    EXPECT_EQ(j.to_double(1.5), 1.5);
}

TEST(to_int_integer) {
    sj::json j(42);
    EXPECT_EQ(j.to_int(), 42);
}

TEST(to_int_default) {
    sj::json j("text");
    EXPECT_EQ(j.to_int(-7), -7);
}

// -- Equality --

TEST(equality_same) {
    sj::json a = {{"x", 1}};
    sj::json b = {{"x", 1}};
    EXPECT_TRUE(a == b);
}

TEST(equality_different) {
    sj::json a = {{"x", 1}};
    sj::json b = {{"x", 2}};
    EXPECT_FALSE(a == b);
}
