#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"

/**
 * The 'remote' ls listing expected available on STDIN.  It is of
 * course unintelligible since it is still XOR'ed with the OTP. 
 *
 * The 'vaultFile' is the local, pristine copy of the original OTP.
 *
 * We can recover the logical remote table contents, and thus see a
 * listing of remote operations, by XOR'ing the actual remote table
 * and vault table together.
 *
 * @see ls.c
 */

int xlsArgs( int argc, char* argv[] ) {

  char* usage = "Usage: xls VAULTOPTFILE";

  if( argc < 1 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  return xlsFile( argv[0] );
}

/*
  The 'remote data' is expected on STDIN.  It would have been obtained
  via an 'ls' command, and shipped to the 'vault' location.
*/

int xlsFile( char* vaultFile ) {

  struct stat st;
  int sc = stat( vaultFile, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", vaultFile );
	return -1;
  }
  size_t vaultLength = st.st_size;

  VFSHeader headerRemote;
  int nin = read( 0, &headerRemote, sizeof( VFSHeader ) );
  if( nin != sizeof( VFSHeader ) ) {
	fprintf( stderr, "Cannot read remoteHeader\n" );
	return -1;
  }

  if( headerRemote.tablePtr > vaultLength ) {
	fprintf( stderr, "Vault length (%"PRIx64") too short, need %"PRIx64"\n",
			 vaultLength, headerRemote.tablePtr );
	return -1;
  }

  // Locate and skip to the tableOffset in the remote data...
  int gapSize = headerRemote.tableOffset - sizeof( VFSHeader );
  char* gap = malloc( gapSize );
  nin = read( 0, gap, gapSize );
  if( nin != gapSize ) {
	fprintf( stderr, "Cannot locate remoteTable\n" );
	free( gap );
	return -1;
  }
  free( gap );
  
  int fd = open( vaultFile, O_RDONLY );
  if( fd < 0 ) {
	fprintf( stderr, "Cannot open vaultFile: %s\n", vaultFile );
	return -1;
  }
  
  int mappedLength = headerRemote.tablePtr;

  void* addr = mmap( NULL, mappedLength, PROT_READ, MAP_PRIVATE, fd, 0 );
  if( addr == MAP_FAILED ) {
	fprintf( stderr, "Cannot mmap vaultFile: %s\n", vaultFile );
	close( fd );
	return -1;
  }

  int tableEntryCount = (headerRemote.tablePtr - headerRemote.tableOffset) /
	sizeof( VFSTableEntry );
  
  VFSTableEntry* tableVault = (VFSTableEntry*)(addr + headerRemote.tableOffset);
  
  int i;
  for( i = 0; i < tableEntryCount; i++ ) {

	VFSTableEntry teRemote;
	nin = read( 0, &teRemote, sizeof( VFSTableEntry ) );
	if( nin != sizeof( VFSTableEntry ) ) {
	  fprintf( stderr, "Cannot read remoteTableEntry\n" );
	  goto done;
	}

	VFSTableEntry* teVault = tableVault + i;

	VFSTableEntry teActual;
	teActual.offset = teRemote.offset ^ teVault->offset;
	teActual.length = teRemote.length ^ teVault->length;
	int c;
	for( c = 0; c < sizeof( teActual.path ); c++ ) {
	  teActual.path[c] = teRemote.path[c] ^ teVault->path[c];
	}

	printf( "%s %"PRIx64" %"PRIx64"\n", 
			teActual.path, teActual.offset, teActual.length );
  }

 done:
  munmap( addr, mappedLength );
  close( fd );

  return 0;
}

// eof
