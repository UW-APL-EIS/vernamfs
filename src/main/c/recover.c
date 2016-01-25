#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"

char* recoverSynopsis = "OTPREMOTE OTPVAULT outputDir";

char* recoverSummary = "Combine vault and remote OTPs";

int recoverArgs( int argc, char* argv[] ) {

  if( argc < 4 ) {
	fprintf( stderr, "Usage: %s\n", recoverSynopsis );
	return -1;
  }

  return recover( argv[1], argv[2], argv[3] );
}

int recover( char* otpRemote, char* otpVault, char* outputDir ) {

  struct stat st;
  int sc = stat( otpRemote, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", otpRemote );
	return -1;
  }
  off_t remoteLength = st.st_size;

  sc = stat( otpVault, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "Not a regular file: %s\n", otpVault );
	return -1;
  }
  off_t vaultLength = st.st_size;

  int fdR = open( otpRemote, O_RDONLY );
  if( fdR < 0 ) {
	fprintf( stderr, "Cannot open: %s\n", otpRemote );
	return -1;
  }
  
  void* addrR = mmap( NULL, remoteLength, PROT_READ, MAP_PRIVATE, 
					  fdR, 0 );
  if( addrR == MAP_FAILED ) {
	fprintf( stderr, "Cannot mmap: %s\n", otpRemote );
	close( fdR );
	return -1;
  }

  int fdV = open( otpVault, O_RDONLY );
  if( fdV < 0 ) {
	fprintf( stderr, "Cannot open: %s\n", otpVault );
	munmap( addrR, remoteLength );
	close( fdR );
	return -1;
  }
  
  void* addrV = mmap( NULL, vaultLength, PROT_READ, MAP_PRIVATE, 
					  fdV, 0 );
  if( addrV == MAP_FAILED ) {
	fprintf( stderr, "Cannot mmap: %s\n", otpVault );
	close( fdV );
	munmap( addrR, remoteLength );
	close( fdR );
	return -1;
  }


  sc = mkdir( outputDir, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH );
  if( sc && errno != EEXIST ) {
	fprintf( stderr, "Cannot mkdir: %s\n", outputDir );
	munmap( addrV, vaultLength );
	close( fdV );
	munmap( addrR, remoteLength );
	close( fdR );
	return -1;
  }

  VFS remoteVFS;
  VFSLoad( &remoteVFS, addrR );
  VFSHeader* hR = &remoteVFS.header;

  uint64_t tableLength = hR->tablePtr - hR->tableOffset;
  uint32_t tableEntrySize = hR->tableEntrySize;
  uint32_t tableEntryCount = tableLength / tableEntrySize;

  char* tableR = (char*)(addrR + hR->tableOffset);
  char* tableV = (char*)(addrV + hR->tableOffset);
  char* teActual = (char*)malloc( tableEntrySize );
  int i;
  for( i = 0; i < tableEntryCount; i++ ) {
	char* teRemote = tableR + i * tableEntrySize;
	char* teVault =  tableV + i * tableEntrySize;
	int j;
	for( j = 0; j < tableEntrySize; j++ )
	  teActual[j] = teRemote[j] ^ teVault[j];
	VFSTableEntryFixed* tef = (VFSTableEntryFixed*)teActual;
	char* name = teActual + sizeof( VFSTableEntryFixed );
	
	if( 0 )
	  printf( "%s %lx %lx\n", name, tef->offset, tef->length );

	char* contentActual = (char*)malloc( tef->length );
	char* contentR = (char*)(addrR + tef->offset );
	char* contentV = (char*)(addrV + tef->offset );
	uint64_t c;
	for( c = 0; c < tef->length; c++ ) {
	  contentActual[c] = contentR[c] ^ contentV[c];
	}
	char path[256];
	// Offset name by 1 char, since the stored value leads with '/'
	sprintf( path, "%s/%s", outputDir, name+1 );
	int fdOut = open( path, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
	if( fdOut < 0 ) {
	  fprintf( stderr, "Cannot open: %s (%d)\n", path, errno );
	} else {
	  ssize_t nout = write( fdOut, contentActual, tef->length );
	  if( nout != tef->length ) {
		fprintf( stderr, "Write failure: %s (%d)\n", path, errno );
	  }
	  close( fdOut );
	}
	free( contentActual );
	free( teActual );
  }

  munmap( addrV, vaultLength );
  close( fdV );
  munmap( addrR, remoteLength );
  close( fdR );
  
  return 0;
}

// eof
