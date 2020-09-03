// light_io.c
// Created on: Jul 23, 2016

// Copyright (c) 2016 Radu Velea

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "light_io_mem.h"
#include "light_io_internal.h"
#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h> 
#include <string.h>

typedef struct mem_context
{
	uint8_t* data;
	size_t offset;
	size_t size;
} mem_context;

size_t light_mem_read(void* context, void* buf, size_t count)
{
	mem_context* mem = context;
	size_t remaining = mem->size - mem->offset;
	size_t len = count < remaining ? count : remaining;

	memcpy(buf, mem->data + mem->offset, len);

	mem->offset += len;

	return len;
}

size_t light_mem_write(void* context, const void* buf, size_t count)
{
	mem_context* mem = context;
	size_t remaining = mem->size - mem->offset;
	size_t len = count < remaining ? count : remaining;

	memcpy(mem->data + mem->offset, buf, len);

	mem->offset += len;

	return len;
}

int light_mem_seek(void* context, long int offset, int origin)
{
	mem_context* mem = context;
	long int new_offset = -1;
	switch (origin)
	{
	case SEEK_SET:
		new_offset = offset;
	case	SEEK_CUR:
		new_offset += offset;
	case	SEEK_END:
		new_offset = mem->size + offset;
	}
	if (new_offset < 0 || new_offset > mem->size) {
		return 1;
	}
	mem->offset = new_offset;
	return 0;
}

int light_mem_flush(void* context)
{
	return 0;
}

int light_mem_close(void* context)
{
	free(context);
	return 0;
}

light_file light_io_mem_create(void* memory, size_t size)
{
	if (!memory)
	{
		return NULL;
	}
	mem_context* mem = calloc(1, sizeof(struct mem_context));
	mem->data = memory;
	mem->size = size;

	light_file fd = calloc(1, sizeof(struct light_file_t));
	fd->context = mem;
	fd->fn_read = &light_mem_read;
	fd->fn_write = &light_mem_write;
	fd->fn_flush = &light_mem_flush;
	fd->fn_close = &light_mem_close;
	return fd;
}