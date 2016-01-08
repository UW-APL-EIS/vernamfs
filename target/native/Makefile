CC = gcc

CFLAGS += -g

# If you DO have fuse.h and libfuse.a (.so) but DON'T have pkg-config,
# replace these with 'manual' paths, e.g.:

# For CPPFLAGS:  -I/path/to/my/fuseheaders
# For LOADLIBES: -L/path/to/my/fuselibs -lfuse

CPPFLAGS  += `pkg-config --cflags fuse`

LOADLIBES += `pkg-config --libs fuse`

BASEDIR = $(abspath ../..)
include $(BASEDIR)/Makefile

# eof
