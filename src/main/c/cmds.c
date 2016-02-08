#include <stdio.h>
#include <string.h>

#include "cmds.h"

Command** cmds;
int N;

char* ProgramName = "vernamfs";

Command* commandLocate( char* name ) {
  Command** cpp;
  for( cpp = cmds; *cpp; cpp++ ) {
	Command* cp = *cpp;
	if( strcmp( cp->name, name ) == 0 )
	  return cp;
  }
  return NULL;
}

static char* SPACES[] = { NULL,
						  "          ",
						  "         ",
						  "        ",
						  "       ",
						  "      ",
						  "     ",
						  "    ",
						  "   ",
						  "  ",
						  " " };

void commandsSummary( char result[] ) {
  int len = sprintf
	( result, "\nUsage: %s <command> [<args>]\n\nCommands:\n", ProgramName );
  Command** cpp;
  for( cpp = cmds; *cpp; cpp++ ) {
	Command* cp = *cpp;
	CommandHelp* help = cp->help;
	
	// Help has no help!
	if( !help )
	  continue;

	len += sprintf( result+len, "  %s%s%s\n", cp->name, 
					SPACES[strlen(cp->name)], help->summary );
  }
}

// eof
