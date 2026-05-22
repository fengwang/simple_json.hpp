#include <iostream>
#include "../simple_json.hpp"

using namespace sj::literals;

// Compile-time JSON parsing — the structure is known at compile time
constexpr auto config = R"({
    "server": {
        "host": "0.0.0.0",
        "port": 8080
    },
    "debug": false,
    "version": 3
})"_json;

// static_assert proves these are compile-time values
static_assert(config.value().server.port == 8080);
static_assert(config.value().debug == false);
static_assert(config.value().version == 3);

int main() {
    // Access at compile time
    std::cout << "Compile-time config:\n"
              << "  host: " << config.value().server.host << "\n"
              << "  port: " << config.value().server.port << "\n"
              << "  debug: " << std::boolalpha << config.value().debug << "\n"
              << "  version: " << config.value().version << "\n\n";

    // Convert to sj::json at runtime
    sj::json j = config;
    std::cout << "As sj::json:\n" << j.dump(2) << "\n";

    return 0;
}
