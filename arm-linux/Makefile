# APL/UW has libfuse cross-compiled for the arm-linux target,
# installed under our local 'maken' build infrastructure.  This build
# is thus highly-specific to APL/UW.  You probably want to be running
# ../Makefile.

PLATFORM = arm-linux

CC = arm-linux-gcc

MAKEN_HOME ?= $(HOME)/.maken

MAKEN_REPO = $(MAKEN_HOME)/repository

FUSE_HOME = $(MAKEN_REPO)/fuse/fuse/2.5.3/

CPPFLAGS += -I$(FUSE_HOME)/$(PLATFORM)/usr/local/include

ifdef FUSE_STATIC
LOADLIBES += $(FUSE_HOME)/$(PLATFORM)/usr/local/lib/libfuse.a
LDLIBS += -lpthread
else
LOADLIBES += -L$(FUSE_HOME)/$(PLATFORM)/usr/local/lib -lfuse
endif

BASEDIR = $(abspath ..)
include $(BASEDIR)/Makefile

# eof
