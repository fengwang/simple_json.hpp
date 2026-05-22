#include <iostream>
#include "../simple_json.hpp"

// User-defined types — no boilerplate needed for JSON conversion
enum class Role { Admin, User, Guest };

struct Address {
    std::string city;
    std::string state;
    int zip;
};

struct Person {
    std::string name;
    int age;
    Role role;
    Address address;
    std::vector<std::string> tags;
    std::optional<double> rating;
};

int main() {
    // Serialize struct → JSON automatically via reflection
    Person alice{
        "Alice", 30, Role::Admin,
        {"New York", "NY", 10001},
        {"developer", "team-lead"},
        4.8
    };

    sj::json j = sj::to_json(alice);
    std::cout << "Serialized:\n" << j.dump(2) << "\n\n";

    // Deserialize JSON → struct
    auto parsed = sj::from_json<Person>(j);
    if (!parsed) {
        std::cerr << "error: " << parsed.error().message
                  << " at " << parsed.error().path << "\n";
        return 1;
    }

    Person& p = *parsed;
    std::cout << "Deserialized:\n"
              << "  name: " << p.name << "\n"
              << "  age: " << p.age << "\n"
              << "  role: " << sj::to_json(p.role).as_string() << "\n"
              << "  city: " << p.address.city << "\n"
              << "  tags: " << p.tags.size() << " items\n";

    if (p.rating)
        std::cout << "  rating: " << *p.rating << "\n";

    // Error path demonstration
    auto bad = sj::parse(R"({"name": "Bob", "age": "not_a_number", "role": "User",
        "address": {"city": "LA", "state": "CA", "zip": 90001}, "tags": []})");
    if (bad) {
        auto result = sj::from_json<Person>(*bad);
        if (!result) {
            std::cout << "\nExpected error: " << result.error().message
                      << " at path: " << result.error().path << "\n";
        }
    }

    return 0;
}
