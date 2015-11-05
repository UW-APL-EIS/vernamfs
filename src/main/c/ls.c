#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"

/**
 * @author Stuart Maclean
 *
 * The 'ls' command enables us to print the VernamFS 'Table', which
 * contains metadata about each file written to the FS.  The table as
 * stored on the 'remote' unit is actually XOR'ed with the OTP, so is
 * unreadable.  However, we can cat it, so can use that cat result
 * later with a copy of the original OTP to recover the actual table.
 *
 * Example usage:
 *
 * 1: On the 'remote unit': $ vernamfs ls FILE > ls.remote
 *
 * 2: Transport the ls.remote data to the 'vault location', where we
 * have a copy of the original OTP. 
 *
 * 3: Recover the actual listing: $ vernamfs xls VAULTFILE < ls.remote
 *
 * In lab testing, the remote and vault locations are likely the same,
 * so
 *
 * $ vernamfs ls REMOTE.OTP | vernamfs xls VAULT.OTP
 *
 * @see xls.c
 */

int lsArgs( int argc, char* argv[] ) {

  char* usage = "Usage: ls OTPFILE";

  if( argc < 1 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  return lsFile( argv[0] );
}

int lsFile( char* file ) {

  struct stat st;
  int sc = stat( file, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", file );
	return -1;
  }
  size_t length = st.st_size;

  int fd = open( file, O_RDONLY );
  if( fd < 0 ) {
	perror( "open" );
	return -1;
  }
  
  void* addr = mmap( NULL, length, PROT_READ, MAP_PRIVATE, fd, 0 );
  if( addr == MAP_FAILED ) {
	perror( "mmap" );
	close( fd );
	return -1;
  }

  VFS vfs;
  VFSLoad( &vfs, addr );

  /*
	We write ALL data up to the current tablePtr, so get the header
	too.  This will be required for later processing of this data.
  */
  write( 1, addr, vfs.header.tablePtr );

  munmap( addr, length );
  close( fd );

  return 0;
}

// eof
