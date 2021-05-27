CC= gcc
GCCFLAGS= -std=c99 -Wall -pedantic -pedantic-errors -Wextra -Werror -Wno-unused-parameter -Wno-implicit-fallthrough -fsanitize=address 

SOURCES_SERVER= httpd.c

SOURCES_UTILS=$(wildcard utils/*.c)

EXECUTABLE_SERVER= httpd

SOURCES_C=$(wildcard *.c)

SOURCES_CPP=$(SOURCES_C:.c=.cpp)

all:
	$(CC) $(GCCFLAGS) $(SOURCES_SERVER) $(SOURCES_UTILS) -I./includes -o $(EXECUTABLE_SERVER)

test: cpp scanbuild #complexity 

cpp: $(SOURCES_CPP)

%.cpp: %.c
	cppcheck --quiet --enable=all --force --inconclusive $< 2>> results.cppOut

scanbuild: 
	CC=gcc scan-build -disable-checker deadcode.DeadStores -o scanBuildResults make >  results.sb

#complexity:
#	complexity --histogram --score $(SOURCES_C) >results.cpxt 2> /dev/null

clean:
	rm -rf httpd

cleanTest:
	rm -rf results.cppOut results.sb scanBuildResults 

#results.cpxt 

.PHONY: all clean cleanTest cpp pvs test
