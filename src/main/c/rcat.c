#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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
 * 1: On the 'remote unit': $ vernamfs rls FILE > ls.remote
 *
 * 2: Transport the ls.remote data to the 'vault location', where we
 * have a copy of the original OTP. 
 *
 * 3: Recover the actual listing: $ vernamfs vls VAULTFILE < ls.remote
 *
 * 4: From the vls listing, select a suitable offset and length, based
 * on some file name of interest, then rcat that:

 * $ On the remote unit: $ vernamfs rcat FILE > cat.remote
 *
 * @see vcat.c
 */

int rcatArgs( int argc, char* argv[] ) {

  char* usage = "Usage: rcat REMOTEOTP offsetHex lengthHex";

  if( argc < 3 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  char* file = argv[0];
  uint64_t offset = strtol( argv[1], NULL, 16 );
  uint64_t length = strtol( argv[2], NULL, 16 );

  return rcatFile( file, offset, length );
}

int rcatFile( char* file, uint64_t offset, uint64_t length ) {

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
  vrr.data = addr + offset;

  // set this for completeness, we are NOT calling RemoteResultFree anyway
  vrr.dataOnHeap = 0;

  VFSRemoteResultWrite( &vrr, STDOUT_FILENO );

  munmap( addr, length );
  close( fd );

  return 0;
}

// eof
