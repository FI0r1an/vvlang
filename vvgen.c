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

#include "vvgen.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vvvm.h"
#include "vvop.h"
#include "vvparser.h"
#include "vvlex.h"

#define destroyVar( v ) ( v->identifier = 0 )
#define isNullVar( v ) ( v.identifier == 0 )

size_t vv_symTableAdd( vvSymTable *tbl, vvVarType type, vvGenVar v );

vvSymTable *vv_newSymTable( )
{
	vvSymTable *rsl = ( vvSymTable * ) malloc( sizeof( vvSymTable ) );
	assert( rsl );

	vvSymSection *sections = ( vvSymSection * ) malloc(
		sizeof( vvSymSection ) * FLAG_VAR_AMOUNT );
	assert( sections );

	rsl->sections = sections;

	for ( int i = 0; i < FLAG_VAR_AMOUNT; i++ )
	{
		vvGenVar *sto = ( vvGenVar * ) calloc( VV_STORAGE_DEFAULT, sizeof( vvGenVar ) );
		assert( sto );

		rsl->sections[ i ] = ( vvSymSection ){ 0, VV_STORAGE_DEFAULT, sto };
	}

	// adding const: true false nil
	for ( size_t i = 0; i < 3; i++ )
	{
		vv_symTableAdd( rsl, VAR_CONST, ( vvGenVar ){
											.identifier = TT_TRUE + i,
											.idx = i,
										} );
	}

	for ( size_t i = 0; i < VV_REGISTER_COUNT; i++ )
	{
		vv_symTableAdd( rsl, VAR_REGISTER, ( vvGenVar ){
											   .identifier = VV_NAMELESS,
											   .idx = i,
										   } );
	}

	return rsl;
}

void vv_symTableExpand( vvSymTable *tbl, vvVarType type )
{
	vvSymSection *section = &tbl->sections[ type ];

	section->len += VV_SYM_TABLE_STEP;

	vvGenVar *sto = ( vvGenVar * ) realloc( section->sto, sizeof( vvGenVar ) * section->len );
	assert( sto );
	// fill with NULL_VAR (0)
	memset( sto + ( section->len - VV_SYM_TABLE_STEP - 1 ), 0, sizeof( vvGenVar ) * VV_SYM_TABLE_STEP );

	section->sto = sto;
}

// return NULL if not found
vvGenVar *vv_symTableGet( vvSymTable *tbl, vvString id )
{
	for ( size_t i = 0; i < FLAG_VAR_AMOUNT; i++ )
	{
		for ( size_t j = 0; j < tbl->sections[ i ].len; j++ )
		{
			vvGenVar *rsl = &tbl->sections[ i ].sto[ j ];
			if ( rsl->identifier == id )
			{
				return rsl;
			}
		}
	}

	return NULL;
}

size_t vv_symTableAdd( vvSymTable *tbl, vvVarType type, vvGenVar v )
{
	v.type = type;

	vvSymSection *section = &tbl->sections[ type ];

	if ( section->idx + 1 > section->len )
	{
		vv_symTableExpand( tbl, type );
	}

	vvGenVar cur = section->sto[ section->idx ];

	if ( cur.identifier ) // has value
	{
		do
		{
			section->idx++;
			if ( section->idx >= section->len )
				vv_symTableExpand( tbl, type );

		} while ( isNullVar( section->sto[ section->idx ] ) );
	}

	section->sto[ section->idx++ ] = v;
	return v.idx;
}

vvGenVar *vv_symTableGetConst( vvSymTable *tbl, size_t idx )
{
	vvSymSection section = tbl->sections[ VAR_CONST ];
	size_t len = section.idx;

	if ( idx < len )
	{
		vvGenVar *var = &section.sto[ idx ];

		if ( var->identifier == 0 )
		{
			var->identifier = VV_NAMELESS;
			//var->pos = idx;
			var->idx = idx;
		}

		return var;
	}

	return NULL;
}

void vv_freeSymTable( vvSymTable *tbl )
{
	for ( int i = 0; i < FLAG_VAR_AMOUNT; i++ )
	{
		free( tbl->sections[ i ].sto );
	}

	free( tbl->sections );

	free( tbl );
}

vvGenField *vv_newGenField( )
{
	vvGenField *rsl = ( vvGenField * ) malloc( sizeof( vvGenField ) );
	assert( rsl );

	rsl->locals = ( vvSymSection * ) malloc( sizeof( vvSymSection ) );

	vvSymSection *locals = rsl->locals;
	assert( locals );

	locals->idx = 0;
	locals->len = VV_LOCAL_SECTION_DEFAULT;
	locals->sto = ( vvGenVar * ) malloc( sizeof( vvGenVar ) * VV_LOCAL_SECTION_DEFAULT );
	assert( locals->sto );

	return rsl;
}

void vv_freeGenField( vvGenField *field );
vvGenField *vv_fieldStackPop( vvFieldStack *stack );
void vv_genFieldLeave( vvGenField *field, vvGenerator *gen )
{
	if ( gen->fields->top == -1 )
		gen->curField = VV_GLOBAL_FIELD;
	else
		gen->curField = vv_fieldStackPop( gen->fields );

	vv_freeGenField( field );
}

vvGenVar *vv_genFieldGet( vvGenField *field, vvString id )
{
	vvSymSection *section = field->locals;

	for ( size_t i = 0; i < section->idx; i++ )
	{
		vvGenVar *v = &section->sto[ i ];

		if ( v->identifier == id )
			return v;
	}

	return NULL;
}

vvGenVar *vv_genFieldAlloc( vvGenField *field )
{
	vvSymSection *section = field->locals;

	if ( section->idx >= section->len )
	{
		for ( size_t i = 0; i < section->len; i++ )
		{
			if ( isNullVar( section->sto[ i ] ) )
			{
				vvGenVar *rsl = &section->sto[ i ];
				rsl->identifier = VV_NAMELESS;
				rsl->type = VAR_LOCAL;
				rsl->idx = 0;

				return rsl;
			}
		}

		section->len += VV_SYM_TABLE_STEP;
		vvGenVar *temp = ( vvGenVar * ) realloc( section->sto, section->len );
		assert( temp );
		section->sto = temp;

		vvGenVar *rsl = &section->sto[ section->idx++ ];
		rsl->identifier = VV_NAMELESS;
		rsl->type = VAR_LOCAL;
		rsl->idx = 0;

		return rsl;
	}

	if ( !isNullVar( section->sto[ section->idx ] ) )
	{
		do
		{
			section->idx++;

			if ( section->idx >= section->len )
			{
				section->len += VV_SYM_TABLE_STEP;
				vvGenVar *temp = ( vvGenVar * ) realloc( section->sto, section->len );
				assert( temp );
				section->sto = temp;

				break;
			}
		} while ( isNullVar( section->sto[ section->idx ] ) );
	}

	vvGenVar *rsl = &section->sto[ section->idx++ ];
	rsl->identifier = VV_NAMELESS;
	rsl->type = VAR_LOCAL;
	rsl->idx = 0;

	return rsl;
}

void vv_freeGenField( vvGenField *field )
{
	free( field->locals->sto );
	free( field->locals );

	free( field );
}

void vv_fieldStackResize( vvFieldStack *stack, size_t len )
{
	vvGenField **temp = ( vvGenField ** ) realloc( stack->storage, len * sizeof( vvGenField * ) );
	assert( temp );

	stack->storage = temp;
	stack->len = len;

	if ( stack->top >= ( int ) len )
		stack->top = len - 1;
}

vvFieldStack *vv_newFieldStack( size_t len )
{
	vvFieldStack *rsl = ( vvFieldStack * ) malloc( sizeof( vvFieldStack ) );
	assert( rsl );

	rsl->top = -1;
	rsl->storage = NULL;

	vv_fieldStackResize( rsl, len );

	return rsl;
}

void vv_freeFieldStack( vvFieldStack *stack )
{
	if ( stack->storage )
		free( stack->storage );

	free( stack );
}

vvGenField *vv_fieldStackPeek( vvFieldStack *stack )
{
	if ( stack->top == -1 )
		return NULL;

	return stack->storage[ stack->top ];
}

vvGenField *vv_fieldStackPop( vvFieldStack *stack )
{
	if ( stack->top == -1 )
		return NULL;

	vvGenField *rsl = vv_fieldStackPeek( stack );

	stack->top--;

	return rsl;
}

void vv_fieldStackPush( vvFieldStack *stack, vvGenField *val )
{
	if ( stack->top + 1 >= ( int ) stack->len )
	{
		vv_error( "[Compiler error] Stack overflowed" );
	}

	stack->storage[ ++stack->top ] = val;
}

vvGenField *vv_fieldStackFetch( vvGenerator *gen, vvFieldStack *stack )
{
	return gen->curField = vv_fieldStackPop( stack );
}

#define ENTER_FIELD( f ) \
	gen->curField = f;   \
	vv_fieldStackPush( gen->fields, f );

#define LEAVE_FIELD( f )             \
	vv_fieldStackPop( gen->fields ); \
	vv_genFieldLeave( f, gen );

vvGenerator *vv_newGenerator( size_t bufLen )
{
	vvGenerator *rsl = ( vvGenerator * ) malloc( sizeof( vvGenerator ) );
	assert( rsl );

	rsl->cnt = 0;
	rsl->len = bufLen;

	rsl->buf = ( vvOpData * ) calloc( bufLen, sizeof( vvOpData ) );
	assert( rsl->buf );

	rsl->tbl = vv_newSymTable( );

	rsl->fields = vv_newFieldStack( VV_STORAGE_DEFAULT );
	rsl->curField = VV_GLOBAL_FIELD;

	return rsl;
}

// vv_genFindVar, vv_genTryAlloc, vv_genGetLVal, vv_genGetRVal, vv_genLoad, vv_genStore
// vv_genSave, vv_genFlush, vv_generateExpr

//statement ::= assign_expr | def_stmt | call_expr
//			| when_stmt | while_stmt | ret_stmt | block
void vv_generateStatement( vvGenerator *gen, vvSyntaxContainer *stmt )
{
	vvSyntaxType st = stmt->st;

	switch ( st )
	{
		case ST_ASSIGN_EXPR:
			break;
		case ST_DEF_STMT:
			break;
		case ST_DEF_PARTIAL_STMT:
			break;
		case ST_CALL_EXPR:
			break;

		case ST_WHEN_STMT:
			break;
		case ST_WHILE_STMT:
			break;
		case ST_RET_STMT:
			break;
		case ST_BLOCK:
			break;
		default:
			vv_error( "[Compiler error] Not a statement" );
			return;
	}
}

void vv_generateBlock( vvGenerator *gen )
{
}

void vv_freeGenerator( vvGenerator *gen )
{
	free( gen->buf );
	vv_freeSymTable( gen->tbl );

	vv_freeFieldStack( gen->fields );

	free( gen );
}