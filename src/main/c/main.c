#include <stdio.h>
#include <string.h>

#include <fuse.h>

#include "cmds.h"
#include "vernamfs.h"

/**
 * @author Stuart Maclean
 *
 * Entry point into our Vernam FileSystem (VFS).  A VFS is a
 * fuse-based write-only file system where the persistent storage (the
 * 'backing') is file/device whose initial contents are a one-time-pad
 * (OTP).  
 * 
 * For any write operation, we READ the OTP data, XOR in our
 * data, and WRITE out the result to the backing.  
 * 
 * We talk of the 'remote' unit and the 'vault' unit.  The remote is
 * where we are storing data we don't want others to read should they
 * capture the remote, and the vault is the local, assumed safe, copy
 * of the OTP.  Ideally we can interrogate the remote while it is
 * remote, and upload data accordingly.
 *
 * Our VFS actually comprises several components, all available
 * through this single cmd line entry point (think how git works, sole
 * executable is git, which then switches off first cmd line arg).
 *
 * Our 'command set' is currently:
 *
 * MOUNT
 * -----
 * mount - The primary feature, mount an OTP device for file write operations, 
 * e.g.
 *
 * remote$ mkdir foo
 * remote$ vernamfs mount /path/to/my/otp foo
 * 
 * OR, with arbitrary FUSE options, e.g.
 * 
 * remote$ vernamfs mount /path/to/my/otp -d foo
 *
 * Remote users can then write to this mountpoint:
 *
 * remote$ cp myFile foo/
 * remote$ mv myFile foo/
 * remote$ seq 1000 > foo/seq.1000
 * remote$ echo HelloWorld > foo/msg
 * 
 * To umount, use the regular fusermount -u command:
 *
 * remote$ fusermount -u foo
 *
 * NB: Single-threaded FUSE operation is REQUIRED, and is set
 * internally.  Our VFS cannot work in a multi-threaded FUSE loop.
 * All access must be serialized.
 *
 * INFO
 * ----
 * info - Print the metadata about a stored VFS.  Essentially prints
 * the 'VFSHeader' info, stored at offset 0 in backing store.  Can be
 * done in any location (remote or vault), e.g.
 * 
 * $ vernamfs info /path/to/my/otp
 *
 * LS
 * --
 *
 * ls - Cat to stdout the (still xor'ed) VFS file metadata table.
 * Works on the remote OTP.  Prints to stdout, so may need
 * re-direction for capture:
 * 
 * remote$ vernam ls /path/to/my/otp > ls.cap
 *
 * XLS
 * ---
 *
 * xls - Uses output of ls and the pristine 'vault' OTP to recover
 * actual remote table metadata, which it prints to stdout.  Works on
 * the vault unit:
 *
 * vault$ vernam xls /path/to/my/otpCopy < ls.cap
 */

VFS Global;

int main( int argc, char* argv[] ) {

  char usage[256];
  sprintf( usage, 
		   "Usage: %s (mount FILE fuseOptions)|(info FILE)|(ls FILE)", 
		   argv[0] );
  
  if( argc < 2 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  char* cmd = argv[1];
  if( 0 ) {
  } else if( strcmp( cmd, "mount" ) == 0 ) {
	// NOTE: fuse_main wants to see the REAL argc, argv (??)
	return mountArgs( argc-2, argv+2 ); 
  } else if( strcmp( cmd, "info" ) == 0 ) {
	return infoArgs( argc-2, argv+2 );
  } else if( strcmp( cmd, "ls" ) == 0 ) {
	return lsArgs( argc-2, argv+2 );
  } else if( strcmp( cmd, "xls" ) == 0 ) {
	return xlsArgs( argc-2, argv+2 );
  } else {
	fprintf( stderr, "%s: Unknown command\n", cmd );
	fprintf( stderr, "%s\n", usage );
	return -1;
  }
}

// eof

