#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cmds.h"
#include "aes128.h"

/**
 * @author Stuart Maclean
 */

char* helpSummary = "Explain a commnd";

char* helpSynopsis = "help <command>";

int helpArgs( int argc, char* argv[] ) {

  if( argc < 2 ) {
	char usage[1024];
	commandsSummary( usage );
	fprintf( stderr, "%s\n", usage );
	return -1;
  }  
  char* cmd = argv[1];
  Command* c = commandLocate( cmd );
  if( c ) {
	CommandHelp* help = c->help;
	printf( "\nNAME\n\t%s %s - %s\n\n", 
			ProgramName, c->name, help->summary );
	printf( "\nSYNOPSIS\n\t%s %s %s\n\n\n", 
			ProgramName, c->name, help->synopsis );
  } else {
	fprintf( stderr, 
			 "'%s' is not a %s command. See '%s help'.\n",
			 cmd, ProgramName, ProgramName );
	return -1;
  }

  return 0;
}

// eof
