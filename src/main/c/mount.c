/**
 * Copyright © 2016, University of Washington
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of the University of Washington nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL UNIVERSITY OF
 * WASHINGTON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <fuse.h>

#include "vernamfs/cmds.h"
#include "vernamfs/vernamfs.h"

static CommandOption f = { .id = "f", .text = "Fuse mount in foreground." };
static CommandOption d = { .id = "d", .text = "Fuse mount in debug mode." };

static CommandOption* options[] = { &f, &d, NULL };

static char example1[] = 
  "$ dd if=/dev/urandom bs=1M count=1 of=OTP.1GB; mkdir mnt";

static char example2[] = "$ vernamfs mount OTP.1GB mnt";

static char example3[] = "$ vernamfs mount OTP.1GB mnt -f";

static char example4[] = "$ cp myFile mnt/; echo foobar > mnt/foo";

static char example5[] = "$ fusermount -u mnt";

static char* examples[] = { example1, example2, example3, 
							example4, example5, NULL };


static CommandHelp help = {
  .summary = "Mount a VernamFS device/file",
  .synopsis = "OTPFile mountPoint [<fuseOptions>]",
  .description = "Mount a mountPoint, with a one-time pad file as the underlying storage.\n  The mount uses FUSE, in single-threaded mode. Any data written to the mount\n  point is encrypted via XOR'ing with the pad contents.  The filesystem is\n  write-only!",
  .options = options,
  .examples = examples
};

Command mountCmd = {
  .name = "mount",
  .help = &help,
  .invoke = mountArgs
};

// argc, argv straight from main, NOT shifted, since fuse_main needs argv[0] ?
int mountArgs( int argc, char* argv[] ) {

  /*
	Must be 4+, since (1) progName, (2) 'mount', (3) our OTP file and 
	(4) a fuse mount point. 5+ would be any fuseOptions
  */
  if( argc < 4 ) {
	commandHelp( &mountCmd );
	return -1;
  }

  char* file = argv[2];
  struct stat st;
  int sc = stat( file, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", file );
	return -1;
  }
  size_t length = st.st_size;

  int fd = open( file, O_RDWR );
  if( fd < 0 ) {
	fprintf( stderr, "%s: Not read/writable\n", file );
	return -1;
  }
  
  void* addr = mmap( NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
  if( addr == MAP_FAILED ) {
	fprintf( stderr, "%s: MMap failed\n", file );
	close( fd );
	return -1;
  }
  
  VFSLoad( &Global, addr );

  // If this backing file/device not VFS-initialized, bail
  if( Global.header.magic != VERNAMFS_MAGIC ) {
	fprintf( stderr, 
			 "Magic number missing. Initialize with 'vernamfs init %s'.\n", 
			 file );
	munmap( addr, length );
	close( fd );
	return -1;
  }

  VFSReport( &Global, 1 );

  /*
	Re-org the command line so that fuse_main doesn't see our 'mount'
	subcommand literal nor our OPTFILE.  Given that we MUST run the
	single-threaded fuse loop, overwrite our argv[1] with the '-s'
	option that forces single-threadedness, then shift all other args
	(including the NULL) down one place, which eliminates our OPTFILE
	from the args list.

	Note how we are preserving argv[0].  Note quite sure WHY we need
	to do this, but if we don't, Fuse does NOT work and we get left
	with un-unmountable broken mount points!  Fuse is using some
	property of argv[0] for sure.
  */
  argv[1] = "-s";
  int i;
  for( i = 2; i < argc; i++ )
	argv[i] = argv[i+1];

  return fuse_main( argc-1, argv, &vernamfs_ops );
}

// eof
