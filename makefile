include makefile.inc
include source_dirs.inc

CFLAGS+=-I$(CURDIR)

SOURCES_SERVER=httpd.c
EXECUTABLE_SERVER=httpd

EXECUTABLE_MANAGEMENT_CLIENT=httpdctl

SOURCES_MANAGEMENT_CLIENT_FILES=$(foreach dir,$(MANAGEMENT_CLIENT_DIRS),$(wildcard $(dir)/*.c))
OBJECTS_MANAGEMENT_CLIENT_FILES=$(SOURCES_MANAGEMENT_CLIENT_FILES:.c=.o)

SOURCES_FILES=$(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJECTS_FILES=$(SOURCES_FILES:.c=.o)

SOURCES_ALL=$(SOURCES_SERVER)
SOURCES_ALL+=$(SOURCES_MANAGEMENT_CLIENT_FILES)
SOURCES_ALL+=$(SOURCES_FILES)

SOURCES_CPP=$(SOURCES_ALL:.c=.cppcheck)


#MAIN DIRECTIVES
all: $(EXECUTABLE_SERVER) $(EXECUTABLE_MANAGEMENT_CLIENT)

$(EXECUTABLE_SERVER): $(OBJECTS_FILES)
	$(CC) $(CFLAGS) $(SOURCES_SERVER) $^ -o $@

$(EXECUTABLE_MANAGEMENT_CLIENT): $(OBJECTS_MANAGEMENT_CLIENT_FILES)
	$(CC) $(CFLAGS) $^ -o $@


#TEST DIRECTIVES
test: testDir scanbuild cppcheck complexity

testDir: clean cleanTest
	@mkdir test_results

cppcheck: $(SOURCES_CPP)

%.cppcheck: %.c
	cppcheck --quiet --enable=all --force --suppress=unusedFunction:* --suppress=missingInclude:* --inconclusive $< 2>> test_results/results.cppcheck

scanbuild: 
	CC=gcc scan-build -disable-checker deadcode.DeadStores -o test_results/scanbuild_results make > test_results/results.sb

complexity:
	complexity --histogram --score --thresh=3 $(SOURCES_ALL) > test_results/results.complexity 2> /dev/null

#CLEAN DIRECTIVES

clean:
	rm -rf $(OBJECTS_FILES) $(EXECUTABLE_SERVER) $(OBJECTS_MANAGEMENT_CLIENT_FILES) $(EXECUTABLE_MANAGEMENT_CLIENT)

cleanTest: clean
	rm -rf test_results

.PHONY: all clean cleanTest test testDir cppcheck scanbuild
