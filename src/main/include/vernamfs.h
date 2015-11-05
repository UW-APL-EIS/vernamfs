#ifndef _VERNAMFS_TYPES_H
#define _VERNAMFS_TYPES_H

#include <stdint.h>

#include <fuse.h>

/**
 * @author Stuart Maclean

  A 'Vernam File System' viewed as a struct would be

  struct {
    VFSHeader header;
    VFSTableEntry table[T];
    char data[D];
  } VFS;

  for some tableCount T and data area size D.  

  The table is a crude 'file allocation table' (FAT). Entries in this
  table are of type VFSTableEntry:

  typedef struct {
    uint64_t offset;
    uint64_t length;
    char path[128-16];
  } VFSTableEntry;

  which contain the file 'name', where its content lives in the data
  area (offset) and its length.

  The 'backing store' for a VernamFS is a one-time-pad, i.e. a file
  (or whole device) whose initial contents are just random data (a OTP!)

  Only the header is ever stored 'as is' into the backing store.  It
  must be, since it is updated as users of the FS perform writes.  These
  writes must move along a 'table pointer' and a 'data pointer'.

  The table and data areas are stored by first reading the original
  OTP data, xor'ing the plain text info (table entry data or actual
  data) with the original, and writing back the result to the backing
  store.  Any such write must only be done ONCE, else recover of any
  plain text is impossible.  Further, since the table and data areas
  are XOR'ed on write, they cannot be recovered by a read.  In
  essence, both the table and data areas are write-only, or actually
  'write-only-and-ONCE-only'
*/

/*
  8 bytes, little-endian, spells 'VERNAMFS', shows up nicely in a hexdump ;)
*/
#define VERNAMFS_MAGIC ((uint64_t)0x53464d414e524556)

typedef struct {
  uint64_t magic;
  uint64_t length;
  uint64_t pageSize;
  uint64_t tableOffset;
  uint64_t tableSize;
  uint64_t tablePtr;
  uint64_t dataOffset;
  uint64_t dataPtr;
  void* backing;
} VFSHeader;

typedef struct {
  uint64_t offset;
  uint64_t length;
  char path[128-16];
} VFSTableEntry;


/*
  Combine the VFSHeader together with its memory-mapped backing store,
  since often need both together.  
*/
typedef struct {
  VFSHeader header;
  void* backing;
} VFS;

void VFSInit( VFS* thiz, size_t length, int tableSize );

void VFSLoad( VFS* thiz, void* addr );


/**
 * Called at each fuse_release and also at fuse_destroy
 */
void VFSStore( VFS* thiz );

/**
 * Called on fuse_open
 */
void VFSAddEntry( VFS* thiz, const char* name );

/**
 * Called on fuse_write
 */
void VFSWrite( VFS* thiz, const void* buf, size_t count );

/**
 * Called on fuse_release
 */
void VFSRelease( VFS* thiz );

// Debug, print out info..
void VFSReport( VFS* );

extern struct fuse_operations vernamfs_ops;

extern VFS Global;

#endif
