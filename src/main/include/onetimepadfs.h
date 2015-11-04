#ifndef _ONETIMEPADFS_H
#define _ONETIMEPADFS_H

#include <stdint.h>

#include <fuse.h>

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
} OTPHeader;

typedef struct {
  uint64_t offset;
  uint64_t length;
  char path[128-16];
} OTPTableEntry;

void OTPHeaderLoad( OTPHeader* thiz, void* addr );

void OTPHeaderInit( OTPHeader* thiz, size_t length, int tableSize );

void OTPHeaderStore( OTPHeader* thiz, void* addr );

void OTPAddEntry( OTPHeader* thiz, const char* name, void* addr );

void OTPWrite( OTPHeader* thiz, const void* buf, size_t count, void* addr );

void OTPRelease( OTPHeader* thiz, void* addr );

void* OTPMap( char* device );

extern struct fuse_operations vernamfs_ops;

void OTPHeaderReport( OTPHeader* );

extern OTPHeader Global;
extern void* Addr;

#endif
