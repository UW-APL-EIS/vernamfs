#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cmds.h"
#include "vernamfs.h"

char* recoverUsage = "recover OTPREMOTE OTPVAULT outputDir";

int recoverArgs( int argc, char* argv[] ) {

  if( argc < 4 ) {
	fprintf( stderr, "Usage: %s\n", recoverUsage );
	return -1;
  }

  return recover( argv[1], argv[2], argv[3] );
}

int recover( char* otpRemote, char* otpVault, char* outputDir ) {

  fprintf( stderr, "On the TODO list\n" );
  return 0;
}

// eof
