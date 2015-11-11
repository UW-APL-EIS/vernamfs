CC = gcc

CFLAGS += -g

CPPFLAGS += `pkg-config --cflags fuse`

LOADLIBES += `pkg-config --libs fuse`

BASEDIR = $(abspath ../..)
include $(BASEDIR)/Makefile

# eof
