#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"
#include "remote.h"

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

char* rcatUsage = "rcat OTPREMOTE offset length";

int rcatArgs( int argc, char* argv[] ) {

  if( argc < 4 ) {
	fprintf( stderr, "Usage: %s\n", rcatUsage );
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
