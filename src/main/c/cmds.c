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
#include <string.h>

#include "vernamfs/cmds.h"

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
