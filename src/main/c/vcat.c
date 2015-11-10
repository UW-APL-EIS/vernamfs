#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"
#include "remote.h"

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
 * A remote ls listing file is optional, and if supplied, will enable
 * naming of the new content.
 *
 * Example usage:
 *
 * vault$ vernamfs vcat VAULTOTP rcat.result rls.result?
 *
 * @see rcat.c
 * @see remote.c
 */

int vcatArgs( int argc, char* argv[] ) {

  char* usage = "Usage: vcat VAULTOTP rcatResult rlsResult?";

  if( argc < 2 ) {
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  char* vaultFile = argv[0];
  char* rcatResultFile = argv[1];
  char* rlsResultFile = argc > 2 ? argv[2] : NULL;

  return vcatFile( vaultFile, rcatResultFile, rlsResultFile );
}

int vcatFile( char* vaultFile, char* rcatResultFile, char* rlsResultFile ) {

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
  
  VFSRemoteResult* rcat = VFSRemoteResultRead( fdRcat );
  close( fdRcat );

  if( rcat->offset + rcat->length > vaultLength ) {
	fprintf( stderr, "Vault length (%"PRIx64") too short, need %"PRIx64"\n",
			 vaultLength, rcat->offset + rcat->length );
	VFSRemoteResultFree( rcat );
	free( rcat );
	return -1;
  }

  int fdVault = open( vaultFile, O_RDONLY );
  if( fdVault < 0 ) {
	perror( "open" );
	VFSRemoteResultFree( rcat );
	free( rcat );
	return -1;
  }
  
  void* addr = mmap( NULL, vaultLength, PROT_READ, MAP_PRIVATE, fdVault, 0 );
  if( addr == MAP_FAILED ) {
	perror( "mmap" );
	close( fdVault );
	VFSRemoteResultFree( rcat );
	free( rcat );
	return -1;
  }

  /*
	LOOK: if content length too long, a completely in-memory recovery
	will not be feasible...
  */
  char* content = malloc( rcat->length );
  if( !content ) {
	munmap( addr, vaultLength );
	close( fdVault );
	VFSRemoteResultFree( rcat );
	free( rcat );
	return -1;
  }

  char* remote = rcat->data;
  char* vault = addr + rcat->offset;

  // LOOK: do most of this a word at a time...
  int i;
  for( i = 0; i < rcat->length; i++ )
	content[i] = remote[i] ^ vault[i];

  /*
	TODO: consult rlsResult, transform to plain-text listing (as per vls)
	and can then look up correct file name based on offsets.  For now
	just write to STDOUT...
  */
  write( STDOUT_FILENO, content, rcat->length );

  munmap( addr, vaultLength );
  close( fdVault );
  VFSRemoteResultFree( rcat );
  free( rcat );

  return 0;
}

// eof
