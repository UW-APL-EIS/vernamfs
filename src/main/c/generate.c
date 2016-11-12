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
#include "vernamfs/aes128.h"

/**
 * @author Stuart Maclean
 *
 * The 'generate' subcommand is used to produce an approximation of a
 * truly random one-time-pad by using the AES symmetric block cipher.
 *
 * See e.g. https://en.wikipedia.org/wiki/Cryptographically_secure_pseudorandom_number_generator
 *
 * Advantages of the 'CPRNG' approach:
 *
 * 1: The 'vault copy' of the OTP does not need to be 'stored' at all.
 * Instead, it can be regenerated at any time, i.e. at remote unit
 * recovery time.  All that is needed is the original aes key, which
 * obviously MUST be kept secured.
 *
 * 2: It is likely much faster to generate a large one-time-pad,
 * e.g. gigabytes, using a CPRNG over a truly random source.
 *
 * Disadvantages of the CPRNG approach:
 *
 * 1: It's NOT truly random, and so is not in the strictest sense a
 * true OTP.
 *
 * Currently we support only the AES-128 variant of aes, since that is
 * the only code we snarfed off the net to add to VernamFS.  This has
 * a 128-bit key.  If/when we locate a nice, clean, small AES-256
 * implementation (256-bit key), we'll add.
 *
 * We use aes in 'counter mode', aka CTR mode.  We simply increment
 * the counter from 0, and keep counting until the desired length of
 * OTP has been produced.
 *
 * Example usage, using some simple Unix tools to generate the 128-bit
 * key.  The generate command expects the key on stdin, as a
 * hex-encoded string:
 *
 * $ echo "The cat sat on the mat" | md5sum | cut -b 1-32 > KEY
 *
 * // A 1MB OTP, 2^20
 * $ vernamfs generate 20 < KEY
 *
 * // A 1GB OTP, 2^30
 * $ vernamfs generate 30 < KEY
 *
 * The key can also be echoed straight into the generator:
 *
 * $ echo 12345678901234567890123456789012 | vernamfs generate 10
 *
 *
 * Some performance numbers, on ptrav2 (author's laptop)
 *
 * log2 size	generation time - secs
 *		20		< 1
 *		24		8
 *		28		120
 *		30		480 
 *		32		2040
 *
 * Since we are using CTR mode, in theory we could parallelize the
 * generation, given each job a subsequence of the counter space.  In
 * practice, the aes code as is seems quick enough to be run
 * sequentially.
 */
static int hexDecode( uint8_t* encoded, int len, uint8_t* result );

static CommandOption z = { .id = "z", .text = "Key is 16 zero bytes." };

static CommandOption* options[] = { &z, NULL };

static char example1[] = 
  "$ echo \"The cat sat on the mat\" | md5sum | cut -b 1-32 > KEY";

static char example2[] = 
  "$ vernamfs generate 16 < KEY > 64K.pad";

static char example3[] = "$ cat KEY | vernamfs generate 20 > 1MB.pad";

static char example4[] = "$ echo 12345678901234567890123456789012 | vernamfs generate 30 > 1GB.pad";

static char example5[] = "$ vernamfs generate -z 24 > 16MB.pad";

static char* examples[] = { example1, example2, example3, example4, 
							example5, NULL };

static CommandHelp help = {
  .summary = "Generate a pseudo one-time pad, using AES128 block cipher",

  .synopsis = "[<options>] log2PadSize",

  .description = "Generate a pseudo one-time pad, using the AES128 block cipher, in CTR mode.\n  This is likely faster than reading /dev/[u]random.  It can be regenerated at\n  will, so no vault copy need be stored. It is of course not random.\n\n  log2PadSize is the base 2 log of the desired pad size. For a 1MB pad, use 20,\n  for 1GB, use 30, etc.  Minimum is 12.\n\n  The 16-byte AES key is expected, hex-encoded, on standard input, unless the \n  -z option is used.\n\n  Pad content is written to standard output, so redirect to a suitable file.",

  .options = options,

  .examples = examples

};

Command generateCmd = {
  .name = "generate",
  .help = &help,
  .invoke = generateArgs
};

/**
 * The aes key expected in hex-encoded form on STDIN.  If a trailing
 * newline character (\n) is found, it is removed.  If the -z option
 * is given, no user key is required, and a zeroed key (N bits all
 * zero) is used, useful in testing.
 *
 * The log2 of the desired OTP length expected as sole mandatory
 * command argument.  So 20 produces a 1MB stream, 30 produces a 1GB
 * stream, etc.
 */

int generateArgs( int argc, char* argv[] ) {

  int log2OTPSize = 0;

  uint8_t userKey[32] = { 0 };
  uint8_t zeroKey[32] = { 0 };

  int keyLen = 0;
  uint8_t* key = NULL;

  int c;
  while( (c = getopt( argc, argv, "z") ) != -1 ) {
	switch( c ) {
	case 'z':
	  key = zeroKey;
	  keyLen = 16;
	  break;
	default:
	  break;
	}
  }

  if( optind+1 > argc ) {
	commandHelp( &generateCmd );
	return -1;
  }

  log2OTPSize = atoi( argv[optind] );
  if( log2OTPSize < 12 || log2OTPSize > 40 ) {
	fprintf( stderr, "%s: Size out-of-bounds: 12 <= log2PadSize <= 40\n", 
			 argv[0] );
	return -1;
  }
  
  if( !key ) {
	uint8_t keyHex[128];
	int nin = read( STDIN_FILENO, keyHex, 32 );
	if( nin < 32 ) {
	  fprintf( stderr, 
			   "%s: Hexed key (length %d) too short. Need 32 hex digits.\n", 
			   argv[0], nin );
	  return -1;
	}
	
	// Crude but effective way to strip whitespace!
	if( keyHex[nin-1] == '\n' )
	  nin--;
	
	keyLen = hexDecode( (uint8_t*)keyHex, nin, userKey );
	key = userKey;
  }

  switch( keyLen ) {

  case 16:
	generate128( (char*)key, log2OTPSize );
	break;

  case 32:
	// LOOK: find an aes256 implementation
	fprintf( stderr, 
			 "%s: AES256 not (yet) supported, use 16-byte key.\n", 
			 argv[0] );
	break;

  default:
	fprintf( stderr, "%s: key length %d not supported\n", argv[0], keyLen );
	return -1;
  }

  return 0;
}

int generate128( char key[], int log2OTPSize ) {

  uint8_t input[16] = { 0 };

  uint8_t output[16];

  uint64_t sz = (uint64_t)(1LL << log2OTPSize);

  // AES outputs 16 bytes at a time...
  uint64_t iterations = sz >> 4;

  /* 
	 The mode of operation here is 'counter mode' aka CTR.
	 We just set the input to a counter, starting at 0.
  */
  uint64_t i;
  for( i = 0; i < iterations; i++ ) {
	uint64_t* ip = (uint64_t*)input;
	*ip = i;
	AES128_ECB_encrypt( input, (uint8_t*)key, output );
	write( STDOUT_FILENO, output, 16 );
  }

  return 0;
}


static int hexDecode( uint8_t* encoded, int len, uint8_t* result ) {

  uint8_t HEXDECODE[256];

  HEXDECODE['0'] = 0x0;
  HEXDECODE['1'] = 0x1;
  HEXDECODE['2'] = 0x2;
  HEXDECODE['3'] = 0x3;
  HEXDECODE['4'] = 0x4;
  HEXDECODE['5'] = 0x5;
  HEXDECODE['6'] = 0x6;
  HEXDECODE['7'] = 0x7;
  HEXDECODE['8'] = 0x8;
  HEXDECODE['9'] = 0x9;
  HEXDECODE['a'] = 0xa;
  HEXDECODE['A'] = 0xa;
  HEXDECODE['b'] = 0xb;
  HEXDECODE['B'] = 0xb;
  HEXDECODE['c'] = 0xc;
  HEXDECODE['C'] = 0xc;
  HEXDECODE['d'] = 0xd;
  HEXDECODE['D'] = 0xd;
  HEXDECODE['e'] = 0xe;
  HEXDECODE['E'] = 0xe;
  HEXDECODE['f'] = 0xf;
  HEXDECODE['F'] = 0xf;
  
  int i;
  for( i = 0; i < len/2; i++ ) {
	uint8_t uch1 = (uint8_t)encoded[2*i];
	uint8_t uch2 = (uint8_t)encoded[2*i+1];
	result[i] = (HEXDECODE[uch1] << 4) | HEXDECODE[uch2];
  }
  return len/2;
}

// eof
