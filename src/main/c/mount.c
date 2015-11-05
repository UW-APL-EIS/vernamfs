#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"

// argc, argv straight from main, NOT shifted...
int mountArgs( int argc, char* argv[] ) {

  char* usage = "Usage: mount OTPFILE fuseOptions";

  /*
	Must be 4+, since (1) vernamfs, (2) mount, (3) our OTP file and 
	(4) a fuse mount point
  */
  if( argc < 2 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  printf( "argc %d\n", argc );
  char** cpp;
  for( cpp = argv; *cpp; cpp++ )
	printf( "argv %s\n", *cpp );
	
  char* file = argv[0];
  struct stat st;
  int sc = stat( file, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", file );
	return -1;
  }
  size_t length = st.st_size;

  int fd = open( file, O_RDWR );
  if( fd < 0 ) {
	fprintf( stderr, "%s: Not read/writable\n", file );
	return -1;
  }
  
  void* addr = mmap( NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
  if( addr == MAP_FAILED ) {
	fprintf( stderr, "%s: MMap failed\n", file );
	close( fd );
	return -1;
  }
  
  VFSLoad( &Global, addr );
  VFSReport( &Global );

  // If this backing file/device never had a VFS on it, initialise one now...
  if( Global.header.magic != VERNAMFS_MAGIC ) {

	// LOOK: User-configurable table size ??
	int tableSize = 1024;

	VFSInit( &Global, length, tableSize );
	VFSReport( &Global );
	VFSStore( &Global );
  }

  // Skip past our OPTFILE option
  argc--;
  argv++;

  // LOOK: We MUST have single-threaded access...
  
  int haveSOption = 0;
  for( cpp = argv; *cpp; cpp++ ) {
	char* cp = *cpp;
	if( strlen( cp ) > 1 && strcmp( "-s", cp ) == 0 ) {
	  haveSOption = 1;
	  break;
	}
  }

  if( haveSOption ) {
	printf( "argc %d\n", argc );
	for( cpp = argv; *cpp; cpp++ )
	  printf( "argv %s\n", *cpp );
	return fuse_main( argc, argv, &vernamfs_ops, NULL );
  }

  char** argv2 = (char**)malloc( (argc+2) * sizeof( char* ) );
  argv2[0] = "-s";
  int i = 1;
  for( cpp = argv; *cpp; cpp++, i++ ) 
	argv2[i] = *cpp;
  argv2[argc+1] = NULL;

  printf( "argc2 %d\n", argc+1 );
  for( cpp = argv2; *cpp; cpp++ )
	printf( "argv2 %s\n", *cpp );

  return fuse_main( argc+1, argv2, &vernamfs_ops, NULL );

}

// eof
