// Copyright (c) 2020 Technica Engineering GmbH
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

#include "light_io_file.h"
#include "light_io_internal.h"
#include <stdio.h>
#include <stdlib.h> 

size_t light_file_read(void* context, void* buf, size_t count)
{
	FILE* file = context;
	return fread(buf, 1, count, file);
}

size_t light_file_write(void* context, const void* buf, size_t count)
{
	FILE* file = context;
	return fwrite(buf, 1, count, file);
}

int light_file_seek(void* context, long int offset, int origin)
{
	FILE* file = context;
	return fseek(file, offset, origin);
}

int light_file_flush(void* context)
{
	FILE* file = context;
	return fflush(file);
}

int light_file_close(void* context)
{
	FILE* file = context;
	return fclose(file);
}

light_file light_io_file_open(const char* filename, const char* mode)
{
	FILE* file = fopen(filename, mode);
	return light_io_file_create(file);
}

light_file light_io_file_create(FILE* file)
{
	if (!file)
	{
		return NULL;
	}
	light_file fd = calloc(1, sizeof(struct light_file_t));
	fd->context = file;
	fd->fn_read = &light_file_read;
	fd->fn_write = &light_file_write;
	fd->fn_flush = &light_file_flush;
	fd->fn_seek = &light_file_seek;
	fd->fn_close = &light_file_close;
	return fd;
}