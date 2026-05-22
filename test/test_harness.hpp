#ifndef SJ_TEST_HARNESS_HPP
#define SJ_TEST_HARNESS_HPP

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>
#include <source_location>
#include <format>
#include <functional>

namespace sj::test {

struct test_case {
    std::string_view name;
    std::source_location loc;
    std::function<void()> fn;
};

inline std::vector<test_case>& registry() {
    static std::vector<test_case> cases;
    return cases;
}

struct registrar {
    registrar(std::string_view name, std::function<void()> fn,
              std::source_location loc = std::source_location::current()) {
        registry().push_back({name, loc, std::move(fn)});
    }
};

inline thread_local int current_failures = 0;
inline thread_local std::string current_test_name;

inline void fail(std::string_view msg, std::source_location loc) {
    std::fprintf(stderr, "  FAIL %s:%d: %s\n",
                 loc.file_name(), static_cast<int>(loc.line()), msg.data());
    ++current_failures;
}

inline int run_all() {
    int total = 0, passed = 0, failed = 0;
    for (auto const& tc : registry()) {
        ++total;
        current_failures = 0;
        current_test_name = tc.name;
        std::fprintf(stdout, "[ RUN  ] %s\n", std::string(tc.name).c_str());
        tc.fn();
        if (current_failures == 0) {
            ++passed;
            std::fprintf(stdout, "[   OK ] %s\n", std::string(tc.name).c_str());
        } else {
            ++failed;
            std::fprintf(stdout, "[ FAIL ] %s (%d failures)\n",
                         std::string(tc.name).c_str(), current_failures);
        }
    }
    std::fprintf(stdout, "\n%d/%d tests passed", passed, total);
    if (failed > 0)
        std::fprintf(stdout, ", %d FAILED", failed);
    std::fprintf(stdout, "\n");
    return failed > 0 ? 1 : 0;
}

} // namespace sj::test

#define SJ_TEST_CONCAT_(a, b) a##b
#define SJ_TEST_CONCAT(a, b) SJ_TEST_CONCAT_(a, b)

#define TEST(name) \
    static void SJ_TEST_CONCAT(test_fn_, name)(); \
    static sj::test::registrar SJ_TEST_CONCAT(test_reg_, name){ \
        #name, SJ_TEST_CONCAT(test_fn_, name)}; \
    static void SJ_TEST_CONCAT(test_fn_, name)()

#define EXPECT_TRUE(expr) \
    do { \
        if (!(expr)) \
            sj::test::fail(std::format("EXPECT_TRUE({}) failed", #expr), \
                           std::source_location::current()); \
    } while (false)

#define EXPECT_FALSE(expr) \
    do { \
        if ((expr)) \
            sj::test::fail(std::format("EXPECT_FALSE({}) failed", #expr), \
                           std::source_location::current()); \
    } while (false)

#define EXPECT_EQ(a, b) \
    do { \
        auto const& lhs_ = (a); \
        auto const& rhs_ = (b); \
        if (!(lhs_ == rhs_)) \
            sj::test::fail(std::format("EXPECT_EQ({}, {}): {} != {}", \
                           #a, #b, lhs_, rhs_), \
                           std::source_location::current()); \
    } while (false)

#define EXPECT_NE(a, b) \
    do { \
        auto const& lhs_ = (a); \
        auto const& rhs_ = (b); \
        if (lhs_ == rhs_) \
            sj::test::fail(std::format("EXPECT_NE({}, {}): both equal {}", \
                           #a, #b, lhs_), \
                           std::source_location::current()); \
    } while (false)

#define EXPECT_OK(expr) \
    do { \
        auto const& res_ = (expr); \
        if (!res_.has_value()) \
            sj::test::fail(std::format("EXPECT_OK({}) failed: {}", \
                           #expr, res_.error().message), \
                           std::source_location::current()); \
    } while (false)

#define EXPECT_ERR(expr) \
    do { \
        auto const& res_ = (expr); \
        if (res_.has_value()) \
            sj::test::fail(std::format("EXPECT_ERR({}) failed: has value", \
                           #expr), \
                           std::source_location::current()); \
    } while (false)

#define EXPECT_ERR_KIND(expr, k) \
    do { \
        auto const& res_ = (expr); \
        if (res_.has_value()) \
            sj::test::fail(std::format("EXPECT_ERR_KIND({}, {}): has value", \
                           #expr, #k), \
                           std::source_location::current()); \
        else if (res_.error().kind != (k)) \
            sj::test::fail(std::format("EXPECT_ERR_KIND({}, {}): got kind {}", \
                           #expr, #k, static_cast<int>(res_.error().kind)), \
                           std::source_location::current()); \
    } while (false)

#endif // SJ_TEST_HARNESS_HPP
