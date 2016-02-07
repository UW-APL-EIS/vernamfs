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
 * Write a VFS Header structure out to a file/device.  Maximum file
 * count needed must be supplied.  Also, a -l N option can provide a
 * maximum file name length.

 * Can then view that header with e.g. xxd/od:
 *
 * $ vernamfs init FILE 1024
 *
 * $ vernamfs init -l 30 FILE 1024
 *
 * $ xxd -l 128 FILE
 *
 * Will only overwrite any existing header that may have been in place
 * if force flag supplied, via -f option:
 *
 * $ vernamfs init -l 30 FILE 1024
 * $ vernamfs init -l 30 -f FILE 1024
 */

static CommandHelp help = {
  .summary = "Initialise a device/file with a VernamFS header",
  .synopsis = "[<options>] device/file maxFiles",
  .description = "INIT DESC",
  .options = { { NULL, NULL } }
};

CommandHelp* helpInit = &help;


char* initOptions = 
  "\t-f\n\t\t Write a new VernamFS header on a device/file with an existing VernamFS\n";

// (-l maxFileNameLength)? OTPFILE maxFiles";



int initArgs( int argc, char* argv[] ) {

  int force = 0;
  int maxFileNameLength = VERNAMFS_NAMELENGTHDEFAULT;
  char* file = NULL;
  int maxFiles = 0;

  int c;
  while( (c = getopt( argc, argv, "fl:") ) != -1 ) {
	switch( c ) {
	case 'f':
	  force = 1;
	  break;
	case 'l':
	  maxFileNameLength = atoi( optarg );
	  break;
	default:
	  break;
	}
  }

  if( optind+2 > argc ) {
	fprintf( stderr, "Usage: %s\n", help.synopsis );
	return -1;
  }
  file = argv[optind];
  maxFiles = atoi( argv[optind+1] );

  if( maxFiles < 1 ) {
	fprintf( stderr, "%s: Max file count %d too small.\n", argv[0], maxFiles );
	return -1;
  }

  return init( file, maxFiles, maxFileNameLength, force );
}

int init( char* file, int maxFiles, int maxFileNameLength, int force ) {

  struct stat st;
  int sc = stat( file, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file.\n", file );
	return -1;
  }
  
  
  VFS vfs;
  sc = VFSInit( &vfs, st.st_size, maxFiles, maxFileNameLength );
  if( sc ) {
	fprintf( stderr, "%s:  Device too small.\n", file );
	return sc;
  }

  int fd = open( file, O_RDWR );
  uint64_t b8 = 0;
  int nin = read( fd, &b8, sizeof( b8 ) );
  if( nin != sizeof( b8 ) ) {
	perror( "init.read" );
	close( fd );
	return -1;
  }
  if( b8 == VERNAMFS_MAGIC && !force ) {
	fprintf( stderr, "%s: Already contains a VFS. Use -f to force init.\n", 
			 file );
	return -1;
  }
  
  lseek( fd, 0, SEEK_SET );
  int nout = write( fd, &vfs.header, sizeof( VFSHeader ) );
  if( nout != sizeof( VFSHeader ) ){
	perror( "init.write" );
	close( fd );
	return -1;
  }

  VFSReport( &vfs );

  close( fd );
  return 0;
}

// eof
