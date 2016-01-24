#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cmds.h"
#include "aes128.h"

/**
 * @author Stuart Maclean
 */

char* helpUsage = "help <command>";

char* helpSynposis = "help <command>";

int helpArgs( int argc, char* argv[] ) {

  if( argc < 2 ) {
	fprintf( stderr, "%s\n", helpUsage );
	return -1;
  }
  return 0;
}

// eof
