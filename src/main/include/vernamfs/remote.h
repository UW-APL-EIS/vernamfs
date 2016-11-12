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
#ifndef _REMOTE_DATA_TYPES_H
#define _REMOTE_DATA_TYPES_H

#include <stdint.h>

/**
 * @author Stuart Maclean
 *
 * The rls and rcat commands produce similar payloads.  We can thus
 * capture that payload type as an 'object' and use it in both places.
 * Can also use this object type in the re-construction vault-side,
 * i.e. in vls and vcat.
 *
 * Basically, anything than be derived remotely and then shipped back
 * to the vault is of the form:
 *
 * 1 offset
 * 2 length	(exclusive, i.e. length of the data only)
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
