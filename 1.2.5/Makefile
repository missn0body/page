# Simply replace CC with your own compiler
# if it doesn't match the ones listed below.

#CC = i586-pc-msdosdjgpp-g++
#CC = i686-w64-mingw32-g++
#CC = x86_64-w64-mingw32-g++
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
