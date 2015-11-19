#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"
#include "remote.h"

/**
 * The 'remote' rls listing expected as a file, or on STDIN.  It is of
 * course unintelligible since it is still XOR'ed with the OTP.
 *
 * The 'vaultFile' is the local, pristine copy of the original OTP.
 *
 * We can recover the logical remote table contents, and thus see a
 * listing of remote operations, by XOR'ing the actual remote table
 * and vault table together.
 *
 * An example test, using rls result in a pipe, no file:
 *
 * $ ./vernamfs rls 1M.R| ./vernamfs vls 1M.V
 *
 * Normally the rls result would be a file, shipped from remote to
 * vault location.
 *
 * The vls result is a printout, to stdout, of all allocated file
 * names with ther associated offset and length within the remote OTP.
 *
 * @see rls.c
 */

int vlsArgs( int argc, char* argv[] ) {

  char* usage = "Usage: vls OTPVAULT rlsResult?";

  if( argc < 1 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  char* file = argv[0];
  char* rlsResult = argc > 1 ? argv[1] : NULL;
  return vlsFile( file, rlsResult );
}

/*
  The 'remote ls' is expected as a file, or on STDIN.  It would have
  been obtained via an 'rls' command, and shipped to the 'vault'
  location.  
*/

int vlsFile( char* vaultFile, char* rlsResult ) {

  struct stat st;
  int sc = stat( vaultFile, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", vaultFile );
	return -1;
  }
  off_t vaultLength = st.st_size;

  int fdRls = STDIN_FILENO;
  if( rlsResult ) {
	fdRls = open( rlsResult, O_RDONLY );
	if( fdRls < 0 ) {
	  fprintf( stderr, "Cannot open rlsResult: %s\n", rlsResult );
	  return -1;
	}
  }
  
  VFSRemoteResult* rrls = VFSRemoteResultRead( fdRls );
  if( rlsResult )
	close( fdRls );

  if( rrls->offset + rrls->length > vaultLength ) {
	fprintf( stderr, "Vault length (%"PRIx64") too short, need %"PRIx64"\n",
			 vaultLength, rrls->offset + rrls->length );
	VFSRemoteResultFree( rrls );
	free( rrls );
	return -1;
  }

  // printf( "Off %x, len %x\n", rlsOffset, rlsLength );

  // Possible that the remote FS be currently empty
  if( rrls->length == 0 ) {
	VFSRemoteResultFree( rrls );
	free( rrls );
	return -1;
  }
  
  int fd = open( vaultFile, O_RDONLY );
  if( fd < 0 ) {
	fprintf( stderr, "Cannot open vaultFile: %s\n", vaultFile );
	return -1;
  }
  
  // Was trying length, offset related to rls info, not working...
  int mappedLength = vaultLength;
  int mappedOffset = 0;

  void* addr = mmap( NULL, mappedLength, PROT_READ, MAP_PRIVATE, 
					 fd, mappedOffset );
  if( addr == MAP_FAILED ) {
	fprintf( stderr, "Cannot mmap vaultFile: %s\n", vaultFile );
	close( fd );
	VFSRemoteResultFree( rrls );
	free( rrls );
	return -1;
  }

  VFS vaultVFS;
  VFSLoad( &vaultVFS, addr );

  // LOOK: Can/should check vaultVFS.header.tableOffset == vrr->offset

  /*
	Assumed that the vault header's tableEntrySize matches that of the
	remote data
  */
  int tableEntrySize = vaultVFS.header.tableEntrySize;
  int tableEntryCount = rrls->length / tableEntrySize;
  char* teActual = (char*)malloc( tableEntrySize );
  char* rls = rrls->data;
  char* vls = (char*)(addr + rrls->offset);
  int i;
  for( i = 0; i < tableEntryCount; i++ ) {
	char* teRemote = rls + i * tableEntrySize;
	char* teVault =  vls + i * tableEntrySize;
	int j;
	for( j = 0; j < tableEntrySize; j++ )
	  teActual[j] = teRemote[j] ^ teVault[j];
	VFSTableEntryFixed* tef = (VFSTableEntryFixed*)teActual;
	char* name = teActual + sizeof( VFSTableEntryFixed );

	// The vls result, just a print out of each file's properties
	printf( "%s 0x%"PRIx64" 0x%"PRIx64"\n", 
			name, tef->offset, tef->length );
  }
  free( teActual );

  VFSRemoteResultFree( rrls );
  free( rrls );

  munmap( addr, mappedLength );
  close( fd );

  return 0;
}

// eof
