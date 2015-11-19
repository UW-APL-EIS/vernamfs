#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"

char* infoUsage = "info OTPFILE";

int infoArgs( int argc, char* argv[] ) {

  if( argc < 2 ) {
	fprintf( stderr, "Usage: %s\n", infoUsage );
	return -1;
  }

  return info( argv[1] );
}

int info( char* file ) {

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
	VFSReport( &vfs );
  }

  close( fd );
  return 0;
}

// eof
