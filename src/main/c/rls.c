#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"
#include "remote.h"

/**
 * @author Stuart Maclean
 *
 * The 'rls' command enables us to print the VernamFS 'Table', which
 * contains metadata about each file written to the FS.  The table as
 * stored on the 'remote' unit is actually XOR'ed with the OTP, so is
 * unreadable.  However, we can cat it, so can use that cat result
 * later with a copy of the original OTP to recover the actual table.
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
 * In lab testing, the remote and vault locations are likely the same,
 * so
 *
 * $ vernamfs rls REMOTE.OTP | vernamfs vls VAULT.OTP
 *
 * @see vls.c
 */

int rlsArgs( int argc, char* argv[] ) {

  char* usage = "Usage: rls OTPREMOTE";

  if( argc < 1 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  return rlsFile( argv[0] );
}

int rlsFile( char* file ) {

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
	We write a 'Remote Result', which is a triple.  Values for an 'ls'
	listing are

	1: table offset in whole VFS

	2: table length in bytes

	3: table entries, as N VFSTableEntry structs
  */

  VFSRemoteResult vrr;
  VFSHeader* h = &vfs.header;
  vrr.offset = h->tableOffset;
  uint64_t tableExtent = h->tablePtr - h->tableOffset;
  vrr.length = tableExtent;
  vrr.data = addr + h->tableOffset;

  // set this for completeness, we are NOT calling RemoteResultFree anyway
  vrr.dataOnHeap = 0;

  VFSRemoteResultWrite( &vrr, 1 );

  munmap( addr, length );
  close( fd );

  return 0;
}

// eof
