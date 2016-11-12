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
#ifndef _VERNAMFS_CMDS_H
#define _VERNAMFS_CMDS_H

#include <inttypes.h>

extern char* ProgramName;

typedef struct {
  char* id;
  char* text;
} CommandOption;
    
typedef struct {
  char* summary;
  char* synopsis;
  char* description;
  CommandOption** options;
  char** examples;
} CommandHelp;  

typedef struct {
  char* name;
  CommandHelp* help;
  int (*invoke)( int argc, char* argv[] );
} Command;

extern Command** cmds;
extern int N;

extern Command helpCmd;
extern Command generateCmd;
extern Command initCmd;
extern Command infoCmd;
extern Command mountCmd;
extern Command rlsCmd;
extern Command vlsCmd;
extern Command rcatCmd;
extern Command vcatCmd;
extern Command recoverCmd;

Command* commandLocate( char* name );

void commandsSummary( char result[] );

void commandHelp( Command* c );

int helpArgs( int argc, char* argv[] );

int initArgs( int argc, char* argv[] );

int init( char* file, int maxFiles, int maxFileNameLength,
	  int force, int expert );

int infoArgs( int argc, char* argv[] );

int info( char* file, int expert );

int mountArgs( int argc, char* argv[] );

int rlsArgs( int argc, char* argv[] );

int rls( char* file );

int vlsArgs( int argc, char* argv[] );

/*
 * @param raw - if TRUE, print the actual table as raw, suitable for formatting
 * by e.g. xxd.  If FALSE, print the table in human-readable form.
 *
 * @param rlsResultOptional - file with rls content, or NULL for stdin
 */
int vls( char* file, int raw, char* rlsResultOptional );

int rcatArgs( int argc, char* argv[] );

int rcat( char* file, uint64_t offset, uint64_t length );

int vcatArgs( int argc, char* argv[] );

int vcat( char* file, char* rcatResult, char* rlsResultOptional );

int generateArgs( int argc, char* argv[] );

// Using aes/ctr mode with a 128-bit key to generate OTP
int generate128( char key[], int log2OTPSize );

// LOOK: what is a good/better name for the entire VFS recovery operation??
int recoverArgs( int argc, char* argv[] );

int recover( char* remoteOTP, char* vaultOTP, char* outputDir );

#endif
