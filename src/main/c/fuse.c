#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "vernamfs.h"

static int vernamfs_getattr(const char *path, struct stat *stbuf ) {
  printf( "%s: %s\n", __FUNCTION__, path );

  memset( stbuf, 0, sizeof( struct stat) );

  gboolean isDir = FALSE;

  if( (strcmp(path, "/") == 0) ) {
	isDir = TRUE;
  } else {
	int i;
	for( i = 0; i < Directories->len; i++ ) {
	  char* dir = (char*)g_ptr_array_index( Directories, i );
	  if( strcmp( path, dir ) == 0 ) {
		isDir = TRUE;
		break;
	  }
	}
  }

  if( isDir ) {
	stbuf->st_mode = S_IFDIR | 0755;
  } else {
	stbuf->st_mode = S_IFREG | 0222;
  }

  return 0;
  //return -ENOENT;
}

static int vernamfs_mkdir(const char* path, mode_t m) {
  printf( "%s: %s\n", __FUNCTION__, path );

  g_ptr_array_add( Directories, g_strdup( path ) );
  return 0;
}
  
static int vernamfs_create(const char *path, mode_t m, 
						   struct fuse_file_info* fi ) {
  printf( "%s: %s\n", __FUNCTION__, path );
  return 0;
}


static int vernamfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
							off_t offset, struct fuse_file_info *fi) {
  
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
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
  .mkdir = vernamfs_mkdir,
  .readdir = vernamfs_readdir,
  //.create = vernamfs_create,
  .open = vernamfs_open,
  .truncate = vernamfs_truncate,
  .write = vernamfs_write,
  .release = vernamfs_release,
  .destroy = vernamfs_destroy
};

// eof
