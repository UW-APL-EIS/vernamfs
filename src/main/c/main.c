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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fuse.h>

#include "vernamfs/cmds.h"
#include "vernamfs/vernamfs.h"

/**
 * @author Stuart Maclean
 *
 * Entry point into our Vernam FileSystem (VFS).  A VFS is a
 * fuse-based write-only file system where the persistent storage (the
 * 'backing') is file/device whose initial contents are a one-time-pad
 * (OTP).  
 * 
 * For any write operation, we READ the OTP data, XOR in our
 * data, and WRITE out the result to the backing.  
 * 
 * We talk of the 'remote' unit and the 'vault' unit.  The remote is
 * where we are storing data we don't want others to read should they
 * capture the remote, and the vault is the local, assumed safe, copy
 * of the OTP.  Ideally we can interrogate the remote while it is
 * remote, and upload data accordingly.
 *
 * Our VFS actually comprises several components, all available
 * through this single cmd line entry point (think how git works, sole
 * executable is git, which then switches off first cmd line arg).
 *
 * Our 'command set' is currently:
 *
 * MOUNT
 * -----
 * mount - The primary feature, mount an OTP device for file write operations, 
 * e.g.
 *
 * remote$ mkdir foo
 * remote$ vernamfs mount /path/to/my/otp foo
 * 
 * OR, with arbitrary FUSE options, e.g.
 * 
 * remote$ vernamfs mount /path/to/my/otp -d foo
 *
 * Remote users can then write to this mountpoint:
 *
 * remote$ cp myFile foo/
 * remote$ mv myFile foo/
 * remote$ seq 1000 > foo/seq.1000
 * remote$ echo HelloWorld > foo/msg
 * 
 * To umount, use the regular fusermount -u command:
 *
 * remote$ fusermount -u foo
 *
 * NB: Single-threaded FUSE operation is REQUIRED, and is set
 * internally.  Our VFS cannot work in a multi-threaded FUSE loop.
 * All access must be serialized.
 *
 * INIT
 * ----
 *
 * init - Initialise a new file/device with a VFS.  Writes the VFS
 * header at the start of the file/device.  Size of the VFS is taken
 * from the size of the file/device.  User supplies a 'tableSize', which
 * identifies the maximum number of files that the VFS can hold.  
 * Can be done in any location (remote or vault), e.g.
 * 
 * $ vernamfs init /path/to/my/otp 1024
 *
 * INFO
 * ----
 * info - Print the metadata about a stored VFS.  Essentially prints
 * the 'VFSHeader' info, stored at offset 0 in backing store.  Can be
 * done in any location (remote or vault), e.g.
 * 
 * $ vernamfs info /path/to/my/otp
 *
 * RLS
 * --
 *
 * rls - Cat to stdout the (still xor'ed) VFS file metadata table.
 * Works on the remote OTP.  Prints to stdout, so may need
 * re-direction for capture:
 * 
 * remote$ vernam rls /path/to/my/otp > ls.cap
 *
 * VLS
 * ---
 *
 * vls - Uses output of rls and the pristine 'vault' OTP to recover
 * actual remote table metadata, which it prints to stdout.  Works on
 * the vault unit:
 *
 * vault$ vernam rls /path/to/my/otpCopy < ls.cap
 */

VFS Global;

int main( int argc, char* argv[] ) {

  cmds = (Command**)calloc( 32, sizeof( Command* ) );
  N = 0;
  
  cmds[N++] = &helpCmd;

  cmds[N++] = &generateCmd;

  cmds[N++] = &initCmd;

  cmds[N++] = &infoCmd;

  cmds[N++] = &mountCmd;
  
  cmds[N++] = &rlsCmd;

  cmds[N++] = &vlsCmd;

  cmds[N++] = &rcatCmd;

  cmds[N++] = &vcatCmd;
  
  cmds[N++] = &recoverCmd;

  cmds[N] = NULL;
  
  if( argc < 2 ) {
	char usage[1024];
	commandsSummary( usage );
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  char* name = argv[1];
  Command* c = commandLocate( name );
  if( c ) {
	if( strcmp( c->name, "mount" ) == 0 ) {
	  /*
		NOTE: fuse_main wants to see the REAL argc, argv.  It seems
		to need/use argv[0], the invoked program name
	  */
	  (c->invoke)( argc, argv );
	} else {
	  (c->invoke)( argc-1, argv+1 );
	}
	return 0;
  } else {
	fprintf( stderr, 
			 "%s: '%s' is not a vernamfs command. See 'vernamfs help'.\n",
			 ProgramName, name );
	return -1;
  }
}

// eof

