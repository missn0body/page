CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g -Os
RM = rm

page.o: page.c
	$(CC) -c $^ -o $@ $(CFLAGS)

page: page.o
	$(CC) $^ -o $@ $(CFLAGS)

.PHONY: all clean

all: page

clean:
	$(RM) -f *.o page
