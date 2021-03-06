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
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <linux/fs.h>

/**
 * Verifying that stat does NOT reveal the correct size for device files.
 * Instead, the size is reported as 0.
 */

off_t deviceSize( char* device ) {

  int fd = open( device, O_RDONLY );
  if( fd == -1 ) {
	return -1;
  }
  
  // derive size...
  int64_t size = 0;
  int sc = ioctl(fd, BLKGETSIZE64, &size);
  close( fd );
  if( sc == -1 ) {
	return -1;
  }
  return size;
}

int main( int argc, char* argv[] ) {

  char* device = "/dev/sda";
  if( argc > 1 )
	device = argv[1];

  struct stat st;

  int sc = stat( device, &st );
  if( sc ) {
	perror( "stat" );
	return -1;
  }
  
  printf( "stat.size of %s = %lu\n",  device, st.st_size );

  if( st.st_size == 0 ) {
	off_t ds = deviceSize( device );
	printf( "ioctl.size of %s = %lu\n",  device, ds );
  }

  return 0;
}

// eof
