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
