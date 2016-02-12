# Building VernamFS for the arm-linux platform, as used at APL/UW.
# Requires libfuse compiled for arm-linux also.
#
# APL/UW has libfuse cross-compiled for the arm-linux target,
# installed under our local 'maken' build infrastructure.  This build
# is thus highly-specific to APL/UW.  
#
# You probably want to be building VernamFS for x86/amd64, see
# ../Makefile.
#

PLATFORM = arm-linux

CC = arm-linux-gcc

MAKEN_HOME ?= $(HOME)/.maken

MAKEN_REPO = $(MAKEN_HOME)/repository

FUSE_HOME = $(MAKEN_REPO)/fuse/fuse/2.5.3/$(PLATFORM)

CPPFLAGS += -I$(FUSE_HOME)/usr/local/include

ifdef FUSE_STATIC
LOADLIBES += $(FUSE_HOME)/usr/local/lib/libfuse.a
LDLIBS += -lpthread
else
LOADLIBES += -L$(FUSE_HOME)/usr/local/lib -lfuse
endif

BASEDIR = $(abspath ..)
include $(BASEDIR)/Makefile

# eof
