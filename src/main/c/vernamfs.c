#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "vernamfs.h"

static uint64_t totalLength = 0;

void OTPHeaderLoad( OTPHeader* thiz, void* addr ) {
  OTPHeader* p = (OTPHeader*)addr;
  // Note that the load is NOT an XOR operation, it cannot be...
  *thiz = *p;
}

void OTPHeaderInit( OTPHeader* thiz, size_t length, int tableSize ) {
  thiz->magic = VERNAMFS_MAGIC;
  thiz->length = length;
  thiz->pageSize = sysconf( _SC_PAGE_SIZE );
  thiz->tableOffset = thiz->pageSize;
  thiz->tableSize = tableSize;
  thiz->tablePtr = thiz->tableOffset;
  thiz->dataOffset = thiz->tableOffset + 
	sizeof( OTPTableEntry ) * thiz->tableSize;
  thiz->dataPtr = thiz->dataOffset;
}

void OTPHeaderStore( OTPHeader* thiz, void* addr ) {
  OTPHeader* p = (OTPHeader*)addr;
  // Note that the store is NOT an XOR operation, it cannot be...
  *p = *thiz;
}

// Debug...
void OTPHeaderReport( OTPHeader* thiz ) {
  printf( "Magic: %"PRIx64"\n", thiz->magic );
  printf( "Length: %"PRIx64"\n", thiz->length );
  printf( "PageSize: %"PRIx64"\n", thiz->pageSize );
  printf( "TableOffset: %"PRIx64"\n", thiz->tableOffset );
  printf( "TablePtr: %"PRIx64"\n", thiz->tablePtr );
  printf( "DataOffset: %"PRIx64"\n", thiz->dataOffset );
  printf( "DataPtr: %"PRIx64"\n", thiz->dataPtr );
}

// When fuse sees an 'open'...
void OTPAddEntry( OTPHeader* thiz, const char* path, void* addr ) {
  OTPTableEntry te;
  memset( &te, 0, sizeof( OTPTableEntry ) );
  strcpy( te.path, path );
  te.offset = thiz->dataPtr;
  
  char* dest = (char*)( addr + thiz->tablePtr );
  char* src = (char*)&te;
  int i;
  for( i = 0; i < sizeof( OTPTableEntry ); i++ )
	dest[i] ^= src[i];
}

// When fuse sees an 'write'...
void OTPWrite( OTPHeader* thiz, const void* buf, size_t count, void* addr ) {
  char* dest = (char*)(addr + thiz->dataPtr);
  char* src = (char*)buf;
  int i;
  for( i = 0; i < count; i++ )
	dest[i] ^= src[i];
  thiz->dataPtr += count;
  totalLength += count;
}

// When fuse sees an 'release'...
void OTPRelease( OTPHeader* thiz, void* addr ) {
  OTPTableEntry* te = (OTPTableEntry*)( addr + thiz->tablePtr );
  te->length ^= totalLength;

  totalLength = 0;
  thiz->tablePtr += sizeof( OTPTableEntry );
  thiz->dataPtr = (uint64_t)
	ceil(thiz->dataPtr / (double)thiz->pageSize) * thiz->pageSize;
}

// eof
