BASEDIR ?= $(abspath .)

BINARIES = onetimepadfs

TESTS = mmapTest headerTest

VPATH = $(BASEDIR)/src/main/c

VPATH += $(BASEDIR)/src/test/c

CPPFLAGS += -I$(BASEDIR)/src/main/include

headerTest: header.o

# eof
