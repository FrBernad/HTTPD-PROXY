CCFLAGS=-std=c11 -Wall -g -pedantic -pthread -Wno-newline-eof -pedantic-errors -O3 -Wextra -Werror -Wno-unused-parameter -Wno-implicit-fallthrough -fsanitize=address -D_POSIX_C_SOURCE=200112L

SOURCES_SERVER=httpd.c

SOURCES_UTILS=$(wildcard utils/*.c)
SOURCES_STATE_MACHINE=$(wildcard state_machine/*.c)
SOURCES_STATES=$(wildcard state_machine/states/*.c)
SOURCES_PARSERS=$(wildcard parsers/*.c)

OBJECTS_UTILS=$(SOURCES_UTILS:.c=.o)
OBJECTS_STATE_MACHINE=$(SOURCES_STATE_MACHINE:.c=.o)
OBJECTS_STATES=$(SOURCES_STATES:.c=.o)
OBJECTS_PARSERS=$(SOURCES_PARSERS:.c=.o)

EXECUTABLE_SERVER=httpd

SOURCES_C=$(wildcard *.c)

SOURCES_CPP=$(SOURCES_C:.c=.cpp)

all: $(EXECUTABLE_SERVER)

$(EXECUTABLE_SERVER): $(OBJECTS_UTILS) $(OBJECTS_STATE_MACHINE) $(OBJECTS_STATES) $(OBJECTS_PARSERS)
	$(CC) $(CCFLAGS) $(CFLAGS) $(SOURCES_SERVER) -o $@ $^

%.o: %.c
	$(CC) $(CCFLAGS) -c $^ -o $@
	
test: cpp scanbuild #complexity 

cpp: $(SOURCES_CPP)

%.cpp: %.c
	cppcheck --quiet --enable=all --force --inconclusive $< 2>> results.cppOut

scanbuild: 
	CC=gcc scan-build -disable-checker deadcode.DeadStores -o scanBuildResults make >  results.sb

#complexity:
#	complexity --histogram --score $(SOURCES_C) >results.cpxt 2> /dev/null

clean:
	rm -rf httpd *.o utils/*.o parsers/*.o state_machine/*.o state_machine/states/*.o

cleanTest:
	rm -rf results.cppOut results.sb scanBuildResults 

#results.cpxt 

.PHONY: all clean cleanTest cpp pvs test
