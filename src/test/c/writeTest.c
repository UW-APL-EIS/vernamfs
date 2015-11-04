#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main( int argc, char* argv[] ) {

  int fd = open( "foo/bar", O_WRONLY | O_CREAT );

  write( fd, "Hello", 5 );

  close( fd );
  return 0;
}
