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

#ifndef VV_PARSER
#define VV_PARSER

#include "vvlex.h"

#include <stdlib.h>
#include <stdio.h>

typedef enum vvSyntaxType
{
	ST_NONE,

	ST_BLOCK,

	ST_DEF_STMT,
	ST_DEF_PARTIAL_STMT,
	ST_WHEN_STMT,
	ST_WHILE_STMT,
	ST_RET_STMT,

	ST_ASSIGN_EXPR,
	ST_FUNC_EXPR,
	ST_CALL_EXPR,
	ST_EQ_EXPR,
	ST_NEQ_EXPR,
	ST_GE_EXPR,
	ST_GT_EXPR,
	ST_LE_EXPR,
	ST_LT_EXPR,
	ST_ADD_EXPR,
	ST_SUB_EXPR,
	ST_MUL_EXPR,
	ST_DIV_EXPR,
	ST_AND_EXPR,
	ST_OR_EXPR,
	ST_NOT_EXPR,
	ST_INV_EXPR,

	ST_ARG_LIST,

	ST_PRIMARY,
} vvSyntaxType;

struct vvSyntaxContainer;

#define VV_PARSER_MAX_CHILDREN 3

typedef struct vvSyntaxContainer
{
	vvSyntaxType st;

	struct vvSyntaxContainer *next;
	struct vvSyntaxContainer *children[ VV_PARSER_MAX_CHILDREN ];

	union
	{
		vvToken tk;
		int flag;
	} attr;
} vvSyntaxContainer;

typedef struct vvParser
{
	vvLexer *lex;
	vvToken tkBuf;
} vvParser;

vvParser *vv_newParser( vvLexer *lex );
void vv_freeParser( vvParser *p );
vvSyntaxContainer *vv_newSyntaxContainer( );
void vv_freeSyntaxContainer( vvSyntaxContainer *syn );

void parseStatement( vvParser *p, vvSyntaxContainer *rsl );
void parseBlock( vvParser *p, vvSyntaxContainer *rsl );
void parseExpr( vvParser *p, vvSyntaxContainer *rsl );

#endif