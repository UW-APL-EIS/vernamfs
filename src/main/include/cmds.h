#ifndef _VERNAMFS_CMDS_H
#define _VERNAMFS_CMDS_H


int initArgs( int argc, char* argv[] );

int initFile( char* file, int tableSize );

int infoArgs( int argc, char* argv[] );

int infoFile( char* file );

int mountArgs( int argc, char* argv[] );

int lsArgs( int argc, char* argv[] );

int lsFile( char* file );

int xlsArgs( int argc, char* argv[] );

int xlsFile( char* file );

#endif
