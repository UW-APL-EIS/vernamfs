#include <stdio.h>

#include "onetimepadfs.h"

int main( int argc, char* argv[] ) {

  if( argc < 2 ) {
	fprintf( stderr, "%s: device|file\n", argv[0] );
	return 1;
  }

  char* device = argv[1];
  
  OTPHeader h;
  void* addr = OTPMap( device );
  OTPHeaderLoad( &h, addr );
  OTPHeaderReport( &h );

  return 0;
}
