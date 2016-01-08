# VernamFS uses Fuse (Filesystem in Userspace). You need the Fuse
# Development headers installed for VernamFS to build.  Essentially,
# you need a 'fuse.h'.  Likely it's at /usr/include/fuse.h.  If the
# build fails like this:
#
# No package 'fuse' found
# /path/to/vernamfs/src/main/c/fuse.c:6:18: fatal error: fuse.h: No such file or# directory
# #include <fuse.h>
#                  ^
# then install the fuse development library, as easy as:
#
# On Debian 8: # apt-get install libfuse-dev
#
# On Ubuntu:   # apt-get install libfuse-dev
#
#
# IMPORTANT!!  In developing VernamFS, our goal was for it run on both
# x86/amd64 (Intel workstations/laptops) AND on Arm processors.  We
# thus split the make procedure into 'targets'.  Each target is
# contained in its own subdirectory.  Currently there are 2 target
# platforms:
#
# ./target/native
# ./target/arm-linux
#
# Most likely you want to build VernamFS in order to run it on the
# same platform you build on. We call that a native build, so do this:

# cd target/native
# make

# Do NOT invoke make from this directory (the one containing the
# Makefile you are reading).  You will get the error below.

ifndef BASEDIR
$(error Likely you are running make from where the main Makefile lives.  cd into target/native and run make from there)
endif

# There are sub-Makefiles in each target directory which DO define BASEDIR
MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

VERSION = $(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_VERSION)

BINARIES = vernamfs

TESTS = base64Tests numParseTests deviceSizeTest

TOOLS = headerInfo

MAINSRCDIR = $(BASEDIR)/src/main/c

# Exclude from the srcs list any Emacs tmp files starting '#'
MAINSRCS = $(shell cd $(MAINSRCDIR) && ls *.c)

MAINOBJS = $(MAINSRCS:.c=.o)

TESTSRCDIR = $(BASEDIR)/src/test/c

VPATH = $(MAINSRCDIR) $(TESTSRCDIR)

CPPFLAGS += -D_FILE_OFFSET_BITS=64

# Use 25 here simply because 2.5.3 is the latest FUSE distro that will
# build on our target arm-linux platform. On x86, later versions may
# work.
CPPFLAGS += -DFUSE_USE_VERSION=25

CPPFLAGS += -I$(BASEDIR)/src/main/include

LDLIBS += -lm

CFLAGS = -Wall -Werror 
#CFLAGS += -ansi
#CFLAGS += -std=c99

default: $(BINARIES)

test: $(TESTS)

vernamfs: $(MAINOBJS)
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

.PHONY: clean
clean:
	rm $(BINARIES) $(TESTS) *.o

.PHONY: zip
zip:
	git archive -o vernamfs-$(VERSION).zip --prefix vernamfs/ HEAD

vernamfs.o : vernamfs.c $(BASEDIR)/src/main/include/version.h

# eof
