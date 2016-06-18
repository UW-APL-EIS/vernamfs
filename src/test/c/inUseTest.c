#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @author Stuart Maclean
 *
 * Test correct serialized access to a VernamFS.
 *
 * 1: Start a VernamFS: vernamfs mount otpFile mnt
 *
 * 2: Run first instance of this program.
 *
 * 3: In new terminal, run second instance of this program, within 60
 * seconds of starting the first.  Should see:
 * 
 * open: Device or resource busy
 */
int main( int argc, char* argv[] ) {

  int fd = open( "mnt/bar", O_WRONLY | O_CREAT );
  if( fd < 0 ) {
	perror( "open" );
	return fd;
  }

  sleep( 60 );

  close( fd );
  return 0;
}
