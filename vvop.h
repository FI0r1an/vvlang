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

#ifndef VV_OPCODE
#define VV_OPCODE

#include <stdio.h>
#include <stdint.h>

typedef enum vvOpcode
{
	// basic testing commands
	OP_HALT,

	OP_STORE,
	OP_LOAD,
	OP_MOV,

	OP_PUSH,
	OP_POP,
	OP_PEEK,
	OP_DUP,
	OP_CALL,
	OP_LEAV,
	
	OP_JMP,
	OP_JMPT,
	OP_JMPF,

	OP_NOT,
	OP_EQ,
	OP_NEQ,
	OP_AND,
	OP_OR,
	
	OP_INV,

	OP_GE,
	OP_GT,
	OP_LE,
	OP_LT,
	
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
} vvOpcode;

typedef struct vvOpData
{
	vvOpcode op;
	size_t info[ 2 ];
	size_t A, B, C;
} vvOpData;

#endif