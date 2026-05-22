// Demonstrates how std::optional fields enable schema evolution:
// older JSON missing new fields still deserializes correctly,
// and new fields are silently set to nullopt.

#include <iostream>
#include "../simple_json.hpp"

// Version 1 of the config (original)
// struct ConfigV1 { std::string name; int port; };

// Version 2 adds optional fields — V1 JSON still loads fine
struct Config {
    std::string name;
    int port;
    std::optional<bool> tls_enabled;
    std::optional<std::string> tls_cert_path;
    std::optional<int> max_connections;
    std::optional<std::vector<std::string>> allowed_origins;
};

int main() {
    std::cout << "=== Loading V1 JSON (missing new fields) ===\n\n";

    auto v1 = sj::parse(R"({
        "name": "api-server",
        "port": 8080
    })");

    if (v1) {
        auto cfg = sj::from_json<Config>(*v1);
        if (cfg) {
            std::cout << "name: " << cfg->name << "\n";
            std::cout << "port: " << cfg->port << "\n";
            std::cout << "tls_enabled:     " << (cfg->tls_enabled ? "set" : "nullopt") << "\n";
            std::cout << "tls_cert_path:   " << (cfg->tls_cert_path ? "set" : "nullopt") << "\n";
            std::cout << "max_connections: " << (cfg->max_connections ? "set" : "nullopt") << "\n";
            std::cout << "allowed_origins: " << (cfg->allowed_origins ? "set" : "nullopt") << "\n";
        }
    }

    std::cout << "\n=== Loading V2 JSON (all fields present) ===\n\n";

    auto v2 = sj::parse(R"({
        "name": "api-server",
        "port": 443,
        "tls_enabled": true,
        "tls_cert_path": "/etc/ssl/server.pem",
        "max_connections": 1000,
        "allowed_origins": ["https://app.example.com", "https://admin.example.com"]
    })");

    if (v2) {
        auto cfg = sj::from_json<Config>(*v2);
        if (cfg) {
            std::cout << "name: " << cfg->name << "\n";
            std::cout << "port: " << cfg->port << "\n";
            std::cout << "tls:  " << std::boolalpha << *cfg->tls_enabled << "\n";
            std::cout << "cert: " << *cfg->tls_cert_path << "\n";
            std::cout << "max:  " << *cfg->max_connections << "\n";
            std::cout << "origins: ";
            for (auto const& o : *cfg->allowed_origins)
                std::cout << o << " ";
            std::cout << "\n";

            // Serialize back — optional fields with values are included
            std::cout << "\n" << sj::to_json(*cfg).dump(2) << "\n";
        }
    }

    return 0;
}
