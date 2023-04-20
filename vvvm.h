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

#ifndef VV_VM
#define VV_VM

#include <stdio.h>
#include "vvop.h"
#include "vvlex.h"

typedef struct vvCallInfo
{
	size_t adr, func;
} vvCallInfo;

typedef enum vvValueType
{
	VAL_NIL,
	VAL_BOOL,
	VAL_NUMBER,
	VAL_STRING,
	VAL_FUNCTION,
} vvValueType;

typedef struct vvValue
{
	vvValueType vt;

	union
	{
		size_t cst_val; // NIL BOOL STRING FUNCTION
		float num_val;	// NUMBER
	} val;
} vvValue;

typedef struct vvCallFrame
{
	vvCallInfo from, to;

	vvValue *stk;
	size_t top, len;
} vvCallFrame;

typedef struct vvCallStack
{
	vvCallFrame *frames;
	size_t top, len;
} vvCallStack;

#define VV_REGISTER_COUNT 8

typedef struct vvVM
{
	vvOpData **insts;
	size_t len;

	vvCallInfo idx;
	vvCallStack *stk;

	vvValue *mem;
	size_t memLen;

	char *fn;
	vvLexTable *tbl;
} vvVM;

#define VM_NEXT 1
#define VM_HALT 0

const extern vvValue TRUE_VAL, FALSE_VAL, NIL_VAL;

size_t vv_addFunction( vvVM *vm, vvOpData *dat );
void vv_removeFunction( vvVM *vm, size_t idx );

vvVM *vv_newVM( );
void vv_VMExecute( vvVM *vm );
void vv_freeVM( vvVM *vm );

#endif