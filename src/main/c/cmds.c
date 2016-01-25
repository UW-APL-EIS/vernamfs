#include <stdio.h>
#include <string.h>

#include "cmds.h"

Command** cmds;
int N;

Command* commandLocate( char* name ) {
  Command** cpp;
  for( cpp = cmds; *cpp; cpp++ ) {
	Command* cp = *cpp;
	if( strcmp( cp->name, name ) == 0 )
	  return cp;
  }
  return NULL;
}

void commandsSummary( char result[] ) {
  int len = sprintf
	( result, "\nUsage: vernamfs <command> [<commandArgs>]\n\nCommands:\n" );
  Command** cpp;
  for( cpp = cmds; *cpp; cpp++ ) {
	Command* cp = *cpp;
	len += sprintf( result+len, "  %s\t\t%s\n", cp->name, cp->summary );
  }
}

// eof
