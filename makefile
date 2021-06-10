include makefile.inc
include source_dirs.inc

CFLAGS+=-I$(CURDIR)

SOURCES_SERVER=httpd.c
EXECUTABLE_SERVER=httpd

EXECUTABLE_MANAGEMENT_CLIENT=client

SOURCES_MANAGEMENT_CLIENT_FILES=$(foreach dir,$(MANAGEMENT_CLIENT_DIRS),$(wildcard $(dir)/*.c))
OBJECTS_MANAGEMENT_CLIENT_FILES=$(SOURCES_MANAGEMENT_CLIENT_FILES:.c=.o)

SOURCES_FILES=$(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJECTS_FILES=$(SOURCES_FILES:.c=.o)

SOURCES_CPP=$(SOURCES_C:.c=.cpp)

all: $(EXECUTABLE_SERVER) $(EXECUTABLE_MANAGEMENT_CLIENT)

$(EXECUTABLE_SERVER): $(OBJECTS_FILES)
	$(CC) $(CFLAGS) $(SOURCES_SERVER) $^ -o $@

$(EXECUTABLE_MANAGEMENT_CLIENT): $(OBJECTS_MANAGEMENT_CLIENT_FILES)
	$(CC) $(CFLAGS) $^ -o $@

test: cpp scanbuild #complexity 

cpp: $(SOURCES_CPP)

%.cpp: %.c
	cppcheck --quiet --enable=all --force --inconclusive $< 2>> results.cppOut

scanbuild: 
	CC=gcc scan-build -disable-checker deadcode.DeadStores -o scanBuildResults make >  results.sb

#complexity:
#	complexity --histogram --score $(SOURCES_C) >results.cpxt 2> /dev/null

clean:
	rm -rf $(OBJECTS_FILES) $(EXECUTABLE_SERVER) $(OBJECTS_MANAGEMENT_CLIENT_FILES) $(EXECUTABLE_MANAGEMENT_CLIENT)

cleanTest:
	rm -rf results.cppOut results.sb scanBuildResults 

#results.cpxt 

.PHONY: all clean cleanTest cpp pvs test
