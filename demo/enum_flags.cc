// Demonstrates enum reflection features: serialization by name,
// deserialization by name matching, enums inside structs, and
// vectors of enums.

#include <iostream>
#include "../simple_json.hpp"

// Simple color enum
enum class Color { Red, Orange, Yellow, Green, Blue, Indigo, Violet };

// HTTP status categories
enum class StatusCode {
    OK          = 200,
    Created     = 201,
    BadRequest  = 400,
    NotFound    = 404,
    ServerError = 500,
};

// Permissions as an enum (individual values, not flags)
enum class Permission { Read, Write, Execute, Admin };

// Task priority
enum class Priority { Low, Medium, High, Critical };

struct Task {
    std::string title;
    Priority priority;
    std::vector<Permission> required_permissions;
    std::optional<Color> label_color;
};

struct TaskBoard {
    std::string name;
    std::vector<Task> tasks;
};

int main() {
    std::cout << "=== Enum Serialization ===\n\n";

    // Single enum values
    for (auto c : {Color::Red, Color::Green, Color::Blue, Color::Violet}) {
        sj::json j = sj::to_json(c);
        std::cout << "  " << j.as_string() << "\n";
    }

    std::cout << "\n=== Enum Deserialization ===\n\n";

    // Parse enum from string
    std::vector<std::string> inputs = {"OK", "NotFound", "ServerError", "Teapot"};
    for (auto const& name : inputs) {
        sj::json j(name);
        auto result = sj::from_json<StatusCode>(j);
        if (result)
            std::cout << "  \"" << name << "\" -> StatusCode (valid)\n";
        else
            std::cout << "  \"" << name << "\" -> error: " << result.error().message << "\n";
    }

    std::cout << "\n=== Struct with Enums ===\n\n";

    TaskBoard board{
        "Sprint 42",
        {
            {"Fix login bug", Priority::Critical,
             {Permission::Read, Permission::Write}, Color::Red},
            {"Update docs", Priority::Low,
             {Permission::Read}, Color::Blue},
            {"Deploy v2.1", Priority::High,
             {Permission::Read, Permission::Write, Permission::Execute, Permission::Admin},
             std::nullopt},
        },
    };

    sj::json j = sj::to_json(board);
    std::cout << j.dump(2) << "\n\n";

    // Round-trip
    auto parsed = sj::from_json<TaskBoard>(j);
    if (!parsed) {
        std::cerr << "Error: " << parsed.error().message << "\n";
        return 1;
    }

    std::cout << "=== Parsed Back ===\n\n";
    std::cout << "Board: " << parsed->name << "\n";
    for (auto const& task : parsed->tasks) {
        std::cout << "  [" << sj::to_json(task.priority).as_string() << "] "
                  << task.title;
        if (task.label_color)
            std::cout << " (label: " << sj::to_json(*task.label_color).as_string() << ")";
        std::cout << "\n    permissions: ";
        for (auto perm : task.required_permissions)
            std::cout << sj::to_json(perm).as_string() << " ";
        std::cout << "\n";
    }

    return 0;
}
