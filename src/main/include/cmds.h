#ifndef _VERNAMFS_CMDS_H
#define _VERNAMFS_CMDS_H

#include <inttypes.h>

extern char* helpUsage;
extern char* generateUsage;
extern char* initUsage;
extern char* infoUsage;
extern char* mountUsage;
extern char* rlsUsage;
extern char* vlsUsage;
extern char* rcatUsage;
extern char* vcatUsage;
extern char* recoverUsage;

extern char* generateSynopis;


int helpArgs( int argc, char* argv[] );

int initArgs( int argc, char* argv[] );

int init( char* file, int maxFiles, int maxFileNameLength, int force );

int infoArgs( int argc, char* argv[] );

int info( char* file );

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

// LOOK: what is a good name for the entire VFS recovery operation??

int recoverArgs( int argc, char* argv[] );

int recover( char* remoteOTP, char* vaultOTP, char* outputDir );

#endif
