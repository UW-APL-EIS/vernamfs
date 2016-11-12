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
#ifndef _VERNAMFS_TYPES_H
#define _VERNAMFS_TYPES_H

#include <stdint.h>


/**
 * @author Stuart Maclean

 A 'Vernam File System' --- data storage based on a One Time Pad.  Named
 after Vernam (https://en.wikipedia.org/wiki/Gilbert_Vernam), a
 pioneer in this area.

  A 'Vernam File System' (VFS) viewed as a struct would be

  struct {
    VFSHeader header;
    char padding[];
    VFSTableEntry table[T];
    char padding[];
    char data[D];
  } VFS;

  for some table entry capacity T and data area size D.  The padding
  areas are to ensure the table and data areas align on a suitable
  boundary (disk sector or memory page size??).  The length L of the
  filesystem is then just sizeof( struct VFS ).

  The table is a crude 'file allocation table' (FAT). Entries in this
  table are of type VFSTableEntry:

  typedef struct {
    uint64_t offset;
    uint64_t length;
    char path[128-16];		// LOOK: Make 128 configurable ??
  } VFSTableEntry;

  which contain the file 'name', where its content lives in the data
  area (the offset) and its length.  Both offset and length are in
  bytes.  The offset acts as a pointer from the table entry into the
  data area. Offsets are likely padded to next page/sector unit.

  As files are added to the VFS, the table grows.  It can never
  shrink.  A pointer in the header locates where the next table entry
  should go.

  If we want (and we do!) no 'information leak' from the table, we use
  the same xor process used for the data area (see below).  This means
  that we cannot read back the table entries (and so nor can anyone
  else).  In particular, we cannot read back the file names, and hence
  have no way of saying whether any file 'exists' already.  The
  conseqeunces of this are that

  1: we can't have a 'directory structure', only a single flat
  'bucket' into which all files go.

  2: files can be added 2+ times.  Any second (third, etc) write does
  NOT append to the orginal data.  Rather, it is treated as a separate
  file.  So a file with name F and content C1 can be added, and later
  a file also with name F but with content C2 can be added.  As far as
  the VernamFS is concerned, these files are unrelated.

  In essence, the 'filesystem' is just a list of pairs (P,C) where P
  is a string resembling a Unix file name, and C is the content (byte
  sequence) associated with that P.  There is no requirement that all
  Ps be unique.

  The 'backing store' for a VernamFS is a one-time-pad (OTP), i.e. a
  file (or whole device) whose initial contents are just random data
  (a OTP!)

  Only the header is ever stored 'as is' (in the clear) into the
  backing store.  It must be, since it is updated as files are written
  to the VFS.  The header must there be readable as well as writable.
  The writes must move along a 'table pointer' and a 'data pointer'.

  The table and data areas are stored by first reading the original
  OTP data at those offsets, xor'ing the plain text info (table entry
  data or actual data) with the original, and writing back the result
  to the backing store.  Any such write must only be done ONCE, else
  recovery of any plain text is impossible.  Further, since the table
  and data areas are XOR'ed on write, they cannot be recovered by a
  read.  In essence, both the table and data areas are write-only, or
  actually 'write-only-and-ONCE-only'.  Every single byte in the
  backing store past the Header must be written at MOST one time.

  TODO:

  1 Have redundant copies of the header and table throughout the backing
  store, perhaps at end?  

  2 Include 'fixups' as used in NTFS, where a multi-byte value write
  becomes atomic, eliminating (or reducing?)  chances of data
  structure corruption in either VFSHeader or VFSTableEntry?

*/

/*
  8 bytes, little-endian, spells 'VERNAMFS', shows up nicely in a hexdump ;)
*/
#define VERNAMFS_MAGIC ((uint64_t)0x53464d414e524556LL)

/*
  Over time, we want to support variants of the Vernamfs.  For nowm
  our initial implementation is the 'encrypted FAT' variant
*/
#define FILESYSTEMTYPE_ENCRYPTEDFAT (1)


// For structs serialised to disk, ensure zero padding...
#pragma pack(1)

typedef struct {
  uint64_t magic;
  
  uint32_t type;

  /*
    Allow versioning over time. By burning the version info into the
    backing store, newer versions of code can identify/understand
    older versions of data.
  */
  uint32_t version;

  // Flags enable some forms of customization
  uint32_t flags;

  /*
    Length of the VFS, in bytes.  May be backed by a file, a whole
    device, or perhaps a partition on a device.
  */
  uint64_t length;

  /*
    Padding ensures that file content begins on a memory page or hard
    disk sector boundary. Currently we are using memory page, likely
    4096.
  */
  uint64_t padding;

  /*
    Where the file allocation table lives.  Comes after the header, 
    and is aligned by padding value, see above
  */
  uint64_t tableOffset;

  /*
    Maximum number of files VFS can hold, which is same
    as number of VFSTableEntry for which space is reserved.  It is 
    NOT the byte count of that space.
  */
  uint32_t maxFiles;

  /*
    Table entry size.  Is derived from the maximum file name length
    the user needs, subject to a limit defined here. We add a NULL to
    stored strings, so that decreases user's max by 1.

    Suggested maxNameLength: up to 15 -> tableEntrySize = 32.
    Suggested maxNameLength: > 15, up to 47 -> tableEntrySize = 64.
    Suggested maxNameLength: > 47, up to 111 -> tableEntrySize = 128.
  */
  uint32_t tableEntrySize;

  /*
    TablePtr is the byte index into the table that the next free file
    will take.  Note the units: bytes, NOT simple ordinal 0, 1, etc.
  */
  uint64_t tablePtr;


  /*
    DataOffset is the index into the table that the next free file will take
  */
  uint64_t dataOffset;
  uint64_t dataPtr;

} VFSHeader;

/*
  The logical view of VFSTableEntry, with a file name as last field.
  Since the actual tableEntrySize is decided at VFS initialization
  time, based on max file name length needed, the actual VFSTableEntry
  excludes the name member:

typedef struct {
  uint64_t offset;
  uint64_t length;
  char path[SOMEMAXNAMELENGTH];
} VFSTableEntryLogical;
*/

/*
  The actual VFSTableEntry description.  Structs on disk will always
  have a name (null-terminated) following the length.

  If/when we ever need any more fields (creation time?), they would go
  here.  But in order to still have new code read existing VFSs on disk,
  would need to bump version!
*/
typedef struct {
  uint64_t offset;
  uint64_t length;
} VFSTableEntryFixed;
  
/*
  Minimum tableEntrySize is 32, since a 16 byte one could hold 
  ONLY offset,length and thus would have no room for a name.
*/
#define VERNAMFS_MINTABLEENTRYSIZE (32)

#define VERNAMFS_MAXTABLEENTRYSIZE (128)

/*
  Given max table entry size, have this much room for the file name,
  given that we have to add a null-terminator to the stored string.
*/
#define VERNAMFS_MAXNAMELENGTH \
  (VERNAMFS_MAXTABLEENTRYSIZE - sizeof( VFSTableEntryFixed ) - 1)

/*
  As used by the init command, see init.c.  User can also
  request longer names, up to maxNameLength above
*/
#define VERNAMFS_NAMELENGTHDEFAULT (64 - sizeof( VFSTableEntryFixed ) -1)

/*
  Combine the VFSHeader together with its memory-mapped backing store,
  since often need both together.  
*/
typedef struct {
  VFSHeader header;
  void* backing;
} VFS;

/**
 * @return 0 if initialization worked, or -1 otherwise.  -1 condition
 * likely due to insufficient space to hold the VFS, given the supplied
 * length
 */
int VFSInit( VFS* thiz, size_t length, int maxFiles, int maxNameLength );

void VFSLoad( VFS* thiz, void* addr );

// Debug, print out info..
void VFSReport( VFS*, int expert );

/**
 * Called at each fuse_release and also at fuse_destroy
 */
void VFSStore( VFS* thiz );

/**
 * Called on fuse_open
 * 
 * @return 0 on success, or -1 if no space left to add a new entry.
 */
int VFSAddEntry( VFS* thiz, const char* name );

/**
 * Called on fuse_write
 */
size_t VFSWrite( VFS* thiz, const void* buf, size_t count );

/**
 * Called on fuse_release
 */
void VFSRelease( VFS* thiz );


extern struct fuse_operations vernamfs_ops;

extern VFS Global;

#pragma pack()

#endif
