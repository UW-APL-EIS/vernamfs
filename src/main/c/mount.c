#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"

// argc, argv straight from main, NOT shifted, since fuse_main needs argv[0] ?
int mountArgs( int argc, char* argv[] ) {

  char* usage = "Usage: mount OTPFILE fuseOption* mountPoint";

  /*
	Must be 4+, since (1) progName, (2) 'mount', (3) our OTP file and 
	(4) a fuse mount point. 5+ would be any fuseOptions
  */
  if( argc < 4 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  char* file = argv[2];
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
  
  void* addr = mmap( NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
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

  /*
	Re-org the command line so that fuse_main doesn't see our 'mount'
	subcommand literal nor our OPTFILE.  Given that we MUST run the
	single-threaded fuse loop, overwrite our argv[1] with the '-s'
	option that forces single-threadedness, then shift all other args
	(including the NULL) down one place, which eliminates our OPTFILE
	from the args list.

	Note how we are preserving argv[0].  Note quite sure WHY we need
	to do this, but if we don't, Fuse does NOT work and we get left
	with un-unmountable broken mount points!  Fuse is using some
	property of argv[0] for sure.
  */
  argv[1] = "-s";
  int i;
  for( i = 2; i < argc; i++ )
	argv[i] = argv[i+1];

  return fuse_main( argc-1, argv, &vernamfs_ops );
}

// eof