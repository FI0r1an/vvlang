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

#ifndef VV_CODEGEN
#define VV_CODEGEN

#include <stdio.h>
#include "vvlex.h"
#include "vvop.h"

#define VV_STORAGE_DEFAULT 16
#define VV_LOCAL_SECTION_DEFAULT 2
#define VV_NAMELESS ( TT_MINUS )

typedef enum vvVarType
{
	VAR_REGISTER = 0,
	VAR_MEMORY,
	VAR_CONST,

	FLAG_VAR_AMOUNT,

	VAR_LOCAL,
} vvVarType;

typedef struct vvGenVar
{
	vvString identifier;
	vvVarType type;
	size_t idx;
} vvGenVar;

#define VV_SYM_TABLE_STEP 2

typedef struct vvSymSection
{
	size_t idx, len;
	vvGenVar *sto;
} vvSymSection;

typedef struct vvSymTable
{
	vvSymSection *sections;
} vvSymTable;

#define VV_GLOBAL_FIELD NULL

typedef struct vvGenField
{
	vvSymSection *locals;
} vvGenField;

#define VV_FIELD_STACK_MAX 16

typedef struct vvFieldStack
{
	size_t len;
	int top;
	vvGenField **storage;
} vvFieldStack;

#define VV_CODEGEN_BUFFER_DEFAULT_LEN 16

typedef struct vvGenerator
{
	vvOpData *buf;
	size_t cnt, len;

	vvSymTable *tbl;

	vvFieldStack *fields;
	vvGenField *curField;
} vvGenerator;

#endif