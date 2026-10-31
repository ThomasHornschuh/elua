#ifndef ELUA_STDIO_H
#define ELUA_STDIO_H

#include <stdio.h>

/*
 * Open an eLua device-manager path as a stdio stream. On host builds this
 * also accepts regular Linux paths, which are delegated to the host libc.
 */
FILE* elua_fopen( const char* name, const char* mode );
FILE* elua_freopen( const char* name, const char* mode, FILE* stream );

#endif
