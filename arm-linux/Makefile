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
BASEDIR = ..
include $(BASEDIR)/Makefile

# eof
