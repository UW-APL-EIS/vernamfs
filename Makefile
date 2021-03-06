#
# Copyright © 2016, University of Washington
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#
#     * Neither the name of the University of Washington nor the names
#       of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL UNIVERSITY OF
# WASHINGTON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

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
#
# Most likely you want to build VernamFS in order to run it on the
# same platform you build on. We call that a native build, so do this:

# make

# We have also builds for other targets, i.e cross-compiles for other
# platforms, currently just for arm-linux.  See ./arm-linux/Makefile
# for details.

BASEDIR ?= $(abspath .)

# Public tagged release may be later than version defined here,
# in which case the later tag is purely for documentation purposes.
MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

VERSION = $(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_VERSION)

BINARIES = vernamfs

TESTS = base64Tests numParseTests deviceSizeTest inUseTest

TOOLS = headerInfo

MAINSRCDIR = $(BASEDIR)/src/main/c

MAINSRCS = $(shell cd $(MAINSRCDIR) && ls *.c)

MAINOBJS = $(MAINSRCS:.c=.o)

TESTSRCDIR = $(BASEDIR)/src/test/c

VPATH = $(MAINSRCDIR) $(TESTSRCDIR)

CC ?= gcc

CPPFLAGS ?= `pkg-config --cflags fuse`

CPPFLAGS += -D_FILE_OFFSET_BITS=64

LOADLIBES ?= `pkg-config --libs fuse`

# Used version 25 here simply because 2.5.3 is the latest FUSE distro
# that will build on our target arm-linux platform. On x86, later
# versions may work.
CPPFLAGS += -DFUSE_USE_VERSION=25

CPPFLAGS += -I$(BASEDIR)/src/main/include/

LDLIBS += -lm

CFLAGS ?= -Wall -Werror
#CFLAGS ?= -std=c99 -Wall
#CFLAGS = -ansi -Wall
#CFLAGS += -ansi
#CFLAGS +=  -std=c99

CFLAGS += -g

default: $(BINARIES)

test: $(TESTS)

vernamfs: $(MAINOBJS)
	$(CC) $^ $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

$(TESTS) $(TOOLS): % : %.o
	$(CC) $^ $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

$(BASEDIR)/src/main/include/vernamfs/version.h : $(BASEDIR)/Makefile
	@echo "#define MAJOR_VERSION" $(MAJOR_VERSION) | tee $@
	@echo "#define MINOR_VERSION" $(MINOR_VERSION) | tee -a $@
	@echo "#define PATCH_VERSION" $(PATCH_VERSION) | tee -a $@

.PHONY: clean
clean:
	rm $(BINARIES) $(TESTS) *.o \
	$(BASEDIR)/src/main/include/vernamfs/version.h

.PHONY: zip
zip:
	git archive -o vernamfs-$(VERSION).zip \
	--prefix vernamfs-$(VERSION)/ HEAD

vernamfs.o : vernamfs.c $(BASEDIR)/src/main/include/vernamfs/version.h

# eof
