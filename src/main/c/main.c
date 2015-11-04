#include <stdio.h>

#include <fuse.h>

#include "onetimepadfs.h"

OTPHeader Global;
void* Addr;

int main( int argc, char* argv[] ) {

  Addr = OTPMap( "1GZ" );
  
  printf( "Addr: %p\n", Addr );

  OTPHeaderLoad( &Global, Addr );
  OTPHeaderReport( &Global );

  if( Global.magic != VERNAMFS_MAGIC ) {
	OTPHeaderInit( &Global, (1 << 30), 1024 );
	OTPHeaderReport( &Global );
	OTPHeaderStore( &Global, Addr );
  }

  fuse_main( argc, argv, &vernamfs_ops );
  return 0;
}
