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
#include <unistd.h>
#include <sys/stat.h>

#include "vernamfs/cmds.h"
#include "vernamfs/vernamfs.h"

/**
 * @author Stuart Maclean
 *
 * Write a VFS Header structure out to a file/device.  Maximum file
 * count needed must be supplied.  Also, a -l N option can provide a
 * maximum file name length.

 * Can then view that header with e.g. xxd/od:
 *
 * $ vernamfs init FILE 1024
 *
 * $ vernamfs init -l 30 FILE 1024
 *
 * $ xxd -l 128 FILE
 *
 * Will only overwrite any existing header that may have been in place
 * if force flag supplied, via -f option:
 *
 * $ vernamfs init -l 30 FILE 1024
 * $ vernamfs init -l 30 -f FILE 1024
 */

static CommandOption f = 
  { .id = "f", 
	.text = "Force initialization even when existing VernamFS "
	"already present.\n    May lose data!" };

static CommandOption l = 
  { .id = "l", 
	.text = "Maximum length of file name.  Defaults to 64-17=47. Minimum is 32-17=15.\n    Maximum is 128-17=111." };

static CommandOption e = 
  { .id = "e", 
	.text = "Expert mode.  Prints out entire VFS header." };

static CommandOption* options[] = { &e, &f, &l, NULL };

static char example1[] = 
  "$ dd if=/dev/urandom bs=1M count=1024 of=OTP.1GB";

static char example2[] = "$ vernamfs init OTP.1GB 128";

static char example3[] = "$ vernamfs init -f -l 128 OTP.1GB 1024";

static char* examples[] = { example1, example2, example3, NULL };

static CommandHelp help = {
  .summary = "Initialise a one-time pad file with a VernamFS header",
  .synopsis = "[<options>] OTPFile maxFileCount",
  .description = "Initialise a VernamFS, by writing a header at the start of the supplied\n  OTPFile.  The maximum number of files that the VernamFS is expected to hold \n  must be supplied at init time.",
  .options = options,
  .examples = examples
};

Command initCmd = {
  .name = "init",
  .help = &help,
  .invoke = initArgs,
};

int initArgs( int argc, char* argv[] ) {

  int expert = 0;
  int force = 0;
  int maxFileNameLength = VERNAMFS_NAMELENGTHDEFAULT;
  char* file = NULL;
  int maxFiles = 0;

  int c;
  while( (c = getopt( argc, argv, "efl:") ) != -1 ) {
	switch( c ) {
	case 'e':
	  expert = 1;
	  break;
	case 'f':
	  force = 1;
	  break;
	case 'l':
	  maxFileNameLength = atoi( optarg );
	  break;
	default:
	  break;
	}
  }

  if( optind+2 > argc ) {
	commandHelp( &initCmd );
	return -1;
  }
  file = argv[optind];
  maxFiles = atoi( argv[optind+1] );

  if( maxFiles < 1 ) {
	fprintf( stderr, "%s: Max file count %d too small.\n", argv[0], maxFiles );
	return -1;
  }

  return init( file, maxFiles, maxFileNameLength, force, expert );
}

int init( char* file, int maxFiles, int maxFileNameLength,
		  int force, int expert ) {

  struct stat st;
  int sc = stat( file, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file.\n", file );
	return -1;
  }
  
  VFS vfs;
  sc = VFSInit( &vfs, st.st_size, maxFiles, maxFileNameLength );
  if( sc ) {
	fprintf( stderr, "%s:  Device too small.\n", file );
	return sc;
  }

  int fd = open( file, O_RDWR );
  uint64_t b8 = 0;
  int nin = read( fd, &b8, sizeof( b8 ) );
  if( nin != sizeof( b8 ) ) {
	perror( "init.read" );
	close( fd );
	return -1;
  }
  if( b8 == VERNAMFS_MAGIC && !force ) {
	fprintf( stderr, "%s: Already contains a VFS. Use -f to force init.\n", 
			 file );
	return -1;
  }
  
  lseek( fd, 0, SEEK_SET );
  int len = sizeof( VFSHeader );
  int nout = write( fd, &vfs.header, len );
  if( nout != len ) {
	perror( "init.write" );
	close( fd );
	return -1;
  }

  VFSReport( &vfs, expert );

  close( fd );
  return 0;
}

// eof
