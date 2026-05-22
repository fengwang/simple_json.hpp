// Simulates handling REST API request/response payloads with
// strongly-typed structs — showing how reflection eliminates
// the manual JSON ↔ struct glue code.

#include <iostream>
#include "../simple_json.hpp"

// ---- Domain types ----

enum class HttpMethod { GET, POST, PUT, PATCH, DELETE };
enum class Status { Active, Suspended, Deleted };

struct Pagination {
    int page;
    int per_page;
    int total;
};

struct User {
    int id;
    std::string username;
    std::string email;
    Status status;
    std::vector<std::string> roles;
    std::optional<std::string> avatar_url;
};

struct ApiResponse {
    bool success;
    std::string message;
    std::optional<Pagination> pagination;
};

struct UserListResponse {
    bool success;
    std::vector<User> data;
    Pagination pagination;
};

struct CreateUserRequest {
    std::string username;
    std::string email;
    std::string password;
    std::vector<std::string> roles;
};

// ---- Simulate API handlers ----

sj::json handle_list_users() {
    UserListResponse resp{
        .success = true,
        .data = {
            {1, "alice",   "alice@example.com",   Status::Active,
             {"admin", "user"}, "https://img.example.com/alice.png"},
            {2, "bob",     "bob@example.com",     Status::Active,
             {"user"}, std::nullopt},
            {3, "charlie", "charlie@example.com", Status::Suspended,
             {"user"}, std::nullopt},
        },
        .pagination = {1, 10, 3},
    };
    return sj::to_json(resp);
}

sj::json handle_create_user(sj::json const& body) {
    auto req = sj::from_json<CreateUserRequest>(body);
    if (!req) {
        ApiResponse err{false, "Bad request: " + req.error().message, std::nullopt};
        return sj::to_json(err);
    }

    // "Create" the user
    User created{
        42, req->username, req->email, Status::Active,
        req->roles, std::nullopt
    };

    std::cout << "  Created user: " << created.username
              << " <" << created.email << ">"
              << " with " << created.roles.size() << " role(s)\n";

    return sj::to_json(created);
}

int main() {
    std::cout << "=== GET /users ===\n\n";

    sj::json list_response = handle_list_users();
    std::cout << list_response.dump(2) << "\n\n";

    // Parse the response back into typed data
    auto users = sj::from_json<UserListResponse>(list_response);
    if (users) {
        std::cout << "Parsed " << users->data.size() << " users:\n";
        for (auto const& u : users->data) {
            std::cout << "  #" << u.id << " " << u.username
                      << " [" << sj::to_json(u.status).as_string() << "]";
            if (u.avatar_url) std::cout << " avatar=" << *u.avatar_url;
            std::cout << "\n";
        }
        std::cout << "Page " << users->pagination.page
                  << "/" << ((users->pagination.total + users->pagination.per_page - 1)
                             / users->pagination.per_page) << "\n";
    }

    std::cout << "\n=== POST /users ===\n\n";

    // Build request from JSON (as if received from client)
    auto request_json = sj::parse(R"({
        "username": "diana",
        "email": "diana@example.com",
        "password": "s3cret!",
        "roles": ["user", "moderator"]
    })");

    if (request_json) {
        sj::json created = handle_create_user(*request_json);
        std::cout << "  Response: " << created.dump(2) << "\n";
    }

    std::cout << "\n=== POST /users (bad request) ===\n\n";

    // Missing required field
    auto bad_request = sj::parse(R"({"username": "eve"})");
    if (bad_request) {
        sj::json err = handle_create_user(*bad_request);
        std::cout << "  Response: " << err.dump(2) << "\n";
    }

    return 0;
}
