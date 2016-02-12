#include <stdio.h>
#include <stdlib.h>
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
 * INIT
 * ----
 *
 * init - Initialise a new file/device with a VFS.  Writes the VFS
 * header at the start of the file/device.  Size of the VFS is taken
 * from the size of the file/device.  User supplies a 'tableSize', which
 * identifies the maximum number of files that the VFS can hold.  
 * Can be done in any location (remote or vault), e.g.
 * 
 * $ vernamfs init /path/to/my/otp 1024
 *
 * INFO
 * ----
 * info - Print the metadata about a stored VFS.  Essentially prints
 * the 'VFSHeader' info, stored at offset 0 in backing store.  Can be
 * done in any location (remote or vault), e.g.
 * 
 * $ vernamfs info /path/to/my/otp
 *
 * RLS
 * --
 *
 * rls - Cat to stdout the (still xor'ed) VFS file metadata table.
 * Works on the remote OTP.  Prints to stdout, so may need
 * re-direction for capture:
 * 
 * remote$ vernam rls /path/to/my/otp > ls.cap
 *
 * VLS
 * ---
 *
 * vls - Uses output of rls and the pristine 'vault' OTP to recover
 * actual remote table metadata, which it prints to stdout.  Works on
 * the vault unit:
 *
 * vault$ vernam rls /path/to/my/otpCopy < ls.cap
 */

VFS Global;

int main( int argc, char* argv[] ) {

  cmds = (Command**)calloc( 32, sizeof( Command* ) );
  N = 0;
  
  Command help = {
	.name = "help",
	.help = helpHelp,
	.invoke = helpArgs
  };
  cmds[N++] = &help;

  Command generate = {
	.name = "generate",
	.help = helpGenerate,
	.invoke = generateArgs
  };
  cmds[N++] = &generate;

  Command init = {
	.name = "init",
	.help = helpInit,
	.invoke = initArgs
  };
  cmds[N++] = &init;

  Command info = {
	.name = "info",
	.help = helpInfo,
	.invoke = infoArgs
  };
  cmds[N++] = &info;
  
  Command mount = {
	.name = "mount",
	.help = helpMount,
	.invoke = mountArgs
  };
  cmds[N++] = &mount;
  
  Command rls = {
	.name = "rls",
	.help = helpRls,
	.invoke = rlsArgs
  };
  cmds[N++] = &rls;

  Command vls = {
	.name = "vls",
	.help = helpVls,
	.invoke = vlsArgs
  };
  cmds[N++] = &vls;

  Command rcat = {
	.name = "rcat",
	.help = helpRcat,
	.invoke = rcatArgs
  };
  cmds[N++] = &rcat;

  Command vcat = {
	.name = "vcat",
	.help = helpVcat,
	.invoke = vcatArgs
  };
  cmds[N++] = &vcat;
  
  Command recover = {
	.name = "recover",
	.help = helpRecover,
	.invoke = recoverArgs
  };
  cmds[N++] = &recover;

  cmds[N] = NULL;
  
  if( argc < 2 ) {
	char usage[1024];
	commandsSummary( usage );
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  char* name = argv[1];
  Command* c = commandLocate( name );
  if( c ) {
	if( strcmp( c->name, "mount" ) == 0 ) {
	  /*
		NOTE: fuse_main wants to see the REAL argc, argv.  It seems
		to need/use argv[0], the invoked program name
	  */
	  (c->invoke)( argc, argv );
	} else {
	  (c->invoke)( argc-1, argv+1 );
	}
	return 0;
  } else {
	fprintf( stderr, 
			 "%s: '%s' is not a vernamfs command. See 'vernamfs help'.\n",
			 ProgramName, name );
	return -1;
  }
}

// eof

