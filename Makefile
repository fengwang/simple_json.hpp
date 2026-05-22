CC := g++
# -Wno-maybe-uninitialized: suppresses false positives from libstdc++
# variant/string/map internals when inlined at -O2.
CFLAGS := -std=c++26 -freflection -O2 -Wall -Wextra -Wno-maybe-uninitialized

# Demos
DEMOS := basic reflection compile_time containers game_config rest_api enum_flags schema_evolution error_handling
DEMO_TARGETS := $(addprefix demo_,$(DEMOS))

# Tests
TEST_SRCS := $(wildcard test/*.cpp)
TEST_OBJS := $(TEST_SRCS:.cpp=.o)

all: test demos

# --- Tests ---
test: run_tests
	./run_tests

run_tests: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test/%.o: test/%.cpp simple_json.hpp test/test_harness.hpp
	$(CC) $(CFLAGS) -c -o $@ $<

# --- Demos ---
demos: $(DEMO_TARGETS)

demo_basic: demo/basic.cc simple_json.hpp
	$(CC) $(CFLAGS) -o $@ $<

demo_reflection: demo/reflection.cc simple_json.hpp
	$(CC) $(CFLAGS) -o $@ $<

demo_compile_time: demo/compile_time.cc simple_json.hpp
	$(CC) $(CFLAGS) -o $@ $<

demo_containers: demo/containers.cc simple_json.hpp
	$(CC) $(CFLAGS) -o $@ $<

demo_game_config: demo/game_config.cc simple_json.hpp
	$(CC) $(CFLAGS) -o $@ $<

demo_rest_api: demo/rest_api.cc simple_json.hpp
	$(CC) $(CFLAGS) -o $@ $<

demo_enum_flags: demo/enum_flags.cc simple_json.hpp
	$(CC) $(CFLAGS) -o $@ $<

demo_schema_evolution: demo/schema_evolution.cc simple_json.hpp
	$(CC) $(CFLAGS) -o $@ $<

demo_error_handling: demo/error_handling.cc simple_json.hpp
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f run_tests test/*.o $(DEMO_TARGETS)

.PHONY: all test demos clean
