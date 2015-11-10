#ifndef _REMOTE_DATA_TYPES_H
#define _REMOTE_DATA_TYPES_H

#include <stdint.h>

/**
 * @author Stuart Maclean
 *
 * The rls and rcat commands produce similar payloads.  We can thus
 * capture that payload type as an 'object' and use it in both places.
 * Can also use this object type in the re-construction vault-side, i.e. in 
 * vls and vcat.
 *
 * Basically, anything than be derived remotely and then shipped back
 * to the vault is of the form:
 *
 * 1 offset
 * 2 length
 * 3 data
 *
 * and we build a struct according to that requirement.  We'll WRITE
 * such objects in rls and rcat invocations, and READ them is vls and
 * vcat invocations.
 */

// For structs serialised to disk, ensure zero padding...
#pragma pack(1)

typedef struct {
  uint64_t offset;
  uint64_t length;
  char* data;
  int dataOnHeap;	// true for reads, false for writes
} VFSRemoteResult;

void VFSRemoteResultFree( VFSRemoteResult* thiz );

// LOOK: No reason why VFSRemoteResult itself need be on the heap...
VFSRemoteResult* VFSRemoteResultRead( int fd );

// TODO: just use fd version for now
VFSRemoteResult* VFSRemoteResultReadFile( char* file );

void VFSRemoteResultWrite( VFSRemoteResult* thiz, int fd );

// TODO: just use fd version for now
void VFSRemoteResultWriteFile( VFSRemoteResult* thiz, char* file );

#pragma pack()

#endif

// eof
