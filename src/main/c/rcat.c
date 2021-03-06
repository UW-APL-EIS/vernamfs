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

#include "vernamfs/cmds.h"
#include "vernamfs/vernamfs.h"
#include "vernamfs/remote.h"

/**
 * @author Stuart Maclean
 *
 * The 'rcat' command enables us to print the contents of a
 * still-encrypted file.  It is NOT available by name, only by offset.
 * So the inputs to rcat are just an offset and a byte count (length).
 *
 * Example usage:
 *
 * 1: On the 'remote unit': $ vernamfs rls OTP > ls.remote
 *
 * 2: Transport the ls.remote data to the 'vault location', where we
 * have a copy of the original OTP. 
 *
 * 3: Recover the actual listing: $ vernamfs vls VAULTFILE ls.remote
 *
 * 4: From the vls listing, select a suitable offset O and length L, based
 * on some file name of interest, then rcat that:
 *
 * $ On the remote unit: $ vernamfs rcat O F > cat.remote
 *
 * rcat accepts the offset and length in either hexadecimal or decimal form.
 *
 * @see vcat.c
 */

static int hexOrDecimal( char* s ) {
  if( strlen( s ) >= 2 && s[0] == '0' && (s[1] == 'X' || s[1] == 'x' ) ) {
	return strtol( s, NULL, 16 );
  }
  return atoi( s );
}

static char example1[] = 
  "remote$ vernamfs rls OTP.remote > remote.ls";
static char example2[] = 
  "vault$  vernamfs vls OTP.vault remote.ls\n"
  "    /foo 0x1234 0x5678";
static char example3[] = 
  "remote$ vernamfs rcat OTP.remote 0x1234 0x5678 > remote.cat";
static char example4[] = 
  "vault$  vernamfs vcat OTP.vault remote.cat";
static char example5[] = 
  "vault$  vernamfs vcat OTP.vault remote.cat remote.ls";

static char* examples[] = { example1, example2, example3, 
							example4, example5, NULL };

static CommandHelp help = {
  .summary = "Cat section of a remote VernamFS",
  .synopsis = "OTPREMOTE offset length",
  .description = "Extracts a byte sequence from the OTP on the remote unit.\n  Result is encrypted, but can be presumably be brought back to the vault,\n  where it can be decrypted with the OTP vault copy.\n  To derive the offset and length, first run rls, vls. If the rls result\n  is passed to vcat, the recovered content is written to the identified\n  file, else to stdout.",
  .examples = examples
};

Command rcatCmd = {
  .name = "rcat",
  .help = &help,
  .invoke = rcatArgs
};

int rcatArgs( int argc, char* argv[] ) {

  if( argc < 4 ) {
	commandHelp( &rcatCmd );
	return -1;
  }

  char* file = argv[1];
  uint64_t offset = hexOrDecimal( argv[2] );
  uint64_t length = hexOrDecimal( argv[3] );

  return rcat( file, offset, length );
}

int rcat( char* file, uint64_t offset, uint64_t length ) {

  struct stat st;
  int sc = stat( file, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", file );
	return -1;
  }
  size_t mappedLength = st.st_size;

  int fd = open( file, O_RDONLY );
  if( fd < 0 ) {
	perror( "open" );
	return -1;
  }
  
  void* addr = mmap( NULL, mappedLength, PROT_READ, MAP_PRIVATE, fd, 0 );
  if( addr == MAP_FAILED ) {
	perror( "mmap" );
	close( fd );
	return -1;
  }

  /*
	We write a 'Remote Result', which is a triple.  Values for a file
	content 'cat' are

	1: offset in whole VFS, i.e. an echo of input offset

	2: length of rcat data, i.e. an echo on input length

	3: the data
  */

  VFSRemoteResult vrr;
  vrr.offset = offset;
  vrr.length = length;
  vrr.data   = addr + offset;

  // Set this for completeness, we are NOT calling RemoteResultFree anyway
  vrr.dataOnHeap = 0;

  VFSRemoteResultWrite( &vrr, STDOUT_FILENO );

  munmap( addr, length );
  close( fd );

  return 0;
}

// eof
