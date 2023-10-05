CC = g++
CFLAGS = -Wall -Wextra -Wpedantic -g -Os -std=c++20

LINT = cppcheck
LINTFLAGS = --check-level=exhaustive --enable=all --inconclusive --library=posix --suppress=missingIncludeSystem --verbose

RM = rm

page.o: page.cpp
	$(CC) -c $^ -o $@ $(CFLAGS)

page: page.o
	$(CC) $^ -o $@ $(CFLAGS)

lint: page.cpp
	$(LINT) $(LINTFLAGS) $^

.PHONY: all clean

all: page

clean:
	$(RM) -f *.o page
