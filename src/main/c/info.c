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
#include <unistd.h>
#include <sys/stat.h>

#include "vernamfs/cmds.h"
#include "vernamfs/vernamfs.h"

static CommandOption e = 
  { .id = "e", 
	.text = "Expert mode.  Prints out entire VFS header." };

static CommandOption* options[] = { &e, NULL };

static char example1[] = 
  "$ vernamfs generate -z 16 > 64K.pad";

static char example2[] = "$ vernamfs info 64K.pad";

static char example3[] = "$ vernamfs info -e 64K.pad";

static char* examples[] = { example1, example2, example3, NULL };

static CommandHelp help = {
  .summary = "Print info about a VernamFS device/file",
  .synopsis = "[<options>] OTPFILE",
  .description = "Prints details of a Vernam Filesystem, including files used,\n  maximum files allowed, data area used, etc",
  .options = options,
  .examples = examples,
};

Command infoCmd = {
  .name = "info",
  .help = &help,
  .invoke = infoArgs
};

int infoArgs( int argc, char* argv[] ) {

  int expert = 0;

  int c;
  while( (c = getopt( argc, argv, "e") ) != -1 ) {
	switch( c ) {
	case 'e':
	  expert = 1;
	  break;
	default:
	  break;
	}
  }

  if( optind+1 > argc ) {
	commandHelp( &infoCmd );
	return -1;
  }
  char* file = argv[optind];
  
  return info( file, expert );
}

int info( char* file, int expert ) {

  struct stat st;
  int sc = stat( file, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", file );
	return -1;
  }

  if( st.st_size < sizeof( VFSHeader ) ) {
	fprintf( stderr, "%s: Too small to contain header\n", file );
	return -1;
  }
  
  VFS vfs;
  // NOT using mmapped-based VFSLoad, so set backing accordingly
  vfs.backing = 0;

  VFSHeader* h = &vfs.header;
  int fd = open( file, O_RDONLY );
  int nin = read( fd, h, sizeof( VFSHeader ) );
  if( nin != sizeof( VFSHeader ) ){
	perror( "infoFile.read" );
	close( fd );
	return -1;
  }

  if( h->magic != VERNAMFS_MAGIC ) {
	fprintf( stderr, "%s: Invalid magic number\n", file );
  } else {
	VFSReport( &vfs, expert );
  }

  close( fd );
  return 0;
}

// eof
