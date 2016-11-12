#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "vernamfs/cmds.h"
#include "vernamfs/vernamfs.h"
#include "vernamfs/remote.h"

/**
 * @author Stuart Maclean
 *
 * The 'vcat' command enables us to recover the true content of a
 * remote file whose contents are encrypted with the OTP.  It takes as
 * input 
 *
 * 1: the output of some previous rcat command,
 * 2: the local vault copy of the OTP.
 * 
 * A remote ls listing (rls) file is optional, and if supplied, will enable
 * naming of the new content.
 *
 * Example usage:
 *
 * vault$ vernamfs vcat OTPVAULT rcat.result rls.result?
 *
 * @see rcat.c
 * @see remote.c
 */

static char example1[] = 
  "remote$ vernamfs rcat OTP.remote 0x1234 0x5678 > remote.cat";
static char example2[] = 
  "vault$  vernamfs vcat OTP.vault remote.cat";
static char example3[] = 
  "vault$  vernamfs vcat OTP.vault remote.cat remote.ls";

static char* examples[] = { example1, example2, example3, NULL };

static CommandHelp help = {
  .summary = "Recover encrypted file content",
  .synopsis = "OTPVault rcatResult rlsResult?",
  .description = "Recover remote file content, by combining remote cat result and local vault\n  copy of the OTP. If remote ls result supplied, recovered content is\n  written to named file, else written to stdout.",
  .examples = examples
};

Command vcatCmd = {
  .name = "vcat",
  .help = &help,
  .invoke = vcatArgs
};

int vcatArgs( int argc, char* argv[] ) {

  if( argc < 3 ) {
	commandHelp( &vcatCmd );
	return -1;
  }

  char* vaultFile = argv[1];
  char* rcatResultFile = argv[2];
  char* rlsResultFile = argc > 3 ? argv[3] : NULL;

  return vcat( vaultFile, rcatResultFile, rlsResultFile );
}

int vcat( char* vaultFile, char* rcatResultFile, char* rlsResultFile ) {

  struct stat st;
  int sc = stat( vaultFile, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", vaultFile );
	return -1;
  }
  size_t vaultLength = st.st_size;

  int fdRcat = open( rcatResultFile, O_RDONLY );
  if( fdRcat < 0 ) {
	fprintf( stderr, "Cannot open rcatResult: %s\n", rcatResultFile );
	return -1;
  }
  
  VFSRemoteResult* rrcat = VFSRemoteResultRead( fdRcat );
  close( fdRcat );

  if( rrcat->offset + rrcat->length > vaultLength ) {
	fprintf( stderr, "Vault length (%"PRIx64") too short, need %"PRIx64"\n",
			 (uint64_t)vaultLength, rrcat->offset + rrcat->length );
	VFSRemoteResultFree( rrcat );
	free( rrcat );
	return -1;
  }

  int fdVault = open( vaultFile, O_RDONLY );
  if( fdVault < 0 ) {
	perror( "open" );
	VFSRemoteResultFree( rrcat );
	free( rrcat );
	return -1;
  }
  
  void* addr = mmap( NULL, vaultLength, PROT_READ, MAP_PRIVATE, fdVault, 0 );
  if( addr == MAP_FAILED ) {
	perror( "mmap" );
	close( fdVault );
	VFSRemoteResultFree( rrcat );
	free( rrcat );
	return -1;
  }

  /*
	LOOK: if content length too long, a completely in-memory recovery
	will not be feasible...
  */
  char* content = malloc( rrcat->length );
  if( !content ) {
	munmap( addr, vaultLength );
	close( fdVault );
	VFSRemoteResultFree( rrcat );
	free( rrcat );
	return -1;
  }

  char* rData = rrcat->data;
  char* vData = addr + rrcat->offset;

  // LOOK: do most of this a word at a time...
  int i;
  for( i = 0; i < rrcat->length; i++ )
	content[i] = rData[i] ^ vData[i];

  /*
	Consult any supplied rlsResult, transform to plain-text listing
	(as per vls) and can then look up correct file name based on
	matching offsets.  If fails, just write the vcat result to STDOUT.
  */
  char fileName[VERNAMFS_MAXNAMELENGTH+1] = {0};
  VFSRemoteResult* rrls = NULL;
  if( rlsResultFile ) {
	int fdRls = open( rlsResultFile, O_RDONLY );
	if( fdRls < 0 ) {
	  fprintf( stderr, "Cannot open rlsResult: %s\n", rlsResultFile );
	} else {
	  rrls = VFSRemoteResultRead( fdRls );
	  close( fdRls );
	  VFS vaultVFS;
	  VFSLoad( &vaultVFS, addr );

	  // LOOK: Can/should check vaultVFS.header.tableOffset == rrls->offset

	  int tableEntrySize = vaultVFS.header.tableEntrySize;
	  int tableEntryCount = rrls->length / tableEntrySize;
	  char* teActual = (char*)malloc( tableEntrySize );
	  char* rls = rrls->data;
	  char* vls = (char*)(addr + rrls->offset);
	  for( i = 0; i < tableEntryCount; i++ ) {
		char* teRemote = (char*)(rls + i * tableEntrySize);
		char* teVault  = (char*)(vls + i * tableEntrySize);
		int j;
		for( j = 0; j < tableEntrySize; j++ )
		  teActual[j] = teRemote[j] ^ teVault[j];
		
		VFSTableEntryFixed* tef = (VFSTableEntryFixed*)teActual;
		if( rrcat->offset == tef->offset ) {
		  char* cp = (char*)tef + sizeof( VFSTableEntryFixed );
		  //		printf( "Found %d: %s\n", i, cp );
		  if( *cp == '/' )
			cp++;
		  strcpy( fileName, cp );
		  break;
		}
	  }
	  free( teActual );
	  VFSRemoteResultFree( rrls );
	  free( rrls );
	}
  }
  
  /*
	Due to the lack of readability of the FS table on the remote unit,
	it is entirely possible that content was associated with the SAME
	file name 2+ times. Remotely, they are simply DIFFERENT
	files. That is not a problem remotely, but is here at the vault.
	We choose to always APPEND data to any existing local file.
  */
  if( strlen( fileName ) ) {
	int fd = open( fileName, 
				   O_WRONLY | O_CREAT | O_APPEND,
				   S_IRUSR | S_IRGRP | S_IROTH );
	int nout = write( fd, content, rrcat->length );
	if( nout != rrcat->length ) {
	  fprintf( stderr, "Failed to write %s: %d = %d (%d)\n",
			   fileName, (int)rrcat->length, nout, errno );
	}
	close( fd );
  } else {
	write( STDOUT_FILENO, content, rrcat->length );
  }

  munmap( addr, vaultLength );
  close( fdVault );
  VFSRemoteResultFree( rrcat );
  free( rrcat );
  return 0;
}

// eof
