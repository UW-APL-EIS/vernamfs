#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

void* OTPMap( char* device ) {
  
  int fd = open( device, O_RDWR );
  if( fd < 0 ) {
	perror( "open" );
	return NULL;
  }

  struct stat st;
  int sc = fstat( fd, &st );
  if( sc ) {
	perror( "fstat" );
	close(fd);
	return NULL;
  }	

  size_t length = st.st_size;
  
  void* addr = mmap( NULL, length, PROT_READ | PROT_WRITE,
					 MAP_SHARED, fd, 0 );
  if( addr == MAP_FAILED ) {
	perror( "mmap" );
	return NULL;
  }
  return addr;
}

// eof

