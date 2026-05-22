// Demonstrates the std::expected error model with monadic chaining,
// error path traces for nested structs, and recovery patterns.

#include <iostream>
#include "../simple_json.hpp"

enum class Env { Development, Staging, Production };

struct Database {
    std::string host;
    int port;
    std::string name;
};

struct AppConfig {
    Env environment;
    Database database;
    std::vector<std::string> features;
};

void show_error(sj::error const& e) {
    std::cout << "  kind:    " << static_cast<int>(e.kind) << "\n";
    std::cout << "  message: " << e.message << "\n";
    std::cout << "  path:    " << (e.path.empty() ? "(root)" : e.path) << "\n\n";
}

int main() {
    std::cout << "=== 1. Parse error ===\n\n";
    {
        auto r = sj::parse("{ broken json }}}");
        if (!r) show_error(r.error());
    }

    std::cout << "=== 2. Missing required field ===\n\n";
    {
        auto r = sj::parse(R"({
            "environment": "Production",
            "database": { "host": "db.prod.internal", "name": "myapp" },
            "features": ["auth"]
        })");
        // database.port is missing
        if (r) {
            auto cfg = sj::from_json<AppConfig>(*r);
            if (!cfg) show_error(cfg.error());
        }
    }

    std::cout << "=== 3. Type mismatch deep in nesting ===\n\n";
    {
        auto r = sj::parse(R"({
            "environment": "Production",
            "database": { "host": "db.prod.internal", "port": "not_a_number", "name": "myapp" },
            "features": ["auth"]
        })");
        if (r) {
            auto cfg = sj::from_json<AppConfig>(*r);
            if (!cfg) show_error(cfg.error());
        }
    }

    std::cout << "=== 4. Invalid enum value ===\n\n";
    {
        auto r = sj::parse(R"({
            "environment": "Canary",
            "database": { "host": "localhost", "port": 5432, "name": "dev" },
            "features": []
        })");
        if (r) {
            auto cfg = sj::from_json<AppConfig>(*r);
            if (!cfg) show_error(cfg.error());
        }
    }

    std::cout << "=== 5. Monadic chaining ===\n\n";
    {
        std::string input = R"({
            "environment": "Development",
            "database": { "host": "localhost", "port": 5432, "name": "devdb" },
            "features": ["auth", "logging", "metrics"]
        })";

        auto port = sj::parse(input)
            .and_then([](sj::json const& j) { return sj::from_json<AppConfig>(j); })
            .transform([](AppConfig const& cfg) { return cfg.database.port; });

        if (port)
            std::cout << "  Database port: " << *port << "\n\n";
        else
            std::cout << "  Error: " << port.error().message << "\n\n";
    }

    std::cout << "=== 6. Successful round-trip ===\n\n";
    {
        AppConfig cfg{
            Env::Production,
            {"db.prod.internal", 5432, "myapp_prod"},
            {"auth", "rate_limiting", "audit_log"},
        };

        sj::json j = sj::to_json(cfg);
        auto cfg2 = sj::from_json<AppConfig>(j);
        if (cfg2) {
            std::cout << "  env:      " << sj::to_json(cfg2->environment).as_string() << "\n";
            std::cout << "  db:       " << cfg2->database.host << ":" << cfg2->database.port << "\n";
            std::cout << "  features: " << cfg2->features.size() << " enabled\n";
        }
    }

    return 0;
}
