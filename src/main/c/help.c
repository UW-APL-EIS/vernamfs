#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cmds.h"

/**
 * @author Stuart Maclean
 */

static CommandHelp help = {
  .summary = "Explain available commands"
};

Command helpCmd = {
  .name = "help",
  .help = &help,
  .invoke = helpArgs
};

int helpArgs( int argc, char* argv[] ) {

  //  printf( "%s: %d\n", __FUNCTION__, argc );

  if( argc < 2 ) {
	char usage[1024];
	commandsSummary( usage );
	fprintf( stderr, "%s\n", usage );
	return -1;
  }  
  char* cmd = argv[1];
  Command* c = commandLocate( cmd );
  if( c ) {
	commandHelp( c );
  } else {
	fprintf( stderr, 
			 "'%s' is not a %s command. See '%s help'.\n",
			 cmd, ProgramName, ProgramName );
	return -1;
  }

  return 0;
}

void commandHelp( Command* c ) {
  CommandHelp* help = c->help;
  printf( "\nNAME\n  %s %s - %s\n", 
		  ProgramName, c->name, help->summary );
  printf( "\nSYNOPSIS\n  %s %s %s\n", 
		  ProgramName, c->name, help->synopsis );
  printf( "\nDESCRIPTION\n  %s\n", help->description );
  if( help->options ) {
	printf( "\nOPTIONS\n" );
	CommandOption** cpp;
	for( cpp = help->options; *cpp; cpp++ ) {
	  CommandOption* cp = *cpp;
	  printf( "  -%s\n    %s\n", cp->id, cp->text );
	}
  }
  if( help->examples ) {
	printf( "\nEXAMPLES\n" );
	char** cpp;
	for( cpp = help->examples; *cpp; cpp++ ) {
	  char* cp = *cpp;
	  printf( "  %s\n", cp );
	}
	printf( "\n" );
  }
  //printf( "\n%s\n", c->help->summary );
  //printf( "\nUsage: %s %s %s\n", ProgramName, c->name, c->help->synopsis );
}

// eof
