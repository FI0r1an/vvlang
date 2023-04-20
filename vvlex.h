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

#pragma once

#ifndef VV_LEXER
#define VV_LEXER

#include <stdlib.h>
#include <stdio.h>

typedef enum vvTokenType
{
	TT_NONE = 0,
	TT_EOF,
	TT_IDENTIFIER,
	TT_NUMBER,
	TT_STRING,

	TT_BASIC_END,
	
	TT_IS,
	TT_COLON,
	TT_SEMICOLON,
	TT_EQ,
	TT_NEQ,
	TT_GE,
	TT_GT,
	TT_LE,
	TT_LT,
	TT_ADD,
	TT_MINUS,
	TT_MUL,
	TT_DIV,
	TT_AND,
	TT_OR,
	TT_NOT,

	TT_COMMA,
	TT_LPAREN,
	TT_RPAREN,
	TT_LBRACKET,
	TT_RBRACKET,
	TT_LBRACE,
	TT_RBRACE,
	TT_MONEY,

	TT_KW_START,

	TT_DEF,
	TT_WHEN,
	TT_WHILE,
	TT_PARTIAL,
	TT_RETURN,
	
	TT_TRUE,
	TT_FALSE,
	TT_NIL,

	TT_KW_END
} vvTokenType;

#define VV_LEX_TABLE_DEFAULT_LEN 16
#define VV_LEX_TABLE_STEP 2

typedef size_t vvString;

typedef struct vvLexTable
{
	size_t size, capacity;

	char **sto;
	unsigned *hashSto;
} vvLexTable;
#define vv_lexTableGet( tbl, idx ) ( tbl->sto[ idx ] )

vvLexTable *vv_newLexTable( );
vvString vv_lexTableAdd( vvLexTable *tbl, char *str );
void vv_freeLexTable( vvLexTable *tbl );

typedef struct vvToken
{
	vvTokenType tt;
	vvString val;
	size_t row, col;
} vvToken;

const char *vv_tkToString( vvToken tk );

#define VV_CHAR_BUFFER_DEFAULT_LEN 31

typedef struct vvLexer
{
	char *input;
	size_t inputIdx, bufIdx, row, col, bufLen;
	char *chrBuf, *fileName;

	vvLexTable *tbl;
} vvLexer;

vvLexer *vv_newLexer( char *input, char *fn, size_t bufLen, vvLexTable *tbl );
void vv_freeLexer( vvLexer *lex );
vvToken vv_lexerRead( vvLexer *lex );

#endif