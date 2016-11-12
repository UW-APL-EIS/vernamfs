/**
 * Copyright © 2016, University of Washington
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of the University of Washington nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL UNIVERSITY OF
 * WASHINGTON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "vernamfs/remote.h"

/**
 * @author Stuart Maclean
 *
 * LOOK: What is this struct for?
 */

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


