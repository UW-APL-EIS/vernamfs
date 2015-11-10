#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"

/**
 * @author Stuart Maclean
 *
 * Write a VFS Header structure out to a file/device.  Tablesize must
 * be supplied.  Can then view that header with e.g. xxd/od:
 *
 * $ vernamfs init FILE 1024
 *
 * $ xxd -l 128 FILE
 *
 * Will overwrite any existing header that may have been in
 * place. LOOK: check for existing header, via magic number, and
 * refuse to init unless a force flag supplied?
 */
int initArgs( int argc, char* argv[] ) {
  char* usage = "Usage: init OTPFILE tableSize";

  if( argc < 2 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  char* file = argv[0];
  int tableSize = atoi( argv[1] );
  if( tableSize < 1 ) {
	fprintf( stderr, "%s: Table size too small %d\n", argv[1], tableSize );
	return -1;
  }
  return initFile( file, tableSize );
}

int initFile( char* file, int tableSize ) {

  struct stat st;
  int sc = stat( file, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", file );
	return -1;
  }
  
  if( st.st_size <= sizeof( VFSHeader ) + tableSize * sizeof( VFSTableEntry ) ){
	fprintf( stderr, "%s: Too small to contain header+table\n", file );
	return -1;
  }
  
  VFS vfs;
  VFSInit( &vfs, st.st_size, tableSize );
  int fd = open( file, O_WRONLY );
  int nout = write( fd, &vfs.header, sizeof( VFSHeader ) );
  if( nout != sizeof( VFSHeader ) ){
	perror( "initFile.write" );
	close( fd );
	return -1;
  }

  VFSReport( &vfs );

  close( fd );
  return 0;
}

// eof
