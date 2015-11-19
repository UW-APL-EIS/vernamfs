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
 * later with a copy of the original OTP (the 'vault' copy) to recover
 * the actual table.  The rls result goes to stdout, as would a
 * regular Unix ls result.
 *
 * Example usage:
 *
 * 1: On the 'remote unit': $ vernamfs rls OTP > ls.remote
 *
 * 2: Transport the ls.remote data to the 'vault location', where we
 * have a copy of the original OTP. 
 *
 * 3: Recover the actual listing: $ vernamfs vls OTPVAULT ls.remote
 *
 * The ls.remote result can also be supplied on stdin.  This is handy
 * for lab testing, the remote and vault locations are likely the
 * same, so
 *
 * $ vernamfs rls OTPREMOTE | vernamfs vls OTPVAULT
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
  
  // LOOK: check magic number, are we actually loading a VFS file?

  VFSHeader* h = &vfs.header;

  /*
	We write a 'Remote Result', which is a triple.  Values for an 'ls'
	listing are

	1: table offset in whole VFS

	2: table length in bytes

	3: table entries, as N VFSTableEntry structs
  */

  VFSRemoteResult vrr;
  vrr.offset = h->tableOffset;
  vrr.length = h->tablePtr - h->tableOffset;
  vrr.data   = addr + h->tableOffset;

  // Set this for completeness, we are NOT calling RemoteResultFree anyway
  vrr.dataOnHeap = 0;

  VFSRemoteResultWrite( &vrr, STDOUT_FILENO );

  munmap( addr, length );
  close( fd );

  return 0;
}

// eof
