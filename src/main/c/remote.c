#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "remote.h"

VFSRemoteResult* VFSRemoteResultRead( int fd ) {
  
  uint64_t offset;
  int nin = read( fd, &offset, sizeof( uint64_t ) );
  if( nin != sizeof( uint64_t ) ) {
	fprintf( stderr, "Cannot read remoteResult offset\n" );
	return NULL;
  }

  uint64_t length;
  nin = read( fd, &length, sizeof( uint64_t ) );
  if( nin != sizeof( uint64_t ) ) {
	fprintf( stderr, "Cannot read remoteResult length\n" );
	return NULL;
  }

  char* data = malloc( length );
  if( !data )
	return NULL;
  nin = read( fd, data, length );
  if( nin != length ) {
	fprintf( stderr, "Cannot read remoteResult data\n" );
	free( data );
	return NULL;
  }

  VFSRemoteResult* result = (VFSRemoteResult*)malloc
	( sizeof( VFSRemoteResult ) );
  if( !result )
	return NULL;
  result->offset = offset;
  result->length = length;
  result->data = data;
  result->dataOnHeap = 1;
  return result;
}

void VFSRemoteResultWrite( VFSRemoteResult* thiz, int fd ) {

  write( fd, &thiz->offset, sizeof( uint64_t ) );

  write( fd, &thiz->length, sizeof( uint64_t ) );
  
  write( fd, thiz->data, thiz->length );
}

void VFSRemoteResultFree( VFSRemoteResult* thiz ) {
  if( thiz->dataOnHeap )
	free( thiz->data );
}

// eof


