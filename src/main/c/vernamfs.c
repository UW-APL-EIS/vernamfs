#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "vernamfs/vernamfs.h"
#include "vernamfs/version.h"

/**
 * @author Stuart Maclean
 *
 * The vernamfs 'back end'.  Called by fuse routines. Could also in
 * theory be called directly by programs linking to Vernamfs as a
 * library, if FUSE were not available.  Advantage of FUSE is that it
 * provides the serialization we require to ensure no byte of the OTP
 * is ever written to twice.
 */

static uint64_t alignUp( uint64_t val, uint64_t boundary );

// Accumulating count of the active file write.  Reset on release
static uint64_t totalLength = 0;

static int VFSHeaderInit( VFSHeader* thiz, size_t length, 
						  int maxFiles, int maxNameLength );
static void VFSHeaderLoad( VFSHeader* hTarget, void* addr );
static void VFSHeaderStore( VFSHeader* hSource, void* addr );
static void VFSHeaderReport( VFSHeader* h, int expert );

int VFSInit( VFS* thiz, size_t length, int maxFiles, int maxNameLength ) {
  VFSHeader* h = &thiz->header;
  return VFSHeaderInit( h, length, maxFiles, maxNameLength );
}

void VFSLoad( VFS* thiz, void* addr ) {
  VFSHeader* h = &thiz->header;
  VFSHeaderLoad( h, addr );
  thiz->backing = addr;
}

void VFSStore( VFS* thiz ) {
  VFSHeader* h = &thiz->header;
  VFSHeaderStore( h, thiz->backing );
}

// Debug...
void VFSReport( VFS* thiz, int expert ) {
  VFSHeader* h = &thiz->header;
  VFSHeaderReport( h, expert );

  //  printf( "Backing: %"PRIx64"\n", (uint64_t)thiz->backing );
}

// When fuse sees an 'open'...
int VFSAddEntry( VFS* thiz, const char* path ) {

  VFSHeader* h = &thiz->header;

  // If have allocated all our files, bail. The path name is irrelevant.
  if( h->tablePtr == h->tableOffset + h->maxFiles * h->tableEntrySize )
	return -ENOSPC;

  /*
	We can accommodate a new file, next check requested name length.
	The 1 is due to us needing to add a NULL to the stored name.
  */
  int requiredSpace = strlen( path ) + 1;
  int maxNameLength =  h->tableEntrySize - sizeof( VFSTableEntryFixed );
  if( requiredSpace > maxNameLength )
	return -ENAMETOOLONG;

  // Fill in the data offset in the table entry...
  VFSTableEntryFixed* te = 
	(VFSTableEntryFixed*)( thiz->backing + h->tablePtr );
  te->offset ^= h->dataPtr;

  // Fill in the name in the table entry...
  char* tableEntryName = 
	(char*)( thiz->backing + h->tablePtr + sizeof( VFSTableEntryFixed ) );
  int i;
  for( i = 0; i < requiredSpace; i++ )
	tableEntryName[i] ^= path[i];

  /*
	Note how the table entry length field is NOT filled in until the
	data length finally known, which is at close/release time.  At that
	time we also bump the tablePtr to start a new table entry.
  */

  return 0;
}

// When fuse sees a 'write'...
size_t VFSWrite( VFS* thiz, const void* buf, size_t count ) {

  VFSHeader* h = &thiz->header;

  // If have no room left, bail.  fuse expected to return ENOSPC
  uint64_t space = h->length - h->dataPtr;
  if( space == 0 )
	return -1;

  size_t actual = space > count ? count : space;
 
  char* dest = (char*)(thiz->backing + h->dataPtr);
  char* src = (char*)buf;
  int i;
  /*
	Write out to backing, XOR'ing as we go. This renders the data
	unreadable (LOOK: Is this costing us a byte-by-byte write to
	backing store??)
  */

  // Thought a word-at-a-time copy would be faster, it isn't!
  if( 0 ) {
	int words = actual / sizeof( size_t );
	int bytes = actual - words * sizeof( size_t );

	size_t* destS = (size_t*)dest;
	size_t* srcS = (size_t*)src;
	for( i = 0; i < words; i++ )
	  destS[i] ^= srcS[i];
	dest += words * sizeof( size_t );
	src += words * sizeof( size_t );
	for( i = 0; i < bytes; i++ )
	  dest[i] ^= src[i];
	
  } else {
	for( i = 0; i < actual; i++ )
	  dest[i] ^= src[i];
  }
	
  // Update the data ptr and total length of the file being written...
  h->dataPtr += actual;
  totalLength += actual;
  return actual;
}

// When fuse sees a 'release'...
void VFSRelease( VFS* thiz ) {
  VFSHeader* h = &thiz->header;
  VFSTableEntryFixed* te = 
	(VFSTableEntryFixed*)( thiz->backing + h->tablePtr );

  // Complete the table entry, with xor'ed length, hence unreadable
  te->length ^= totalLength;

  // Reset file total length and bump table and data ptrs
  totalLength = 0;
  h->tablePtr += h->tableEntrySize;
  h->dataPtr = alignUp( h->dataPtr, h->padding );
}

/********************** Private Impl: Header Read/Write ******************/

/**
 * @param length - Desired length of the entire VFS, in bytes.  The
 * length comprises the header, the FAT and the data area.
 *
 * @param maxFiles - Upper limit on the number of files the VFS must
 * hold.  Once set, cannot be changed.  Affects the FAT size.
 *
 * @param maxNameLength - How long file names can be in the VFS.
 * Affects FAT entry size and thus FAT size.  Typical values are 32,
 * 64.  FAT entry size is rounded up for next pow2.
 */
static int VFSHeaderInit( VFSHeader* thiz, size_t length, 
						  int maxFiles, int maxNameLength ) {
  
  if( maxFiles < 1 )
	return -1;
  if( maxNameLength < 1 || maxNameLength > VERNAMFS_MAXNAMELENGTH )
	return -1;

  // LOOK: Would this be better off as sectorSize == 512 bytes??
  uint64_t padding = sysconf( _SC_PAGE_SIZE );

  // The VFSHeader comes first, the table next, at padded offset
  uint64_t tableOffset = alignUp( sizeof( VFSHeader ), padding );

  /* 
	 Given user's suggested maxNameLength, we minimise our
	 VFSTableEntry size such that it can hold the required fixed parts
	 (currently offset and length) and a name of length at least the
	 maxNameLength, and we pad size of VFSTableEntry up to next 2^N.
	 We know the loop test will succeed, since we checked
	 maxNameLength too big above.
  */
  uint32_t tableEntrySize;
  int i;
  for( i = VERNAMFS_MINTABLEENTRYSIZE; i <= VERNAMFS_MAXTABLEENTRYSIZE; i<<=1) {
	// The 1 is needed for the NULL terminating the name
	uint64_t spaceForName = i - sizeof( VFSTableEntryFixed ) - 1;
	if( maxNameLength <= spaceForName ) {
	  tableEntrySize = i;
	  break;
	}
  }

  uint64_t tableExtent = alignUp( maxFiles * tableEntrySize, padding );

  uint64_t minDataArea = maxFiles * padding;

#if 0
  printf( "TO %d, TE %d, DM %d\n",
	  (int)tableOffset, (int)tableExtent, (int)minDataArea );
#endif
  
  /*
	Given table offset and extent plus minDataArea, have a lower bound
	on space required for the VFS.
  */
  if( tableOffset + tableExtent + minDataArea > length ) {
	return -1;
  }

  thiz->magic = VERNAMFS_MAGIC;
  thiz->type = FILESYSTEMTYPE_ENCRYPTEDFAT;
  thiz->version = (MAJOR_VERSION << 16) | (MINOR_VERSION << 8) |
	PATCH_VERSION;
  thiz->flags = 0;
  thiz->length = length;
  thiz->padding = padding;

  thiz->tableOffset = tableOffset;
  thiz->tablePtr = thiz->tableOffset;
  thiz->maxFiles = maxFiles;
  thiz->tableEntrySize = tableEntrySize;

  thiz->dataOffset = tableOffset + tableExtent;
  thiz->dataPtr = thiz->dataOffset;
  return 0;
}

static void VFSHeaderReport( VFSHeader* h, int expert ) {
  if( expert ) {
	printf( "Magic         : %"PRIx64" (%.8s)\n", h->magic, (char*)&h->magic );
	int maj = (h->version >> 16) & 0xff;
	int min = (h->version >> 8) & 0xff;
	int patch = h->version & 0xff;
	printf( "Version       : %d.%d.%d\n", maj, min, patch );
	printf( "Type          : %d\n", h->type );
	printf( "Flags         : 0x%04X\n", h->flags );
	printf( "Padding       : 0x%"PRIx64"\n", h->padding );
	printf( "Length        : 0x%"PRIx64"\n", h->length );
	printf( "\n" );
	printf( "TableOffset   : 0x%"PRIx64"\n", h->tableOffset );
	printf( "TableExtent   : 0x%x\n", h->tableEntrySize * h->maxFiles );
	printf( "TableEntrySize: %d\n", h->tableEntrySize );
	printf( "MaxFiles      : %d\n", h->maxFiles );
	printf( "\n" );
	printf( "DataOffset    : 0x%"PRIx64"\n", h->dataOffset );
	printf( "DataExtent    : 0x%"PRIx64"\n", h->length - h->dataOffset );
	printf( "\n" );
	printf( "TablePtr      : 0x%"PRIx64"\n", h->tablePtr );
	printf( "File Count    : 0x%"PRIx64"\n",
			(h->tablePtr - h->tableOffset) / h->tableEntrySize );
	printf( "DataPtr       : 0x%"PRIx64"\n", h->dataPtr );
	printf( "Data Total    : 0x%"PRIx64"\n", h->dataPtr - h->dataOffset );
  } else {
	printf( "Total filesystem size (bytes)           : %"PRIu64"\n",
			h->length );
	printf( "Maximum file name length                : %d\n",
			h->tableEntrySize - (int)sizeof( VFSTableEntryFixed ) - 1);
	printf( "\n" );
	printf( "Number of files the filesystem can hold : %d\n",
			h->maxFiles );
	printf( "Number of files already allocated       : %"PRId64"\n",
			(h->tablePtr - h->tableOffset) / h->tableEntrySize );
	printf( "\n" );
	printf( "Total space for file content (bytes)    : %"PRId64"\n",
			h->length - h->dataOffset );
	printf( "Space already used for file content     : %"PRId64"\n",
			h->dataPtr - h->dataOffset );
  }
}

static void VFSHeaderLoad( VFSHeader* hTarget, void* addr ) {
  VFSHeader* hSource = (VFSHeader*)addr;
  // Note that the header load is NOT an XOR operation, it cannot be...
  *hTarget = *hSource;
}

static void VFSHeaderStore( VFSHeader* hSource, void* addr ) {
  VFSHeader* hTarget = (VFSHeader*)addr;
  // Note that the header store is NOT an XOR operation, it cannot be...
  *hTarget = *hSource;
}

/**
 * @return val rounded up to next multiple of boundary
 *
 * Examples:
 *
 * alignUp(2000,4096) -> 4096
 * alignUp(8192,4096) -> 8192
 */

static uint64_t alignUp( uint64_t val, uint64_t boundary ) {
  return (uint64_t)(ceil( val / (double)boundary) * boundary);
}

// eof
