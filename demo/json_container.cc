#include <iostream>

#include "../simple_json.hpp"

int main() {
    using sj::json;
    using namespace sj::literals;

    std::cout << std::boolalpha;

    // ----- Array-like usage -----
    json j; // null, will become an array on first push_back
    j.push_back("foo");
    j.push_back(1);
    j.push_back(true);
    j.emplace_back(1.78);

    std::cout << "j = " << j << "\n";

    // iterate with iterators
    std::cout << "iterator loop:\n";
    for (auto it = j.begin(); it != j.end(); ++it) {
        std::cout << "  " << *it << '\n';
    }

    // range-based for
    std::cout << "range-for loop:\n";
    for (auto& element : j) {
        std::cout << "  " << element << '\n';
    }

    // getter / setter
    const auto tmp = j[std::size_t{0}].get<json::string_type>();
    j[1] = 42;
    bool flag = j.at(2); // implicit get<bool>()
    std::cout << "tmp = " << tmp << ", j[1] = " << j[1]
              << ", flag = " << flag << "\n";

    // comparison
    json expected_array = R"(["foo", 42, true, 1.78])"_json;
    std::cout << "j == expected_array ? " << (j == expected_array) << "\n";

    // size / empty / type / clear
    std::cout << "size = " << j.size()
              << ", empty = " << j.empty()
              << ", type = " << static_cast<int>(j.type()) << "\n";

    j.clear();
    std::cout << "after clear: size = " << j.size()
              << ", empty = " << j.empty()
              << ", type = " << static_cast<int>(j.type()) << "\n\n";

    // ----- Object-like usage -----
    json o;
    o["foo"] = 23;
    o["bar"] = false;
    o["baz"] = 3.141;
    o.emplace("weather", "sunny");

    std::cout << "o = " << o.dump(2) << "\n";

    // iterator over key/value pairs
    std::cout << "iterator over object:\n";
    for (auto it = o.begin(); it != o.end(); ++it) {
        std::cout << "  " << it.key() << " : " << it.value() << "\n";
    }

    // items() view
    std::cout << "items() view:\n";
    for (auto el : o.items()) {
        std::cout << "  " << el.key() << " : " << el.value() << "\n";
    }

    // structured bindings (C++17)
    std::cout << "structured bindings over items():\n";
    for (auto [key, value] : o.items()) {
        std::cout << "  " << key << " : " << value << "\n";
    }

    // lookup helpers
    std::cout << "contains(\"foo\") = " << o.contains("foo") << "\n";
    std::cout << "count(\"foo\") = " << o.count("foo")
              << ", count(\"fob\") = " << o.count("fob") << "\n";

    if (o.find("foo") != o.end()) {
        std::cout << "\"foo\" found via find()\n";
    }

    // erase
    o.erase("foo");
    std::cout << "after erase(\"foo\"): " << o.dump(2) << "\n";

    return 0;
}
