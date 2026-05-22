#include "test_harness.hpp"
#include "../simple_json.hpp"

TEST(error_construction) {
    sj::error e{sj::error_kind::type, "expected integer", ""};
    EXPECT_EQ(static_cast<int>(e.kind), static_cast<int>(sj::error_kind::type));
    EXPECT_EQ(e.message, std::string("expected integer"));
    EXPECT_EQ(e.path, std::string(""));
}

TEST(error_with_context_on_empty) {
    sj::error e{sj::error_kind::type, "msg", ""};
    auto e2 = e.with_context("field");
    EXPECT_EQ(e2.path, std::string("field"));
}

TEST(error_with_context_prepends) {
    sj::error e{sj::error_kind::type, "msg", "x"};
    auto e2 = e.with_context("child");
    EXPECT_EQ(e2.path, std::string("child.x"));
}

TEST(error_with_context_chain) {
    sj::error e{sj::error_kind::type, "msg", ""};
    auto e2 = e.with_context("c").with_context("b").with_context("a");
    EXPECT_EQ(e2.path, std::string("a.b.c"));
}
