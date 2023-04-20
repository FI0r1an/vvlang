/*
MIT License

Copyright (c) 2022 Cluck

	Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "vvcom.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

char *vv_strClone( const char *source )
{
	size_t len = strlen( source ) + 1;
	char *dest = ( char * ) calloc( len, sizeof( char ) );

	assert( dest );

	strcpy( dest, source );
	return dest;
}

char *vv_readFile( const char *fn )
{
	FILE *f = fopen( fn, "r" );

	if ( f )
	{
		fseek( f, 0, SEEK_END );
		size_t len = ( size_t ) ftell( f ) + 1;
		rewind( f );

		char *str = ( char * ) calloc( len, sizeof( char ) );
		if ( str == NULL )
			return 0;

		size_t cnt = 0;

		for ( ;; )
		{
			int c = fgetc( f );

			if ( c == EOF )
				break;

			str[ cnt++ ] = ( char ) c;
		}

		fclose( f );

		return str;
	}

	return NULL;
}

void vv_error( const char *msg, ... )
{
	va_list ap;

	va_start( ap, msg );

	vprintf( msg, ap );

	va_end( ap );
	abort( );
}
