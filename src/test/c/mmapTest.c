#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

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


  void* addr = mmap( NULL, st.st_size, PROT_READ | PROT_WRITE,
					 MAP_PRIVATE, fd, 0 );
  if( addr == MAP_FAILED ) {
	perror( "mmap" );
	exit(1);
  }

  close( fd );
}
