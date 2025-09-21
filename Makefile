CC := g++
CFLAGS := -std=c++23 -O2 -Wall -Wextra

DEMOS := array object printer rect
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

clean:
	rm -f $(TARGETS)

.PHONY: all clean
