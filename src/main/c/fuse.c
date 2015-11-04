#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "onetimepadfs.h"

static int vernamfs_getattr(const char *path, struct stat *stbuf ) {
  printf( "%s: %s\n", __FUNCTION__, path );

  if( strcmp( path, "/" ) == 0 )
	stbuf->st_mode = S_IFDIR | 0755;
  else 
	stbuf->st_mode = S_IFREG | 0222;

  return 0;
}

static int vernamfs_open( const char* path, struct fuse_file_info* fi ) {

  printf( "%s: %s %x\n", __FUNCTION__, path, fi->flags );

  if( (fi->flags & 3) != O_WRONLY )
	return -EACCES;

  fi->fh = (uint64_t)&Global;

  OTPAddEntry( &Global, path, Addr );
  OTPHeaderReport( &Global );
  return 0;
}

static int vernamfs_truncate(const char *path, off_t size) {
  printf( "%s: %s %u\n", __FUNCTION__, path, (unsigned)size );

  return 0;
}

static int vernamfs_write(const char *path, const char *buf, size_t size,
						  off_t offset, struct fuse_file_info *fi) {
  printf( "%s: %s %u %u\n",
		  __FUNCTION__, path, (unsigned)size, (unsigned)offset );

  if( fi->fh == 0 )
	return 0;

  OTPWrite( &Global, buf, size, Addr );
  OTPHeaderReport( &Global );

  return size;
}

static int vernamfs_release(const char *path, struct fuse_file_info *fi) {
  printf( "%s: %s\n", __FUNCTION__, path );

  if( fi->fh == 0 )
	return 0;

  OTPRelease( &Global, Addr );
  OTPHeaderReport( &Global );

  return 0;
}

static void vernamfs_destroy(void* env ) {

  printf( "%s\n", __FUNCTION__ );

  OTPHeaderStore( &Global, Addr );
}

struct fuse_operations vernamfs_ops = {
  .getattr = vernamfs_getattr,
  .open = vernamfs_open,
  .truncate = vernamfs_truncate,
  .write = vernamfs_write,
  .release = vernamfs_release,
  .destroy = vernamfs_destroy
};

// eof
