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
#include "vvparser.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vvlex.h"
#include "vvcom.h"

vvParser *vv_newParser( vvLexer *lex )
{
	vvParser *rsl = ( vvParser * ) malloc( sizeof( vvParser ) );
	assert( rsl );

	rsl->lex = lex;
	rsl->tkBuf.tt = TT_NONE;

	return rsl;
}

void vv_freeParser( vvParser *p )
{
	free( p );
}

vvSyntaxContainer *vv_newSyntaxContainer( )
{
	vvSyntaxContainer *rsl = ( vvSyntaxContainer * ) malloc( sizeof( vvSyntaxContainer ) );
	assert( rsl );

	rsl->st = ST_NONE;
	rsl->next = NULL;
	rsl->children[ 0 ] = rsl->children[ 1 ] = rsl->children[ 2 ] = NULL;

	return rsl;
}

void vv_freeSyntaxContainer( vvSyntaxContainer *syn )
{
	if ( syn == NULL )
		return;

	vv_freeSyntaxContainer( syn->children[ 0 ] );
	vv_freeSyntaxContainer( syn->children[ 1 ] );
	vv_freeSyntaxContainer( syn->children[ 2 ] );
	free( syn );
}

#define pcurr( ) \
	( p->tkBuf.tt == TT_NONE ? ( p->tkBuf = vv_lexerRead( p->lex ) ) : ( p->tkBuf ) )
#define pnext( ) \
	( p->tkBuf = vv_lexerRead( p->lex ) )
#define pexpectg( t, msg )                                      \
	if ( pcurr( ).tt == t )                                     \
		pnext( );                                               \
	else                                                        \
		vv_error( "[%s %zd:%zd] Expected %s, got \"%s\"",       \
				  p->lex->fileName, p->tkBuf.row, p->tkBuf.col, \
				  msg, vv_tkToString( p->tkBuf ) );

void parseAssignExpr( vvParser *p, vvSyntaxContainer *rsl );
void parseDefStmt( vvParser *p, vvSyntaxContainer *rsl );
void parseWhenStmt( vvParser *p, vvSyntaxContainer *rsl );
void parseWhileStmt( vvParser *p, vvSyntaxContainer *rsl );
void parseRetStmt( vvParser *p, vvSyntaxContainer *rsl );
void parseBlock( vvParser *p, vvSyntaxContainer *rsl );
void parseExpr( vvParser *p, vvSyntaxContainer *rsl );
void parseFunExpr( vvParser *p, vvSyntaxContainer *rsl );
void parseCallExpr( vvParser *p, vvSyntaxContainer *rsl );

void parseStatement( vvParser *p, vvSyntaxContainer *rsl )
{
	vvToken tk = pcurr( );

	while ( 1 )
	{
		switch ( tk.tt )
		{
			case TT_EOF:
				return;
			case TT_SEMICOLON: {
				tk = pnext( );
				continue;
			}
			case TT_LBRACE:
				parseBlock( p, rsl );
				break;
			case TT_IDENTIFIER:
				parseAssignExpr( p, rsl );
				break;
			case TT_DEF:
				parseDefStmt( p, rsl );
				break;
			case TT_WHEN:
				parseWhenStmt( p, rsl );
				break;
			case TT_WHILE:
				parseWhileStmt( p, rsl );
				break;
			case TT_RETURN:
				parseRetStmt( p, rsl );
				break;
			case TT_MONEY:
				parseCallExpr( p, rsl );
				break;
			default:
				vv_error( "[%s %zd:%zd] Unrecognised token", p->lex->fileName, tk.row, tk.col );
				break;
		}
		break;
	}
}

static int BINARY_PRIORITY[][ 2 ] = {
	{ TT_MUL, 10 },
	{ TT_DIV, 10 },
	{ TT_ADD, 9 },
	{ TT_MINUS, 9 },
	{ TT_GE, 8 },
	{ TT_GT, 8 },
	{ TT_LE, 8 },
	{ TT_LT, 8 },
	{ TT_EQ, 7 },
	{ TT_NEQ, 7 },
	{ TT_AND, 6 },
	{ TT_OR, 5 },
}; // 12 elements

#define UNARY_PRIORITY 20

int getPriority( vvTokenType tt )
{
	for ( int i = 0; i <= 12; i++ )
	{
		if ( BINARY_PRIORITY[ i ][ 0 ] == tt )
			return BINARY_PRIORITY[ i ][ 1 ];
	}
	return 0;
}

void parsePartExpr( vvParser *p, vvSyntaxContainer *rsl );

void parseArgList( vvParser *p, vvSyntaxContainer *rsl )
{
	vvSyntaxContainer *start = NULL, *previous = NULL;

	while ( p->tkBuf.tt != TT_RPAREN )
	{
		if ( p->tkBuf.tt == TT_EOF )
			vv_error( "[%s %zd:%zd] Expected ')', got \"EOF\"",
					  p->lex->fileName, p->tkBuf.row, p->tkBuf.col );

		vvSyntaxContainer *arg = vv_newSyntaxContainer( );
		parseExpr( p, arg );

		if ( start )
		{
			previous->next = arg;
			previous = arg;
		}
		else
		{
			previous = start = arg;
		}

		if ( pcurr( ).tt != TT_RPAREN )
			pexpectg( TT_COMMA, "','" );
	}

	rsl->children[ 1 ] = start;
}

//primary_expr ::= <value> | '(' part_expr ')' | func_expr | call_expr
void parsePrimaryExpr( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	vvToken tk = pcurr( );

	if ( tk.tt == TT_LPAREN )
	{
		pnext( );
		parsePartExpr( p, rsl );
		pexpectg( TT_RPAREN, "')'" );
	}
	else if ( tk.tt == TT_LBRACKET )
	{
		parseFunExpr( p, rsl );
	}
	else if ( tk.tt < TT_BASIC_END || tk.tt == TT_TRUE || tk.tt == TT_FALSE || tk.tt == TT_NIL )
	{
		pnext( );
		rsl->attr.tk = tk;
		rsl->st = ST_PRIMARY;
	}
	else if ( tk.tt == TT_MONEY )
	{
		parseCallExpr( p, rsl );
	}
	else
	{
		vv_error( "[%s %zd:%zd] Unrecognised token", p->lex->fileName, tk.row, tk.col );
	}
}

//tail_part_expr ::= binop part_expr
int parseTailPartExpr( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return 0;

	vvToken cur = pcurr( );
	int priority = getPriority( cur.tt );

	if ( priority == 0 )
	{
		return 0;
	}
	pnext( );

	rsl->children[ 0 ] = vv_newSyntaxContainer( );
	parsePartExpr( p, rsl->children[ 0 ] );
	rsl->attr.tk = cur;

	return priority;
}

//part_expr ::= unop part_expr
//			  | primary_expr { binop part_expr }
void parsePartExpr( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	vvToken cur = pcurr( );

	if ( cur.tt == TT_NOT || cur.tt == TT_MINUS )
	{
		rsl->st = cur.tt == TT_NOT ? ST_NOT_EXPR : ST_INV_EXPR;
		rsl->children[ 0 ] = vv_newSyntaxContainer( );
		parsePartExpr( p, rsl->children[ 0 ] );

		return;
	}

	parsePrimaryExpr( p, rsl );

	vvSyntaxContainer *previous = rsl, *tail = vv_newSyntaxContainer( );

	static vvSyntaxContainer *stk[ 10 ];
	static int stkIdx = 0;

	int priority, lastPriority = 0;

	while ( priority = parseTailPartExpr( p, tail ) )
	{
		assert( tail );
		if ( priority >= lastPriority )
		{
			previous->next = tail;
			previous = tail;
		}
		else
		{
			stk[ stkIdx++ ] = tail;
		}

		lastPriority = priority;
	}

	for ( ; stkIdx >= 0; stkIdx-- )
	{
		previous->next = stk[ stkIdx ];
		previous = stk[ stkIdx ];

		stkIdx--;
	}

	stkIdx = 0;
}

//expr ::= part_expr
void parseExpr( vvParser *p, vvSyntaxContainer *rsl )
{
	parsePartExpr( p, rsl );
}

void parseBlock( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	rsl->st = ST_BLOCK;

	pexpectg( TT_LBRACE, "'{'" );

	vvSyntaxContainer *start = NULL, *previous = NULL;

	while ( p->tkBuf.tt != TT_RBRACE )
	{
		if ( p->tkBuf.tt == TT_EOF )
			vv_error( "[%s %zd:%zd] Expected '}', got \"EOF\"",
					  p->lex->fileName, p->tkBuf.row, p->tkBuf.col );
		vvSyntaxContainer *temp = vv_newSyntaxContainer( );
		parseStatement( p, temp );

		if ( start )
		{
			previous->next = temp;
			previous = temp;
		}
		else
		{
			previous = start = temp;
		}
	}

	rsl->children[ 0 ] = start;
}

void parseAssignExpr( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	rsl->st = ST_ASSIGN_EXPR;

	if ( p->tkBuf.tt != TT_IDENTIFIER )
		vv_error( "[%s %zd:%zd] Expected IDENTIFIER, got \"%s\"",
				  p->lex->fileName, p->tkBuf.row, p->tkBuf.col, vv_tkToString( p->tkBuf ) );
	rsl->attr.tk = p->tkBuf;
	pnext( );

	pexpectg( TT_IS, "\":=\"" );

	rsl->children[ 0 ] = vv_newSyntaxContainer( );
	parseExpr( p, rsl->children[ 0 ] );
}

void parseDefStmt( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	rsl->st = ST_DEF_STMT;

	pexpectg( TT_DEF, "\"def\"" );

	if ( p->tkBuf.tt != TT_IDENTIFIER )
		vv_error( "[%s %zd:%zd] Expected IDENTIFIER, got \"%s\"",
				  p->lex->fileName, p->tkBuf.row, p->tkBuf.col, vv_tkToString( p->tkBuf ) );
	rsl->attr.tk = p->tkBuf;
	pnext( );

	if ( pcurr( ).tt == TT_COLON )
	{
		pnext( );
		pexpectg( TT_PARTIAL, "\"partial\"" );
		rsl->st = ST_DEF_PARTIAL_STMT;
	}

	pexpectg( TT_IS, "\":=\"" );

	rsl->children[ 0 ] = vv_newSyntaxContainer( );
	parseExpr( p, rsl->children[ 0 ] );
}

void parseWhenStmt( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	rsl->st = ST_WHEN_STMT;

	pexpectg( TT_WHEN, "\"when\"" );

	rsl->children[ 0 ] = vv_newSyntaxContainer( );
	parseExpr( p, rsl->children[ 0 ] );

	rsl->children[ 1 ] = vv_newSyntaxContainer( );
	parseBlock( p, rsl->children[ 1 ] );

	if ( pcurr( ).tt == TT_COLON )
	{
		rsl->children[ 2 ] = vv_newSyntaxContainer( );
		parseBlock( p, rsl->children[ 2 ] );
	}
}

void parseWhileStmt( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	rsl->st = ST_WHILE_STMT;

	pexpectg( TT_WHILE, "\"while\"" );

	rsl->children[ 0 ] = vv_newSyntaxContainer( );
	parseExpr( p, rsl->children[ 0 ] );

	rsl->children[ 1 ] = vv_newSyntaxContainer( );
	parseBlock( p, rsl->children[ 1 ] );
}

void parseRetStmt( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	rsl->st = ST_RET_STMT;

	pexpectg( TT_RETURN, "\"return\"" );

	if ( pcurr( ).tt == TT_SEMICOLON )
	{
		pnext( );
		return;
	}

	rsl->children[ 0 ] = vv_newSyntaxContainer( );
	parseExpr( p, rsl->children[ 0 ] );
}

void parseFunExpr( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	rsl->st = ST_FUNC_EXPR;

	pexpectg( TT_LBRACKET, "'['" );
	pexpectg( TT_COLON, "'':'" );

	vvSyntaxContainer *start = NULL, *previous = NULL;

	while ( p->tkBuf.tt != TT_RBRACKET )
	{
		if ( p->tkBuf.tt == TT_EOF )
			vv_error( "[%s %zd:%zd] Expected ']', got \"EOF\"",
					  p->lex->fileName, p->tkBuf.row, p->tkBuf.col );
		vvToken tk = pcurr( );

		if ( tk.tt != TT_IDENTIFIER )
			vv_error( "[%s %zd:%zd] Expected \"IDENTIFIER\", got \"%s\"",
					  p->lex->fileName, p->tkBuf.row, p->tkBuf.col, vv_tkToString( tk ) );

		vvSyntaxContainer *arg = vv_newSyntaxContainer( );
		arg->st = ST_ARG_LIST;
		arg->attr.tk = tk;

		if ( start )
		{
			previous->next = arg;
			previous = arg;
		}
		else
		{
			previous = start = arg;
		}

		pnext( );
		pexpectg( TT_COMMA, "','" );
	}

	pnext( );
	pexpectg( TT_COLON, "':'" );

	rsl->children[ 0 ] = start;

	vvSyntaxContainer *blk = vv_newSyntaxContainer( );
	parseBlock( p, blk );

	rsl->children[ 1 ] = blk;
}

// call_expr ::= "$" "(" expr arg_list ")"
void parseCallExpr( vvParser *p, vvSyntaxContainer *rsl )
{
	if ( pcurr( ).tt == TT_EOF )
		return;

	rsl->st = ST_CALL_EXPR;

	pexpectg( TT_MONEY, "'$'" );
	pexpectg( TT_LPAREN, "'('" );

	vvSyntaxContainer *expr = vv_newSyntaxContainer( );
	parseExpr( p, expr );

	rsl->children[ 0 ] = expr;

	parseArgList( p, rsl );

	pexpectg( TT_RPAREN, "')'" );
}