#ifndef _VERNAMFS_CMDS_H
#define _VERNAMFS_CMDS_H

#include <inttypes.h>

int initArgs( int argc, char* argv[] );

int initFile( char* file, int maxFiles, int maxFileNameLength );

int infoArgs( int argc, char* argv[] );

int infoFile( char* file );

int mountArgs( int argc, char* argv[] );

int rlsArgs( int argc, char* argv[] );

int rlsFile( char* file );

int vlsArgs( int argc, char* argv[] );

/*
 * @param rlsResultOptional - file with rls content, or NULL for stdin
 */
int vlsFile( char* file, char* rlsResultOptional );

int rcatArgs( int argc, char* argv[] );

int rcatFile( char* file, uint64_t offset, uint64_t length );

int vcatArgs( int argc, char* argv[] );

int vcatFile( char* file, char* rcatResult, char* rlsResultOptional );

// LOOK: what is a good name for the entire VFS recovery operation??
int recover( char* remoteOTP, char* vaultOTP );

#endif
