#include <iostream>
#include <sstream>
#include "../simple_json.hpp"

int main() {
    using sj::json;
    using namespace sj::literals;

    // 1. Parse from string
    auto r = sj::parse(R"({
        "pi": 3.141,
        "happy": true,
        "name": "Niels",
        "nothing": null,
        "answer": { "everything": 42 },
        "list": [1, 0, 2]
    })");

    if (!r) {
        std::cerr << "parse error: " << r.error().message << "\n";
        return 1;
    }
    json j = *r;

    // 2. Access values
    std::cout << "pi = " << j["pi"].to_double() << "\n";
    std::cout << "happy = " << std::boolalpha << j["happy"].as_boolean() << "\n";
    std::cout << "name = " << j["name"].as_string() << "\n";
    std::cout << "answer.everything = " << j["answer"]["everything"].as_integer() << "\n";

    // 3. get<T>() with expected
    auto name = j["name"].get<std::string>();
    if (name) std::cout << "got name: " << *name << "\n";

    // 4. value_or
    double pi = j.value_or("pi", 0.0);
    std::cout << "pi via value_or: " << pi << "\n";

    // 5. Build JSON in code
    json built = {
        {"greeting", "hello"},
        {"count", 42},
        {"flag", true},
    };
    std::cout << "\nbuilt: " << built.dump(2) << "\n";

    // 6. Compile-time literal → runtime json
    json from_literal = R"({"x": 1, "y": 2})"_json;
    std::cout << "from literal: " << from_literal << "\n";

    // 7. Round-trip
    std::string dumped = j.dump(2);
    auto r2 = sj::parse(dumped);
    if (r2 && *r2 == j)
        std::cout << "\nround-trip OK\n";

    return 0;
}
