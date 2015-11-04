#include <stdio.h>

#include <fuse.h>

#include "vernamfs.h"

OTPHeader Global;
void* Addr;
GPtrArray* Directories;

int main( int argc, char* argv[] ) {

  Directories = g_ptr_array_new();

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
