CCFLAGS=-std=c11 -Wall -g -pedantic -Wno-newline-eof -pedantic-errors -O3 -Wextra -Werror -Wno-unused-parameter -Wno-implicit-fallthrough -fsanitize=address -D_POSIX_C_SOURCE=200112L

SOURCES_SERVER=httpd.c

SOURCES_UTILS=$(wildcard utils/*.c)

OBJECTS_UTILS=$(SOURCES_UTILS:.c=.o)


EXECUTABLE_SERVER= httpd

SOURCES_C=$(wildcard *.c)

SOURCES_CPP=$(SOURCES_C:.c=.cpp)

all: $(EXECUTABLE_SERVER)

$(EXECUTABLE_SERVER):  $(OBJECTS_UTILS)
	$(CC) $(CCFLAGS) $(CFLAGS) $(SOURCES_SERVER) -I./includes -o $@ $^

%.o: %.c
	$(CC) $(CCFLAGS) -I./includes -c $^ -o $@
	
test: cpp scanbuild #complexity 

cpp: $(SOURCES_CPP)

%.cpp: %.c
	cppcheck --quiet --enable=all --force --inconclusive $< 2>> results.cppOut

scanbuild: 
	CC=gcc scan-build -disable-checker deadcode.DeadStores -o scanBuildResults make >  results.sb

#complexity:
#	complexity --histogram --score $(SOURCES_C) >results.cpxt 2> /dev/null

clean:
	rm -rf httpd *.o utils/*.o

cleanTest:
	rm -rf results.cppOut results.sb scanBuildResults 

#results.cpxt 

.PHONY: all clean cleanTest cpp pvs test
