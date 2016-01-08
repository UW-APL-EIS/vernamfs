# APL/UW has libfuse cross-compiled for Arm, installed under our local
# 'maken' build infrastructure.  This build is thus highly-specific to
# APL/UW.  You probably want to be in ../native.

CC = arm-linux-gcc

MAKEN_HOME = $(HOME)/.maken/repository

FUSE_HOME = $(MAKEN_HOME)/fuse/fuse/2.5.3/

CPPFLAGS += -I$(FUSE_HOME)/arm-linux/usr/local/include

ifdef FUSE_STATIC
LOADLIBES += $(FUSE_HOME)/arm-linux/usr/local/lib/libfuse.a
LDLIBS += -lpthread
else
LOADLIBES += -L$(FUSE_HOME)/arm-linux/usr/local/lib -lfuse
endif

BASEDIR = $(abspath ../..)
include $(BASEDIR)/Makefile

# eof
