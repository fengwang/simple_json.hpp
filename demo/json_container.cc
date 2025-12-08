#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <array>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

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

    // ----- Conversions from STL containers -----

    std::vector<int> c_vector {1, 2, 3, 4};
    json j_vec(c_vector);
    std::cout << "j_vec = " << j_vec << "\n";

    std::deque<double> c_deque {1.2, 2.3, 3.4, 5.6};
    json j_deque(c_deque);
    std::cout << "j_deque = " << j_deque << "\n";

    std::list<bool> c_list {true, true, false, true};
    json j_list(c_list);
    std::cout << "j_list = " << j_list << "\n";

    std::forward_list<std::int64_t> c_flist {
        12345678909876LL,
        23456789098765LL,
        34567890987654LL,
        45678909876543LL
    };
    json j_flist(c_flist);
    std::cout << "j_flist = " << j_flist << "\n";

    std::array<unsigned long, 4> c_array {{1, 2, 3, 4}};
    json j_array(c_array);
    std::cout << "j_array = " << j_array << "\n\n";

    std::set<std::string> c_set {"one", "two", "three", "four", "one"};
    json j_set(c_set);
    std::cout << "j_set = " << j_set << "\n";

    std::unordered_set<std::string> c_uset {"one", "two", "three", "four", "one"};
    json j_uset(c_uset);
    std::cout << "j_uset = " << j_uset << "\n";

    std::multiset<std::string> c_mset {"one", "two", "one", "four"};
    json j_mset(c_mset);
    std::cout << "j_mset = " << j_mset << "\n";

    std::unordered_multiset<std::string> c_umset {"one", "two", "one", "four"};
    json j_umset(c_umset);
    std::cout << "j_umset = " << j_umset << "\n\n";

    std::map<std::string, int> c_map { {"one", 1}, {"two", 2}, {"three", 3} };
    json j_map(c_map);
    std::cout << "j_map = " << j_map << "\n";

    std::unordered_map<const char*, double> c_umap {
        {"one", 1.2}, {"two", 2.3}, {"three", 3.4}
    };
    json j_umap(c_umap);
    std::cout << "j_umap = " << j_umap << "\n\n";

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
