BASEDIR ?= $(abspath .)

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

BINARIES = vernamfs

TESTS = base64Tests numParseTests

TOOLS = headerInfo

VPATH = $(BASEDIR)/src/main/c

VPATH += $(BASEDIR)/src/test/c

#CPPFLAGS += -DFUSE_USE_VERSION=25
CPPFLAGS += -DFUSE_USE_VERSION=25

CPPFLAGS += -D_FILE_OFFSET_BITS=64

CPPFLAGS += -I$(BASEDIR)/src/main/include

LDLIBS += -lm

CFLAGS = -Wall -g 
#CFLAGS += -ansi
#CFLAGS += -std=c99

default: $(BINARIES)

test: $(TESTS)

vernamfs: main.o vernamfs.o fuse.o mount.o init.o info.o \
	remote.o rls.o vls.o rcat.o vcat.o
	$(CC) $^ $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

$(TESTS) $(TOOLS): % : %.o
	$(CC) $^ $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

$(BASEDIR)/src/main/include/version.h : $(BASEDIR)/Makefile
	@echo "#define MAJOR_VERSION" $(MAJOR_VERSION) \
	| tee $(BASEDIR)/src/main/include/version.h
	@echo "#define MINOR_VERSION" $(MINOR_VERSION) \
	| tee -a $(BASEDIR)/src/main/include/version.h
	@echo "#define PATCH_VERSION" $(PATCH_VERSION) \
	| tee -a $(BASEDIR)/src/main/include/version.h

.PHONY: ver.h
ver.h : $(BASEDIR)/src/main/include/version.h

.PHONY: clean
clean:
	rm $(BINARIES) $(TESTS) *.o

# eof
