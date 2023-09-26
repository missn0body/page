CC = g++
CFLAGS = -Wall -Wextra -Wpedantic -g -Os -std=c++20
RM = rm

page.o: page.cpp
	$(CC) -c $^ -o $@ $(CFLAGS)

page: page.o
	$(CC) $^ -o $@ $(CFLAGS)

.PHONY: all clean

all: page

clean:
	$(RM) -f *.o page
