#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "vernamfs.h"

static uint64_t totalLength = 0;

static void VFSHeaderInit( VFSHeader* thiz, size_t length, int tableSize );
static void VFSHeaderLoad( VFSHeader* hTarget, void* addr );
static void VFSHeaderStore( VFSHeader* hSource, void* addr );

void VFSInit( VFS* thiz, size_t length, int tableSize ) {
  VFSHeader* h = &thiz->header;
  VFSHeaderInit( h, length, tableSize );
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
void VFSReport( VFS* thiz ) {
  VFSHeader* h = &thiz->header;
  printf( "Magic: %"PRIx64" (%.8s)\n", h->magic, (char*)&h->magic );
  printf( "Length: %"PRIx64"\n", h->length );
  printf( "PageSize: %"PRIx64"\n", h->pageSize );
  printf( "TableOffset: %"PRIx64"\n", h->tableOffset );
  printf( "TablePtr: %"PRIx64"\n", h->tablePtr );
  printf( "DataOffset: %"PRIx64"\n", h->dataOffset );
  printf( "DataPtr: %"PRIx64"\n", h->dataPtr );

  //  printf( "Backing: %"PRIx64"\n", (uint64_t)thiz->backing );

}

// When fuse sees an 'open'...
void VFSAddEntry( VFS* thiz, const char* path ) {
  VFSTableEntry te;
  memset( &te, 0, sizeof( VFSTableEntry ) );

  // 1: Write the entry name
  strcpy( te.path, path );

  // 2: Write the entry offset
  VFSHeader* h = &thiz->header;
  te.offset = h->dataPtr;
  
  /*
	3: Write out to backing, XOR'ing as we go. This renders the entry
	unreadable...
  */
  char* dest = (char*)( thiz->backing + h->tablePtr );
  char* src = (char*)&te;
  int i;
  for( i = 0; i < sizeof( VFSTableEntry ); i++ )
	dest[i] ^= src[i];

  /*
	4: Note how the entry length is not filled in until the data
	length finally known, which at close/release time.  At that time
	we also bump the tablePtr to start a new table entry.
  */

}

// When fuse sees a 'write'...
void VFSWrite( VFS* thiz, const void* buf, size_t count ) {
  VFSHeader* h = &thiz->header;
  char* dest = (char*)(thiz->backing + h->dataPtr);
  char* src = (char*)buf;
  int i;
  /*
	Write out to backing, XOR'ing as we go. This renders the data
	unreadable (LOOK: Is this costing us a byte-by-byte write to
	backing store??)
  */
  for( i = 0; i < count; i++ )
	dest[i] ^= src[i];

  // Update the data ptr and total length of the file being written...
  h->dataPtr += count;
  totalLength += count;
}

// When fuse sees a 'release'...
void VFSRelease( VFS* thiz ) {
  VFSHeader* h = &thiz->header;
  VFSTableEntry* te = (VFSTableEntry*)( thiz->backing + h->tablePtr );

  // Complete the table entry, with xor'ed length, hence unreadable
  te->length ^= totalLength;

  // Reset file total length and bump table and data ptrs
  totalLength = 0;
  h->tablePtr += sizeof( VFSTableEntry );
  h->dataPtr = (uint64_t)
	ceil(h->dataPtr / (double)h->pageSize) * h->pageSize;
}

/********************** Private Impl: Header Read/Write ******************/

static void VFSHeaderInit( VFSHeader* thiz, size_t length, int tableSize ) {
  thiz->magic = VERNAMFS_MAGIC;
  thiz->length = length;
  thiz->pageSize = sysconf( _SC_PAGE_SIZE );
  thiz->tableOffset = thiz->pageSize;
  thiz->tableSize = tableSize;
  thiz->tablePtr = thiz->tableOffset;
  thiz->dataOffset = thiz->tableOffset + 
	sizeof( VFSTableEntry ) * thiz->tableSize;
  thiz->dataPtr = thiz->dataOffset;
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


// eof
