#ifndef _VERNAMFS_CMDS_H
#define _VERNAMFS_CMDS_H

int mountArgs( int argc, char* argv[] );

int infoArgs( int argc, char* argv[] );

int infoFile( char* file );

int lsArgs( int argc, char* argv[] );

int lsFile( char* file );

int xlsArgs( int argc, char* argv[] );

int xlsFile( char* file );

#endif
