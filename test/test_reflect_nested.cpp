#include "test_harness.hpp"
#include "../simple_json.hpp"

enum class Status { Active, Inactive };

struct Address {
    std::string city;
    int zip;
};

struct Person {
    std::string name;
    int age;
    Address address;
    std::vector<std::string> tags;
    std::optional<double> score;
    Status status;
};

TEST(nested_struct_roundtrip) {
    Person p{
        "Alice", 30,
        {"NYC", 10001},
        {"dev", "lead"},
        42.5,
        Status::Active
    };

    sj::json j = sj::to_json(p);
    EXPECT_EQ(j["name"].as_string(), std::string("Alice"));
    EXPECT_EQ(j["age"].as_integer(), 30);
    EXPECT_EQ(j["address"]["city"].as_string(), std::string("NYC"));
    EXPECT_EQ(j["address"]["zip"].as_integer(), 10001);
    EXPECT_TRUE(j["tags"].is_array());
    EXPECT_EQ(j["tags"].size(), std::size_t{2});
    EXPECT_EQ(j["status"].as_string(), std::string("Active"));

    auto r = sj::from_json<Person>(j);
    EXPECT_OK(r);
    EXPECT_EQ(r->name, std::string("Alice"));
    EXPECT_EQ(r->age, 30);
    EXPECT_EQ(r->address.city, std::string("NYC"));
    EXPECT_EQ(r->address.zip, 10001);
    EXPECT_EQ(r->tags.size(), std::size_t{2});
    EXPECT_TRUE(r->score.has_value());
    EXPECT_TRUE(r->status == Status::Active);
}

TEST(nested_missing_optional) {
    auto j = sj::parse(R"({
        "name": "Bob", "age": 25,
        "address": {"city": "LA", "zip": 90001},
        "tags": [],
        "status": "Inactive"
    })");
    EXPECT_OK(j);
    auto r = sj::from_json<Person>(*j);
    EXPECT_OK(r);
    EXPECT_FALSE(r->score.has_value());
}

// -- vector of structs --

struct Item { std::string id; int count; };

TEST(vector_of_structs) {
    std::vector<Item> items{{"a", 1}, {"b", 2}};
    sj::json j = sj::to_json(items);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), std::size_t{2});
    EXPECT_EQ(j[std::size_t{0}]["id"].as_string(), std::string("a"));

    auto r = sj::from_json<std::vector<Item>>(j);
    EXPECT_OK(r);
    EXPECT_EQ(r->size(), std::size_t{2});
    EXPECT_EQ((*r)[0].id, std::string("a"));
    EXPECT_EQ((*r)[1].count, 2);
}

// -- map of structs --

TEST(map_of_structs) {
    std::map<std::string, Item> m{
        {"first", {"a", 1}},
        {"second", {"b", 2}}
    };
    sj::json j = sj::to_json(m);
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["first"]["id"].as_string(), std::string("a"));

    auto r = sj::from_json<std::map<std::string, Item>>(j);
    EXPECT_OK(r);
    EXPECT_EQ((*r)["first"].id, std::string("a"));
}

// -- struct with variant --

struct Tagged {
    std::string label;
    std::variant<int, std::string> data;
};

TEST(struct_with_variant_roundtrip) {
    Tagged t{"test", 42};
    sj::json j = sj::to_json(t);
    EXPECT_TRUE(j["data"].is_object());
    EXPECT_TRUE(j["data"].contains("_type"));

    auto r = sj::from_json<Tagged>(j);
    EXPECT_OK(r);
    EXPECT_EQ(r->label, std::string("test"));
    EXPECT_TRUE(std::holds_alternative<int>(r->data));
    EXPECT_EQ(std::get<int>(r->data), 42);
}

// -- struct with unique_ptr --

struct WithPtr {
    std::string name;
    std::unique_ptr<int> value;
};

TEST(struct_with_unique_ptr) {
    // to_json
    WithPtr w;
    w.name = "test";
    w.value = std::make_unique<int>(99);
    sj::json j = sj::to_json(w);
    EXPECT_EQ(j["value"].as_integer(), 99);

    // from_json
    auto r = sj::from_json<WithPtr>(j);
    EXPECT_OK(r);
    EXPECT_EQ(r->name, std::string("test"));
    EXPECT_TRUE(r->value != nullptr);
    EXPECT_EQ(*r->value, 99);
}

// -- deeply nested error propagation --

struct Deep3 { int val; };
struct Deep2 { Deep3 inner; };
struct Deep1 { Deep2 mid; };

TEST(deep_nested_error_path) {
    auto j = sj::parse(R"({"mid": {"inner": {"val": "not_int"}}})");
    EXPECT_OK(j);
    auto r = sj::from_json<Deep1>(*j);
    EXPECT_ERR(r);
    EXPECT_EQ(r.error().path, std::string("mid.inner.val"));
}
