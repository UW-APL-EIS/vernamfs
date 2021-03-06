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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <fuse.h>

#include "vernamfs/vernamfs.h"

/**
 * @author Stuart Maclean
 *
 * Fuse callbacks required for vernamfs. Uses a single VFS struct,
 * named Global, for the actual back-end implementation.
 */

static int inUse = 0;

static int vernamfs_getattr(const char *path, struct stat *stbuf ) {
  if( 1 )
	printf( "%s: %s\n", __FUNCTION__, path );

  /*
	If we don't say 'everything except / is a file' then we cannot
	open any file, even for writing.
  */

  if( strcmp( path, "/" ) == 0 ) {
	stbuf->st_mode = S_IFDIR | 0755;
	stbuf->st_uid = 0;
	stbuf->st_gid = 0;
	stbuf->st_size = 0;
	stbuf->st_nlink = 0;
	return 0;
  }

  stbuf->st_mode = S_IFREG | 0222;
  return 0;
}

/*
  Without any impl of readdir, a 'ls mountPoint' returns 'Function not
  implemented'.  I prefer the result to be 'Operation not supported',
  which we achieve by including this readdir impl. 
*/ 
static int vernamfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
							off_t offset, struct fuse_file_info *fi) {
  if( 1 )
	printf( "%s: %s\n", __FUNCTION__, path );

  //  return -ENOTSUP;
  return 0;
}

static int vernamfs_access( const char *path, int mask ) {
  if( 1 )
	printf( "%s: %s %x\n", __FUNCTION__, path, mask );

  if( strcmp( path, "/" ) == 0 )
	return 0;

  if( 0 )
	return -ENOENT;
  
  if( mask != W_OK )
	return -EACCES;
	  
  return 0;
}

#if 0
static int vernamfs_create( const char* path, mode_t mask,
							struct fuse_file_info * fi ) {
  if( 1 )
	printf( "%s: %s %x\n", __FUNCTION__, path, mask );

  return 0;
}
#endif


/**
 * When a user program does
 <code>
 int fd = open( "mount/file.c", O_WRONLY );
 </code>
 *
 */
static int vernamfs_open( const char* path, struct fuse_file_info* fi ) {
  if( 1 )
	printf( "%s: %s %x\n", __FUNCTION__, path, fi->flags );

  // Has to be write-only.  Any read access is meaningless
  if( (fi->flags & (O_RDONLY|O_WRONLY|O_RDWR)) != O_WRONLY )
	return -ENOTSUP;

  /*
	In addition to being write-only, cannot open for appends since
	we cannot locate any existing file name and so also no data.
  */
  if( (fi->flags & O_APPEND) == O_APPEND )
	return -ENOTSUP;

  if( inUse )
	return -EBUSY;
  inUse = 1;

  fi->fh = (uint64_t)&Global;

  int sc = VFSAddEntry( &Global, path );

  // LOOK: make reporting a debug option...
  if( 0 ) 
	VFSReport( &Global, 1 );

  return sc;
}

static int vernamfs_truncate(const char *path, off_t size) {
  if( 1 )
	printf( "%s: %s %u\n", __FUNCTION__, path, (unsigned)size );

  return 0;
}

/*
  Without any impl of unlink, any mv or rm on a file 'Function not
  implemented'.  I prefer the result to be 'Operation not supported',
  which we achieve by including this unlink impl. 
*/ 
static int vernamfs_unlink(const char *path) {
  if( 1 )
	printf( "%s: %s\n", __FUNCTION__, path );

  return -ENOTSUP;
}

static int vernamfs_write(const char *path, const char *buf, size_t size,
						  off_t offset, struct fuse_file_info *fi) {
  if( 1 )
	printf( "%s: %s %u %u\n",
		  __FUNCTION__, path, (unsigned)size, (unsigned)offset );

  if( fi->fh == 0 )
	return 0;

  size_t sc = VFSWrite( &Global, buf, size );
  
  if( 0 )
	VFSReport( &Global, 1 );

  return sc == -1 ? -ENOSPC : sc;
}

static int vernamfs_release(const char *path, struct fuse_file_info *fi) {
  if( 1 )
	printf( "%s: %s\n", __FUNCTION__, path );

  if( fi->fh == 0 )
	return 0;

  VFSRelease( &Global );
  VFSStore( &Global );

  if( 0 )
	VFSReport( &Global, 1 );

  inUse = 0;

  return 0;
}

static void vernamfs_destroy(void* env ) {

  if( 1 )
	printf( "%s\n", __FUNCTION__ );

  VFSStore( &Global );
}

struct fuse_operations vernamfs_ops = {
  .getattr = vernamfs_getattr,
  .readdir = vernamfs_readdir,
  .access = vernamfs_access,
  //  .create = vernamfs_create,
  .open = vernamfs_open,
  .truncate = vernamfs_truncate,
  .unlink = vernamfs_unlink,
  .write = vernamfs_write,
  .release = vernamfs_release,
  .destroy = vernamfs_destroy
};

// eof
