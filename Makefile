BASEDIR ?= $(abspath .)

BINARIES = vernamfs

TESTS = mmapTest headerTest 

TOOLS = headerInfo

VPATH = $(BASEDIR)/src/main/c

VPATH += $(BASEDIR)/src/test/c

CPPFLAGS += `pkg-config --cflags fuse`

CPPFLAGS += -DFUSE_USE_VERSION=25

CPPFLAGS += -I$(BASEDIR)/src/main/include

LOADLIBES += `pkg-config --libs fuse`

LDLIBS += -lm

CFLAGS = -Wall -g
#CFLAGS += -ansi
#CFLAGS += -std=c99

default: vernamfs

vernamfs: main.o header.o mmap.o fuse.o
	$(CC) $^ $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

$(TESTS) $(TOOLS): headerInfo.o header.o mmap.o
	$(CC) $^ $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

.PHONY: clean
clean:
	rm $(BINARIES) *.o

# eof
