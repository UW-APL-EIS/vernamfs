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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Investigating the use of atoi, strtol and how they parse input strings.
 */

static void test8(void) {

  printf( "%s\n", __FUNCTION__ );

  // This LOOKS like octal, but atoi sees this as decimal!
  char* s1 = "0100";
  int i1 = atoi( s1 );
  assert( i1 == 100 );
}

static void test10(void) {

  printf( "%s\n", __FUNCTION__ );

  char* s1 = "100";
  int i1 = atoi( s1 );
  assert( i1 == 100 );

  char* s2 = " 100";
  int i2 = atoi( s2 );
  assert( i2 == 100 );

  // atoi will NOT correctly parse a hex number, gives 0
  char* s3 = "0xff";
  int i3 = atoi( s3 );
  printf( "%s -> %d\n", s3, i3 );
  assert( i3 == 0 );
}

static void test16(void) {

  printf( "%s\n", __FUNCTION__ );

  char* s1 = "0xff";
  int i1 = strtol( s1, NULL, 16 );
  printf( "%s -> %d\n", s1, i1 );
  assert( i1 == 255 );

  // strtol will NOT correctly non-hex digits, gives 0
  char* s2 = "0xXX";
  int i2 = strtol( s2, NULL, 16 );
  printf( "%s -> %d\n", s2, i2 );
  assert( i2 == 0 );
}

int parseAny( char* s ) {
  if( strlen( s ) >= 2 && s[0] == '0' &&
	  (s[1] == 'X' || s[1] == 'x' ) ) {
	return strtol( s, NULL, 16 );
  }
  return atoi( s );
}

static void testBoth(void) {

  printf( "%s\n", __FUNCTION__ );

  char* s1 = "0xff";
  int i1 = parseAny( s1 );
  assert( i1 == 255 );

  char* s3 = "0x10000";
  int i3 = parseAny( s3 );
  assert( i3 == (1 << 16) );

  char* s2 = " 100";
  int i2 = parseAny( s2 );
  assert( i2 == 100 );

  // With no trailing data after the 0x, will get 0
  char* s4 = "0x";
  int i4 = parseAny( s4 );
  assert( i4 == 0 );

  // With no trailing data after left-padded 0x, will get 0
  char* s5 = "  0x";
  int i5 = parseAny( s5 );
  assert( i5 == 0 );
}

int main( int argc, char* argv[] ) {

  test8();

  test10();

  test16();

  testBoth();

  return 0;

}

// eof
