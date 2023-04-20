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

#include "vvlex.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vvcom.h"

static const char *const reservedWords[] = {
	"?",
	"EOF",
	"IDENTIFIER",
	"NUMBER",
	"STRING",

	"BASIC_END",

	":=",
	":",
	";",
	"==",
	"!=",
	">=",
	">",
	"<=",
	"<",
	"+",
	"-",
	"*",
	"/",
	"&&",
	"||",
	"!",

	",",
	"(",
	")",
	"[",
	"]",
	"{",
	"}",
	"$",

	"KW_START",

	"def",
	"when",
	"while",
	"partial",
	"return",

	"true",
	"false",
	"nil",

	"KW_END",
};

const char *vv_tkToString( vvToken tk )
{
	return reservedWords[ tk.tt >= TT_NONE && tk.tt <= TT_KW_END ? tk.tt : TT_NONE ];
}

static unsigned int DJBHash( char *str )
{
	unsigned int hash = 5381;

	while ( *str )
	{
		hash = ( ( hash << 5 ) + hash ) + ( *str );

		str++;
	}

	return hash;
}

vvLexTable *vv_newLexTable( )
{
	vvLexTable *rsl = ( vvLexTable * ) malloc( sizeof( vvLexTable ) );
	assert( rsl );

	rsl->size = 0;
	rsl->capacity = VV_LEX_TABLE_DEFAULT_LEN;

	rsl->sto = ( char ** ) calloc( VV_LEX_TABLE_DEFAULT_LEN, sizeof( char * ) );
	assert( rsl->sto );
	rsl->hashSto = ( unsigned * ) malloc( VV_LEX_TABLE_DEFAULT_LEN * sizeof( unsigned ) );
	assert( rsl->hashSto );

	return rsl;
}

vvString vv_lexTableAdd( vvLexTable *tbl, char *str )
{
	unsigned strHash = DJBHash( str );

	for ( size_t i = 0; i < tbl->size; i++ )
	{
		unsigned tblHash = tbl->hashSto[ i ];
		if ( tblHash == strHash )
		{
			return ( vvString ) i;
		}
	}

	if ( tbl->size >= tbl->capacity )
	{
		tbl->capacity += VV_LEX_TABLE_STEP;

		void *temp;

		temp = realloc( tbl->sto, tbl->capacity * sizeof( char * ) );
		assert( temp );
		tbl->sto = ( char ** ) temp;

		temp = realloc( tbl->hashSto, tbl->capacity * sizeof( unsigned ) );
		assert( temp );
		tbl->hashSto = ( unsigned * ) temp;
	}

	// avoid warning
	*( tbl->sto + tbl->size ) = str;
	*( tbl->hashSto + tbl->size ) = strHash;

	return tbl->size++;
}

void vv_freeLexTable( vvLexTable *tbl )
{
	for ( size_t i = 0; i < tbl->size; i++ )
	{
		free( tbl->sto[ i ] );
	}

	free( tbl->sto );
	free( tbl->hashSto );

	free( tbl );
}

vvLexer *vv_newLexer( char *input, char *fn, size_t bufLen, vvLexTable *tbl )
{
	vvLexer *rsl = ( vvLexer * ) malloc( sizeof( vvLexer ) );
	assert( rsl );

	rsl->bufIdx = rsl->inputIdx = 0;
	rsl->fileName = fn;
	rsl->input = input;
	rsl->chrBuf = ( char * ) calloc( bufLen, sizeof( char ) );
	rsl->bufLen = bufLen;
	rsl->tbl = tbl;

	rsl->row = rsl->col = 1;

	return rsl;
}

void vv_freeLexer( vvLexer *lex )
{
	free( lex->chrBuf );
	free( lex );
}

#define lcurr( lex ) ( lex->input[ lex->inputIdx ] )
#define llookahead( lex ) ( lcurr( lex ) == '\0' ? '\0' : lex->input[ lex->inputIdx + 1 ] )

static char lnext( vvLexer *lex )
{
	char cur = lcurr( lex );

	if ( cur == '\r' || cur == '\n' )
	{
		lex->row++;
		lex->col = 1;
		char nex = llookahead( lex );
		if ( ( nex == '\r' || nex == '\n' ) && nex != cur )
			lex->inputIdx++;
	}
	else
	{
		lex->col++;
	}

	if ( cur != '\0' )
		lex->inputIdx++;

	return cur;
}

static void lwrite( vvLexer *lex, char c )
{
	if ( lex->bufIdx < lex->bufLen )
		lex->chrBuf[ lex->bufIdx++ ] = c;
	else
		vv_error( "[%s %zd:%zd] Too long content", lex->fileName, lex->row, lex->col );
}

static vvToken lsave( vvLexer *lex, vvTokenType tt, size_t row, size_t col )
{
	char *rsl = ( char * ) calloc( lex->bufIdx + 1, sizeof( char ) );
	assert( rsl );

	for ( size_t i = 0; i < lex->bufIdx; i++ )
	{
		rsl[ i ] = lex->chrBuf[ i ];
	}

	lex->bufIdx = 0;

	if ( isalpha( *rsl ) )
	{
		for ( size_t i = TT_KW_START + 1; i < TT_KW_END; i++ )
		{
			if ( !strcmp( rsl, reservedWords[ i ] ) )
				return ( vvToken ){
					.tt = i,
					.val = 0,
					.row = row,
					.col = col,
				};
		}
	}

	size_t presize = lex->tbl->size;
	vvString str = vv_lexTableAdd( lex->tbl, rsl );

	if ( presize == lex->tbl->size )
		free( rsl );

	return ( vvToken ){
		.tt = tt,
		.val = str,
		.row = row,
		.col = col,
	};
}

static vvToken readString( vvLexer *lex )
{
	char sign = lnext( lex );
	lwrite( lex, '$' );

	size_t row = lex->row, col = lex->col;

	for ( ;; )
	{
		char c = lcurr( lex );

		if ( c == sign )
			break;
		if ( c == '\0' )
			vv_error( "[%s EOF] Expected '%c'", lex->fileName, sign );
		if ( c == '\r' || c == '\n' )
			vv_error( "[%s %zd:%zd] Unexpected new line", lex->fileName, lex->row, lex->col - 1 );

		lwrite( lex, c );
		lnext( lex );
	}

	lnext( lex );
	return lsave( lex, TT_STRING, row, col );
}

static vvToken readNumber( vvLexer *lex )
{
	int meetPoint = 0;

	size_t row = lex->row, col = lex->col;

	while ( isalnum( lcurr( lex ) ) || lcurr( lex ) == '.' || lcurr( lex ) == '_' )
	{
		if ( lcurr( lex ) == '.' )
		{
			if ( meetPoint )
			{
				vv_error( "[%s %zd:%zd] Unexpected character '.'", lex->fileName, lex->row, lex->col );
			}
			meetPoint = 1;
		}
		if ( lcurr( lex ) != '_' )
			lwrite( lex, lcurr( lex ) );
		lnext( lex );
	}

	return lsave( lex, TT_NUMBER, row, col );
}

static vvToken readAlpha( vvLexer *lex )
{
	if ( !isalpha( lcurr( lex ) ) )
		vv_error( "[%s %zd:%zd] Unexpected character '%c'", lex->fileName, lex->row, lex->col, lcurr( lex ) );

	size_t row = lex->row, col = lex->col;

	while ( isalpha( lcurr( lex ) ) || lcurr( lex ) == '_' )
	{
		lwrite( lex, lnext( lex ) );
	}

	return lsave( lex, TT_IDENTIFIER, row, col );
}

#define retSingle( op ) \
	lnext( lex );       \
	return ( vvToken )  \
	{                   \
		.tt = op,       \
		.val = 0,       \
		.row = row,     \
		.col = col,     \
	}

#define retTwo( sec, op )                                                           \
	if ( nex != sec )                                                               \
		vv_error( "[%s %zd:%zd] Expected '%c'", lex->fileName, row, col + 1, sec ); \
	lnext( lex ), lnext( lex );                                                     \
	return ( vvToken ){                                                             \
		.tt = op,                                                                   \
		.val = 0,                                                                   \
		.row = row,                                                                 \
		.col = col,                                                                 \
	};

#define retSingleOrTwo( op1, sec, op2 ) \
	lnext( lex );                       \
	if ( nex == sec )                   \
	{                                   \
		lnext( lex );                   \
		return ( vvToken ){             \
			.tt = op2,                  \
			.val = 0,                   \
			.row = row,                 \
			.col = col,                 \
		};                              \
	}                                   \
	return ( vvToken ){                 \
		.tt = op1,                      \
		.val = 0,                       \
		.row = row,                     \
		.col = col,                     \
	};

vvToken vv_lexerRead( vvLexer *lex )
{
	for ( ;; )
	{
		size_t row = lex->row, col = lex->col;

		char cur = lcurr( lex ), nex = llookahead( lex );

		switch ( cur )
		{
			case '\0':
				return ( vvToken ){
					.tt = TT_EOF,
					.val = 0,
					.row = row,
					.col = col,
				};
			case '\r':
			case '\n':
			case ' ':
			case '\t':
				lnext( lex );
				continue;
			case '#':
				if ( llookahead( lex ) == '#' )
				{
					for ( ;; )
					{
						lnext( lex );
						cur = lcurr( lex );

						if ( cur == '#' && llookahead( lex ) == '#' )
						{
							lnext( lex );
							lnext( lex );
							break;
						}
						if ( cur == '\0' )
						{
							vv_error( "[%s EOF] Expected \"##\"", lex->fileName );
							break;
						}
					}
				}
				else
				{
					for ( ;; )
					{
						lnext( lex );
						cur = lcurr( lex );

						if ( cur == '\r' || cur == '\n' || cur == '\0' )
							break;
					}
				}
				continue;
			case '\'':
			case '"':
				return readString( lex );
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				goto number;
			case '$':
				retSingle( TT_MONEY );
			case '+':
				retSingle( TT_ADD );
			case '-':
				retSingle( TT_MINUS );
			number:
				return readNumber( lex );
			case ':':
				retSingleOrTwo( TT_COLON, '=', TT_IS );
			case ';':
				retSingle( TT_SEMICOLON );
			case '=':
				if ( nex == '=' )
				{
					lnext( lex );
					retSingle( TT_EQ );
				}
				vv_error( "[%s %zd:%zd] Unexpected character '='", lex->fileName, row, col );
				break;
			case '!':
				retSingle( TT_NOT );
			case '>':
				retSingleOrTwo( TT_GT, '=', TT_GE );
			case '<':
				retSingleOrTwo( TT_LT, '=', TT_LE );
			case '*':
				retSingle( TT_MUL );
			case '/':
				retSingle( TT_DIV );
			case '&':
				retTwo( '&', TT_AND );
			case '|':
				retTwo( '|', TT_OR );
			case ',':
				retSingle( TT_COMMA );
			case '(':
				retSingle( TT_LPAREN );
			case ')':
				retSingle( TT_RPAREN );
			case '{':
				retSingle( TT_LBRACE );
			case '}':
				retSingle( TT_RBRACE );
			default:
				return readAlpha( lex );
		}
	}

	return ( vvToken ){ 0 };
}
