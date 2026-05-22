# Specification: Test Harness

## ADDED Requirements

### Requirement: TEST macro

`TEST(name)` SHALL define a test function and register it with the global test registry.

#### Scenario: Test registration
WHEN `TEST(my_test) { EXPECT_TRUE(true); }` is defined in a translation unit linked to the runner
THEN `my_test` SHALL appear in the test registry and be executed by `run_all()`

#### Scenario: Multiple tests in one file
WHEN multiple `TEST(...)` blocks are defined in a single file
THEN all SHALL be registered and executed

---

### Requirement: EXPECT_EQ assertion

`EXPECT_EQ(a, b)` SHALL compare `a` and `b` for equality. On failure, it SHALL print both values, the expression text, and the source location.

#### Scenario: Equal values pass
WHEN `EXPECT_EQ(1 + 1, 2)` is evaluated
THEN the test SHALL pass silently

#### Scenario: Unequal values fail
WHEN `EXPECT_EQ(1, 2)` is evaluated
THEN the test SHALL fail with a message showing `1 != 2` and the file/line

---

### Requirement: EXPECT_TRUE and EXPECT_FALSE assertions

#### Scenario: EXPECT_TRUE with true
WHEN `EXPECT_TRUE(true)` is evaluated
THEN the test SHALL pass

#### Scenario: EXPECT_TRUE with false
WHEN `EXPECT_TRUE(false)` is evaluated
THEN the test SHALL fail with the expression text and source location

#### Scenario: EXPECT_FALSE with false
WHEN `EXPECT_FALSE(false)` is evaluated
THEN the test SHALL pass

---

### Requirement: EXPECT_OK assertion

`EXPECT_OK(expr)` SHALL assert that a `std::expected` expression has a value. On failure, it SHALL print the error.

#### Scenario: Expected with value passes
WHEN `EXPECT_OK(sj::parse("{}"))` is evaluated
THEN the test SHALL pass

#### Scenario: Expected with error fails
WHEN `EXPECT_OK(sj::parse("broken"))` is evaluated
THEN the test SHALL fail with the error message displayed

---

### Requirement: EXPECT_ERR assertion

`EXPECT_ERR(expr)` SHALL assert that a `std::expected` expression holds an error.

#### Scenario: Expected with error passes
WHEN `EXPECT_ERR(sj::parse("broken"))` is evaluated
THEN the test SHALL pass

#### Scenario: Expected with value fails
WHEN `EXPECT_ERR(sj::parse("{}"))` is evaluated
THEN the test SHALL fail

---

### Requirement: EXPECT_ERR_KIND assertion

`EXPECT_ERR_KIND(expr, kind)` SHALL assert that the expression holds an error with the specified `error_kind`.

#### Scenario: Matching error kind
WHEN the expression returns an error with `error_kind::parse` and expected kind is `error_kind::parse`
THEN the test SHALL pass

#### Scenario: Mismatched error kind
WHEN the expression returns an error with `error_kind::type` and expected kind is `error_kind::parse`
THEN the test SHALL fail showing expected vs actual kind

---

### Requirement: Test runner

`sj::test::run_all()` SHALL execute all registered tests, print results, and return `0` on all-pass or `1` on any failure.

#### Scenario: All tests pass
WHEN all registered tests pass
THEN `run_all()` SHALL print a summary like `"12/12 tests passed"` and return `0`

#### Scenario: Some tests fail
WHEN 2 of 12 tests fail
THEN `run_all()` SHALL print each failure with details, print `"10/12 tests passed"`, and return `1`

#### Scenario: Test continues after failure
WHEN a test has multiple assertions and the first fails
THEN subsequent assertions in the same test SHALL still be evaluated (non-fatal assertions)

---

### Requirement: Compile-time tests via static_assert

Compile-time features (consteval parsing, concept satisfaction, type inference) SHALL be tested using `static_assert` in dedicated test files.

#### Scenario: static_assert in test_constexpr.cpp
WHEN `static_assert(sj::JsonSerializable<int>)` is in a test file
THEN it SHALL be verified at compile time — a compilation failure IS a test failure

#### Scenario: Compile-time parse verification
WHEN `constexpr auto v = R"({"x": 1})"_json; static_assert(v.value().x == 1);`
THEN the assertion SHALL pass at compile time

---

### Requirement: Test build target

The Makefile SHALL have a `test` target that compiles all test files and runs the test binary.

#### Scenario: make test
WHEN `make test` is executed
THEN all test files SHALL compile with `-std=c++26 -freflection` and the test binary SHALL run, reporting results
