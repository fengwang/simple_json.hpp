#include "test_harness.hpp"

TEST(smoke_true)  { EXPECT_TRUE(true); }
TEST(smoke_false) { EXPECT_FALSE(false); }
TEST(smoke_eq)    { EXPECT_EQ(1 + 1, 2); }
TEST(smoke_ne)    { EXPECT_NE(1, 2); }
