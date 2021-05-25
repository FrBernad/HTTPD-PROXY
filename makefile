CC= gcc
GCCFLAGS= -std=c99 -Wall -pedantic -pedantic-errors -Wextra -Werror -Wno-unused-parameter -Wno-implicit-fallthrough -fsanitize=address 

SOURCES_SERVER= httpd.c

SOURCES_UTILS=$(wildcard utils/*.c)

EXECUTABLE_SERVER= httpd

all:
	$(CC) $(GCCFLAGS) $(SOURCES_SERVER) $(SOURCES_UTILS) -I./utils -o $(EXECUTABLE_SERVER)

clean:
	rm -rf httpd

.PHONY: all clean
