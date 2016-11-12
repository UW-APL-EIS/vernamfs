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
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "onetimepadfs.h"

/**
 * Testing the mmap system call for memory mapping a file/device into
 * the address space of the calling program.
 */

int main( int argc, char* argv[] ) {

  int ps = sysconf( _SC_PAGE_SIZE );
  printf( "PageSize: %d\n", ps );

  int fd = open( "1GB", O_RDWR );
  if( fd < 0 ) {
	perror( "open" );
	exit(1);
  }

  struct stat st;
  int sc = fstat( fd, &st );
  if( sc ) {
	perror( "fstat" );
	exit(1);
  }	

  size_t length = st.st_size;

  void* addr = mmap( NULL, length, PROT_READ | PROT_WRITE,
					 MAP_SHARED, fd, 0 );
  if( addr == MAP_FAILED ) {
	perror( "mmap" );
	exit(1);
  }

  OTPHeader hdr;
  OTPHeaderInit( &hdr, length );
  OTPHeaderWrite( &hdr, addr );

  munmap( addr, length );
  
  close( fd );
}
