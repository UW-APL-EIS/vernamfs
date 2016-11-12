/**
 * Copyright © 2016, University of Washington
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of the University of Washington nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL UNIVERSITY OF
 * WASHINGTON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "vernamfs/cmds.h"

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
