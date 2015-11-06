#ifndef _VERNAMFS_TYPES_H
#define _VERNAMFS_TYPES_H

#include <stdint.h>

#include <fuse.h>

/**
 * @author Stuart Maclean

  A 'Vernam File System' (VFS) viewed as a struct would be

  struct {
    VFSHeader header;
    char padding[];
    VFSTableEntry table[T];
    char padding[];
    char data[D];
  } VFS;

  for some table entry count T and data area size D.  The padding
  areas are to ensure the table and data areas align on a suitable
  boundary (disk sector or memory page size??).  The length L of the
  filesystem is then just sizeof( struct VFS ).

  The table is a crude 'file allocation table' (FAT). Entries in this
  table are of type VFSTableEntry:

  typedef struct {
    uint64_t offset;
    uint64_t length;
    char path[128-16];
  } VFSTableEntry;

  which contain the file 'name', where its content lives in the data
  area (the offset) and its length.  Both offset and length are in
  bytes.  The offset acts as a pointer from the table entry into the
  data area. Offsets are likely padded to next page/sector unit.

  As files are added to the VFS, the table grows.  It can never
  shrink.  A pointer in the header locates where the next table entry
  should go.

  If we want full 'encryption' of the table using the same xor process
  used for the data area (see below), this means that we cannot read
  back the table entries.  In particular, we cannot read back the file
  names, and hence have no way of saying whether any file 'exists'
  already.  The conseqeunces of this are that

  1: we can't have a 'directory structure', only a single flat
  'bucket' into which all files go.

  2: files can be added 2+ times.  Any second (third, etc) write does
  NOT append to the orginal data.  Rather, it is treated as a separate
  file.  

  In essence, the filesystem is a list of pairs (P,C) where P is a
  string resembling a Unix file name, and C is the content (byte
  sequence) associated with that P.  There is no requirement that all
  Ps be unique.

  The 'backing store' for a VernamFS is a one-time-pad (OTP), i.e. a
  file (or whole device) whose initial contents are just random data
  (a OTP!)

  Only the header is ever stored 'as is' into the backing store.  It
  must be, since it is updated as files are written to the VFS.  These
  writes must move along a 'table pointer' and a 'data pointer'.

  The table and data areas are stored by first reading the original
  OTP data, xor'ing the plain text info (table entry data or actual
  data) with the original, and writing back the result to the backing
  store.  Any such write must only be done ONCE, else recovery of any
  plain text is impossible.  Further, since the table and data areas
  are XOR'ed on write, they cannot be recovered by a read.  In
  essence, both the table and data areas are write-only, or actually
  'write-only-and-ONCE-only' 

  TODO:

  1 Have redundant copies of the header and table throughout the backing
  store, perhaps at end.  

  2 Include 'fixups' as used in NTFS, where a multi-byte value write
  becomes atomic, eliminating (or reducing?)  chances of data
  structure corruption in either VFSHeader or VFSTableEntry.

*/

/*
  8 bytes, little-endian, spells 'VERNAMFS', shows up nicely in a hexdump ;)
*/
#define VERNAMFS_MAGIC ((uint64_t)0x53464d414e524556LL)

// For structs serialised to disk, ensure zero padding...
#pragma pack(1)

typedef struct {
  uint64_t magic;
  
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
  uint64_t pageSize;
  uint64_t tableOffset;

  /*
    tableSize is maximum number of files VFS can hold, which is same
    as number of VFSTableEntry for which space is reserved.  It is 
    NOT the byte count of that space.
  */
  uint32_t tableSize;

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

// Debug, print out info..
void VFSReport( VFS* );

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


extern struct fuse_operations vernamfs_ops;

extern VFS Global;

#pragma pack()

#endif
