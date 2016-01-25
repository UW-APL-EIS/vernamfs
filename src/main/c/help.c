#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cmds.h"
#include "aes128.h"

/**
 * @author Stuart Maclean
 */

char* helpSummary = "help <command>";

char* helpSynopsis = "help <command>";

int helpArgs( int argc, char* argv[] ) {

  if( argc < 2 ) {
	char usage[1024];
	commandsSummary( usage );
	fprintf( stderr, "%s\n", usage );
	return -1;
  }

  return 0;
}

// eof
