#include <iostream>
#include <set>
#include <array>
#include "../simple_json.hpp"

int main() {
    // vector → JSON array → vector
    std::vector<int> v{1, 2, 3, 4, 5};
    sj::json jv = sj::to_json(v);
    std::cout << "vector: " << jv << "\n";

    auto v2 = sj::from_json<std::vector<int>>(jv);
    if (v2) std::cout << "  round-trip size: " << v2->size() << "\n";

    // map → JSON object → map
    std::map<std::string, double> m{{"pi", 3.14}, {"e", 2.718}};
    sj::json jm = sj::to_json(m);
    std::cout << "\nmap: " << jm << "\n";

    auto m2 = sj::from_json<std::map<std::string, double>>(jm);
    if (m2) std::cout << "  round-trip pi: " << (*m2)["pi"] << "\n";

    // set → JSON array (deduplicated)
    std::set<std::string> s{"apple", "banana", "cherry"};
    sj::json js = sj::to_json(s);
    std::cout << "\nset: " << js << "\n";

    // optional
    std::optional<int> opt_val(42);
    std::optional<int> opt_none;
    std::cout << "\noptional(42): " << sj::to_json(opt_val) << "\n";
    std::cout << "optional(none): " << sj::to_json(opt_none) << "\n";

    // tuple
    auto t = std::tuple{1, std::string("hello"), true};
    sj::json jt = sj::to_json(t);
    std::cout << "\ntuple: " << jt << "\n";

    auto t2 = sj::from_json<std::tuple<int, std::string, bool>>(jt);
    if (t2) std::cout << "  round-trip: " << std::get<0>(*t2)
                      << ", " << std::get<1>(*t2)
                      << ", " << std::boolalpha << std::get<2>(*t2) << "\n";

    // std::array with size check
    std::array<int, 3> arr{10, 20, 30};
    sj::json ja = sj::to_json(arr);
    std::cout << "\narray<3>: " << ja << "\n";

    auto arr2 = sj::from_json<std::array<int, 3>>(ja);
    if (arr2) std::cout << "  round-trip: " << (*arr2)[0] << ", " << (*arr2)[1] << ", " << (*arr2)[2] << "\n";

    // Size mismatch error
    auto bad = sj::from_json<std::array<int, 5>>(ja);
    if (!bad) std::cout << "  size mismatch error: " << bad.error().message << "\n";

    return 0;
}
