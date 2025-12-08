CC := g++
CFLAGS := -std=c++23 -O2 -Wall -Wextra

DEMOS := array object printer rect json_test json_container
TARGETS := $(DEMOS)

all: $(TARGETS)

array: demo/array.cc
	$(CC) $(CFLAGS) -o $@ $<

object: demo/object.cc
	$(CC) $(CFLAGS) -o $@ $<

printer: demo/printer.cc
	$(CC) $(CFLAGS) -o $@ $<

rect: demo/rect.cc
	$(CC) $(CFLAGS) -o $@ $<

json_test: demo/json_test.cc
	$(CC) $(CFLAGS) -o $@ $<

json_container: demo/json_container.cc
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
