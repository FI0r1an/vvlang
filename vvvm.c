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

#include "vvvm.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vvop.h"
#include "vvcom.h"

vvCallFrame vv_newCallFrame( size_t adr1, size_t func1, size_t adr2, size_t func2 )
{
	vvCallFrame rsl = ( vvCallFrame ){
		.from = { adr1,
				  func1 },
		.to = { adr2,
				func2 },
	};

	rsl.stk = ( vvValue * ) calloc( 1, sizeof( vvValue ) );
	assert( rsl.stk );

	rsl.top = 0, rsl.len = 1;

	return rsl;
}

size_t vv_callFramePush( vvCallFrame *frame, vvValue *val )
{
	if ( frame->top >= frame->len )
	{
		frame->len++;

		vvValue *temp = ( vvValue * ) realloc( frame->stk, sizeof( vvValue ) * frame->len );
		assert( temp );

		frame->stk = temp;
	}

	frame->stk[ frame->top ] = *val;

	return frame->top++;
}

vvValue *vv_callFramePop( vvCallFrame *frame )
{
	if ( frame->top == 0 )
	{
		return NULL;
	}

	return &frame->stk[ frame->top-- ];
}

vvCallStack *vv_newCallStack( )
{
	vvCallStack *rsl = ( vvCallStack * ) malloc( sizeof( vvCallStack ) );
	assert( rsl );

	rsl->len = 1, rsl->top = 0;
	rsl->frames = ( vvCallFrame * ) calloc( 1, sizeof( vvCallFrame ) );
	assert( rsl->frames );

	return rsl;
}

size_t vv_callStackPush( vvCallStack *stk, vvCallFrame frame )
{
	if ( stk->top >= stk->len )
	{
		stk->len++;

		vvCallFrame *temp = ( vvCallFrame * ) realloc( stk->frames, sizeof( vvCallFrame ) * stk->len );
		assert( temp );

		stk->frames = temp;
	}

	stk->frames[ stk->top ] = frame;

	return stk->top++;
}

void vv_callStackPop( vvCallStack *stk )
{
	if ( stk->top == 0 )
	{
		return;
	}

	free( stk->frames[ stk->top-- ].stk );
}

void vv_freeCallStack( vvCallStack *stk )
{
	for ( size_t i = 0; i < stk->len; i++ )
	{
		free( stk->frames[ i ].stk );
	}
	free( stk->frames );
	free( stk );
}

vvValue *initMemory( size_t len )
{
	vvValue *rsl = ( vvValue * ) calloc( VV_REGISTER_COUNT + len, sizeof( vvValue ) );
	assert( rsl );

	return rsl;
}

vvValue *expandMemory( vvValue *mem, size_t len )
{
	vvValue *temp = ( vvValue * ) realloc( mem, sizeof( vvValue ) * ( VV_REGISTER_COUNT + len ) );
	assert( temp );

	return temp;
}

const extern vvValue
	TRUE_VAL = {
		.vt = VAL_BOOL,
		.val.cst_val = 1,
	},
	FALSE_VAL = {
		.vt = VAL_BOOL,
		.val.cst_val = 0,
	},
	NIL_VAL = {
		.vt = VAL_NIL,
		.val.cst_val = 0,
	};

size_t vv_valueEqual( vvValue *A, vvValue *B )
{
	if ( A->vt != B->vt )
		return 0;
	return A->val.cst_val == B->val.cst_val || A->val.num_val == B->val.num_val;
}

vvValue vv_valueToBool( vvValue *A )
{
	size_t rsl = 1;

	if ( A->vt == VAL_NIL || A->vt == VAL_BOOL && A->val.cst_val == 0 )
		rsl = 0;

	return ( vvValue ){
		.vt = VAL_BOOL,
		.val.cst_val = rsl,
	};
}

vvVM *vv_newVM( char *fn, vvLexTable *tbl )
{
	vvVM *rsl = ( vvVM * ) malloc( sizeof( vvVM ) );
	assert( rsl );

	rsl->fn = fn;
	rsl->tbl = tbl;

	rsl->stk = vv_newCallStack( );
	rsl->idx = ( vvCallInfo ){
		.adr = 0,
		.func = 0,
	};

	rsl->len = 0;

	rsl->mem = initMemory( 1 );
	rsl->memLen = VV_REGISTER_COUNT + 1;
	rsl->insts = NULL;

	return rsl;
}

/*
format:
..
..
HALT/LEAV
*/
size_t vv_addFunction( vvVM *vm, vvOpData *dat )
{
	for ( size_t i = 0; i < vm->len; i++ )
	{
		if ( vm->insts[ i ] == NULL )
		{
			vm->insts[ i ] = dat;
			return i;
		}
	}

	size_t idx = vm->len;

	vvOpData **temp = ( vvOpData ** ) realloc( vm->insts, sizeof( vvOpData * ) * ( ++vm->len ) );
	assert( temp );

	vm->insts = temp;
	vm->insts[ idx ] = dat;

	return idx;
}

void vv_removeFunction( vvVM *vm, size_t idx )
{
	assert( idx < vm->len );

	free( vm->insts[ idx ] );
	vm->insts[ idx ] = NULL;
}

int vv_VMStep( vvVM *vm, vvOpData inst )
{
	vvValue *mem = vm->mem;
	vvCallFrame *frame = &vm->stk->frames[ vm->stk->top ];
	size_t A = inst.A, B = inst.B, C = inst.C;

	if ( inst.op >= OP_GE )
		if ( mem[ A ].vt != VAL_NUMBER || mem[ B ].vt != VAL_NUMBER )
			vv_error( "[%s %zd:%zd] Attempt to perform arithmetic on non-numbers",
					  vm->fn,
					  inst.info[ 0 ], inst.info[ 1 ] );

	switch ( inst.op )
	{
		case OP_HALT:
			return VM_HALT;
		case OP_STORE:
			mem[ A + VV_REGISTER_COUNT ] = mem[ B ];
			break;
		case OP_LOAD:
			mem[ A ] = mem[ B + VV_REGISTER_COUNT ];
			break;
		case OP_MOV:
			mem[ A ] = mem[ B ];
			break;
		case OP_PUSH:
			vv_callFramePush( frame, &mem[ A ] );
			break;
		case OP_POP:
			mem[ A ] = *vv_callFramePop( frame );
			break;
		case OP_PEEK:
			mem[ A ] = frame->stk[ frame->top ];
			break;
		case OP_DUP:
			vv_callFramePush( frame, &frame->stk[ frame->top ] );
			break;
		case OP_CALL:
			vv_callStackPush( vm->stk,
							  vv_newCallFrame(
								  frame->from.adr,
								  frame->from.func,
								  0,
								  A ) );
			for ( ; B; B-- )
			{
				vv_callFramePush( &vm->stk->frames[ vm->stk->top ], vv_callFramePop( frame ) );
			}
			break;
		case OP_LEAV:
			for ( ; A; A-- )
			{
				vv_callFramePush( &vm->stk->frames[ vm->stk->top - 1 ], vv_callFramePop( &vm->stk->frames[ vm->stk->top ] ) );
			}
			vv_callStackPop( vm->stk );
			break;
		case OP_JMP:
			vm->idx.adr = A;
			break;
		case OP_JMPT:
			if ( vv_valueToBool( &mem[ B ] ).val.cst_val )
				vm->idx.adr = A;
			break;
		case OP_JMPF:
			if ( !vv_valueToBool( &mem[ B ] ).val.cst_val )
				vm->idx.adr = A;
			break;
		case OP_NOT:
			mem[ A ] = vv_valueToBool( &mem[ B ] );
			mem[ A ].val.cst_val = !mem[ A ].val.cst_val;
			break;
		case OP_EQ:
			mem[ A ].vt = VAL_BOOL;
			mem[ A ].val.cst_val = vv_valueEqual( &mem[ B ], &mem[ C ] );
			break;
		case OP_NEQ:
			mem[ A ].vt = VAL_BOOL;
			mem[ A ].val.cst_val = !vv_valueEqual( &mem[ B ], &mem[ C ] );
			break;
		case OP_GE:
			mem[ A ].vt = VAL_BOOL;
			mem[ A ].val.cst_val = mem[ B ].val.num_val >= mem[ C ].val.num_val;
			break;
		case OP_GT:
			mem[ A ].vt = VAL_BOOL;
			mem[ A ].val.cst_val = mem[ B ].val.num_val > mem[ C ].val.num_val;
			break;
		case OP_LE:
			mem[ A ].vt = VAL_BOOL;
			mem[ A ].val.cst_val = mem[ B ].val.num_val <= mem[ C ].val.num_val;
			break;
		case OP_LT:
			mem[ A ].vt = VAL_BOOL;
			mem[ A ].val.cst_val = mem[ B ].val.num_val < mem[ C ].val.num_val;
			break;
		case OP_AND:
			mem[ A ].vt = VAL_BOOL;
			mem[ A ].val.cst_val = vv_valueToBool( &mem[ B ] ).val.cst_val &&
								   vv_valueToBool( &mem[ C ] ).val.cst_val;
			break;
		case OP_OR:
			mem[ A ].vt = VAL_BOOL;
			mem[ A ].val.cst_val = vv_valueToBool( &mem[ B ] ).val.cst_val ||
								   vv_valueToBool( &mem[ C ] ).val.cst_val;
			break;
		case OP_INV:
			if ( mem[ B ].vt != VAL_NUMBER )
				vv_error( "[%s %zd:%zd] Attempt to perform arithmetic on non-numbers",
						  vm->fn,
						  inst.info[ 0 ], inst.info[ 1 ] );
			mem[ A ].vt = VAL_NUMBER;
			mem[ A ].val.num_val = -mem[ B ].val.num_val;
			break;
		case OP_ADD:
			mem[ A ].vt = VAL_NUMBER;
			mem[ A ].val.num_val = mem[ B ].val.num_val + mem[ C ].val.num_val;
			break;
		case OP_SUB:
			mem[ A ].vt = VAL_NUMBER;
			mem[ A ].val.num_val = mem[ B ].val.num_val - mem[ C ].val.num_val;
			break;
		case OP_MUL:
			mem[ A ].vt = VAL_NUMBER;
			mem[ A ].val.num_val = mem[ B ].val.num_val * mem[ C ].val.num_val;
			break;
		case OP_DIV:
			if ( mem[ C ].val.num_val == 0. )
				vv_error( "[%s %zd:%zd] Attempt to divide with 0",
						  vm->fn,
						  inst.info[ 0 ], inst.info[ 1 ] );
			mem[ A ].vt = VAL_NUMBER;
			mem[ A ].val.num_val = mem[ B ].val.num_val / mem[ C ].val.num_val;
			break;
	}

	return VM_NEXT;
}

void vv_VMExecute( vvVM *vm )
{
	int flag;
	vvOpData curr = vm->insts[ vm->idx.func ][ vm->idx.adr ];

	while ( flag = vv_VMStep( vm, curr ) )
		curr = vm->insts[ vm->idx.func ][ ++vm->idx.adr ];
}

void vv_freeVM( vvVM *vm )
{
	free( vm->mem );
	vv_freeCallStack( vm->stk );
	free( vm->insts );
}