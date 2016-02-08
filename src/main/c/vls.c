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
 * @author Stuart Maclean
 *
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
 * names with their associated offset and length within the remote OTP.
 *
 * If the raw printout is required (a -r option to vls), the table
 * entries are printed in raw form, i.e the results of the XOR
 * operation.  This is useful for demonstration purposes, since it can
 * be piped to e.g. xxd and compared to the rls result also piped to
 * xxd.
 *
 * @see rls.c
 */

static CommandHelp help = {
  .summary ="Combine vault, remote listings. Recovers remote file metadata",
  .synopsis = "-r? OTPVAULT rlsResult|STDIN",
  .description = "vls desc",
  .options = { NULL } 
};

CommandHelp* helpVls = &help;

int vlsArgs( int argc, char* argv[] ) {

  int raw = 0;
  char* vaultFile = NULL;
  char* rlsResult = NULL;
  
  int c;
  while( (c = getopt( argc, argv, "r") ) != -1 ) {
	switch( c ) {
	case 'r':
	  raw = 1;
	  break;
	default:
	  break;
	}
  }

  if( optind < argc ) {
	vaultFile = argv[optind];
  } else {
	fprintf( stderr, "Usage: %s\n", help.synopsis );
	return -1;
  }

  if( optind+1 < argc ) 
	rlsResult = argv[optind+1];

  // printf( "raw %d, vaultFile %s, rlsResult %p\n", raw, vaultFile,rlsResult );

  return vls( vaultFile, raw, rlsResult );
}

/*
  The 'remote ls' is expected as a file, or on STDIN.  It would have
  been obtained via an 'rls' command, and shipped to the 'vault'
  location.  
*/

int vls( char* vaultFile, int raw, char* rlsResult ) {

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

	/*
	  The vls result, just a print out each file's properties,
	  in either raw form or formatted
	*/
	if( raw ) {
	  write( STDOUT_FILENO, teActual, tableEntrySize ); 
	} else {
	  printf( "%s 0x%"PRIx64" 0x%"PRIx64"\n", 
			  name, tef->offset, tef->length );
	}
  }
  free( teActual );

  VFSRemoteResultFree( rrls );
  free( rrls );

  munmap( addr, mappedLength );
  close( fd );

  return 0;
}

// eof
