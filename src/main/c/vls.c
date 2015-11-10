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
 * The 'remote' rls listing expected available on STDIN.  It is of
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
 * Normally the rlsResult would be a file, shipped from remote to
 * vault location.
 *
 * @see rls.c
 */

int vlsArgs( int argc, char* argv[] ) {

  char* usage = "Usage: vls VAULTOTP rlsResult?";

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
  size_t vaultLength = st.st_size;

  int fdRls = STDIN_FILENO;
  if( rlsResult ) {
	fdRls = open( rlsResult, O_RDONLY );
	if( fdRls < 0 ) {
	  fprintf( stderr, "Cannot open rlsResult: %s\n", rlsResult );
	  return -1;
	}
  }
  
  VFSRemoteResult* vrr = VFSRemoteResultRead( fdRls );
  if( rlsResult )
	close( fdRls );

  if( vrr->offset + vrr->length > vaultLength ) {
	fprintf( stderr, "Vault length (%"PRIx64") too short, need %"PRIx64"\n",
			 vaultLength, vrr->offset + vrr->length );
	VFSRemoteResultFree( vrr );
	free( vrr );
	return -1;
  }

  // printf( "Off %x, len %x\n", rlsOffset, rlsLength );

  if( vrr->length == 0 ) {
	VFSRemoteResultFree( vrr );
	free( vrr );
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
	VFSRemoteResultFree( vrr );
	free( vrr );
	return -1;
  }

  VFSTableEntry* tableRemote = (VFSTableEntry*)(vrr->data);
  VFSTableEntry* tableVault = (VFSTableEntry*)(addr + vrr->offset);
  int tableEntryCount = vrr->length / sizeof( VFSTableEntry );
  int i;
  for( i = 0; i < tableEntryCount; i++ ) {
	VFSTableEntry* teRemote = tableRemote + i;
	VFSTableEntry* teVault =  tableVault + i;

	VFSTableEntry teActual;
	teActual.offset = teRemote->offset ^ teVault->offset;
	teActual.length = teRemote->length ^ teVault->length;
	int c;
	for( c = 0; c < sizeof( teActual.path ); c++ ) {
	  teActual.path[c] = teRemote->path[c] ^ teVault->path[c];
	}

	printf( "%s 0x%"PRIx64" 0x%"PRIx64"\n", 
			teActual.path, teActual.offset, teActual.length );
  }

  VFSRemoteResultFree( vrr );
  free( vrr );

  munmap( addr, mappedLength );
  close( fd );

  return 0;
}

// eof
