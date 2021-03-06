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
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "vernamfs/cmds.h"
#include "vernamfs/vernamfs.h"
#include "vernamfs/remote.h"

/**
 * @author Stuart Maclean
 *
 * The 'remote' rls listing expected as a file, or on STDIN.  It is of
 * course unintelligible since it is still XOR'ed with the OTP.
 *
 * The 'vaultFile' is the local, pristine copy of the original OTP.
 *
 * We can recover the logical remote table contents, and thus see a
 * listing of remote operations, by XOR'ing the actual remote table
 * and vault table together.
 *
 * An example test, using rls result in a pipe, no file:
 *
 * $ ./vernamfs rls 1M.R| ./vernamfs vls 1M.V
 *
 * Normally the rls result would be a file, shipped from remote to
 * vault location.
 *
 * The vls result is a printout, to stdout, of all allocated file
 * names with their associated offset and length within the remote OTP.
 *
 * If the raw printout is required (a -r option to vls), the table
 * entries are printed in raw form, i.e the results of the XOR
 * operation.  This is useful for demonstration purposes, since it can
 * be piped to e.g. xxd and compared to the rls result also piped to
 * xxd.
 *
 * @see rls.c
 */

static CommandOption r = 
  { .id = "r", 
	.text = "Print raw file table content. Default is readable listing." };

static CommandOption* options[] = { &r, NULL };

static char example1[] = 
  "$ vernamfs vls OTP.vault rlsResult";

static char example2[] = 
  "$ vernamfs vls -r OTP.vault rlsResult";

static char example3[] = 
  "$ cat rlsResult | vernamfs vls OTP.vault";

static char example4[] = 
  "$ vernamfs rls OTP.remote | vernamfs vls OTP.vault";

static char* examples[] = { example1, example2, example3, example4, NULL };

static CommandHelp help = {
  .summary ="Recover encrypted file listing",
  .synopsis = "[<options>] OTPVAULT rlsResultFile|STDIN",
  .description = "Recover remote file listing, showing name and size of each allocated file.\n  Does this by combining remote ls result and local vault copy of the OTP.\n  Remote ls result supplied as a file, or on stdin.",
  .options = options,
  .examples = examples,
};

Command vlsCmd = {
  .name = "vls",
  .help = &help,
  .invoke = vlsArgs
};

int vlsArgs( int argc, char* argv[] ) {

  int raw = 0;
  char* vaultFile = NULL;
  char* rlsResult = NULL;
  
  int c;
  while( (c = getopt( argc, argv, "r") ) != -1 ) {
	switch( c ) {
	case 'r':
	  raw = 1;
	  break;
	default:
	  break;
	}
  }

  if( optind < argc ) {
	vaultFile = argv[optind];
  } else {
	commandHelp( &vlsCmd );
	return -1;
  }

  if( optind+1 < argc ) 
	rlsResult = argv[optind+1];

  // printf( "raw %d, vaultFile %s, rlsResult %p\n", raw, vaultFile,rlsResult );

  return vls( vaultFile, raw, rlsResult );
}

/*
  The 'remote ls' is expected as a file, or on STDIN.  It would have
  been obtained via an 'rls' command, and shipped to the 'vault'
  location.  
*/

int vls( char* vaultFile, int raw, char* rlsResult ) {

  struct stat st;
  int sc = stat( vaultFile, &st );
  if( sc || !S_ISREG( st.st_mode ) ) {
	fprintf( stderr, "%s: Not a regular file\n", vaultFile );
	return -1;
  }
  off_t vaultLength = st.st_size;

  int fdRls = STDIN_FILENO;
  if( rlsResult ) {
	fdRls = open( rlsResult, O_RDONLY );
	if( fdRls < 0 ) {
	  fprintf( stderr, "Cannot open rlsResult: %s\n", rlsResult );
	  return -1;
	}
  }
  
  VFSRemoteResult* rrls = VFSRemoteResultRead( fdRls );
  if( rlsResult )
	close( fdRls );

  if( rrls->offset + rrls->length > vaultLength ) {
	fprintf( stderr, "Vault length (%"PRIx64") too short, need %"PRIx64"\n",
			 vaultLength, rrls->offset + rrls->length );
	VFSRemoteResultFree( rrls );
	free( rrls );
	return -1;
  }

  // printf( "Off %x, len %x\n", rlsOffset, rlsLength );

  // Possible that the remote FS be currently empty
  if( rrls->length == 0 ) {
	VFSRemoteResultFree( rrls );
	free( rrls );
	return -1;
  }
  
  int fd = open( vaultFile, O_RDONLY );
  if( fd < 0 ) {
	fprintf( stderr, "Cannot open vaultFile: %s\n", vaultFile );
	return -1;
  }
  
  // Was trying length, offset related to rls info, not working...
  int mappedLength = vaultLength;
  int mappedOffset = 0;

  void* addr = mmap( NULL, mappedLength, PROT_READ, MAP_PRIVATE, 
					 fd, mappedOffset );
  if( addr == MAP_FAILED ) {
	fprintf( stderr, "Cannot mmap vaultFile: %s\n", vaultFile );
	close( fd );
	VFSRemoteResultFree( rrls );
	free( rrls );
	return -1;
  }

  VFS vaultVFS;
  VFSLoad( &vaultVFS, addr );

  // LOOK: Can/should check vaultVFS.header.tableOffset == vrr->offset

  /*
	Assumed that the vault header's tableEntrySize matches that of the
	remote data
  */
  int tableEntrySize = vaultVFS.header.tableEntrySize;
  int tableEntryCount = rrls->length / tableEntrySize;
  char* teActual = (char*)malloc( tableEntrySize );
  char* rls = rrls->data;
  char* vls = (char*)(addr + rrls->offset);
  int i;
  for( i = 0; i < tableEntryCount; i++ ) {
	char* teRemote = rls + i * tableEntrySize;
	char* teVault =  vls + i * tableEntrySize;
	int j;
	for( j = 0; j < tableEntrySize; j++ )
	  teActual[j] = teRemote[j] ^ teVault[j];
	VFSTableEntryFixed* tef = (VFSTableEntryFixed*)teActual;
	char* name = teActual + sizeof( VFSTableEntryFixed );

	/*
	  The vls result, just a print out each file's properties,
	  in either raw form or formatted
	*/
	if( raw ) {
	  write( STDOUT_FILENO, teActual, tableEntrySize ); 
	} else {
	  printf( "%s 0x%"PRIx64" 0x%"PRIx64"\n", 
			  name, tef->offset, tef->length );
	}
  }
  free( teActual );

  VFSRemoteResultFree( rrls );
  free( rrls );

  munmap( addr, mappedLength );
  close( fd );

  return 0;
}

// eof
