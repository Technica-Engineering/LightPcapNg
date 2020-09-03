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

#include "light_io.h"
#include "light_io_internal.h"
#include "light_io_file.h"
#include "light_io_zstd.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if _WIN32
// https://stackoverflow.com/a/5820836
#define strcasecmp _stricmp
#endif

const char* get_filename_ext(const char* filename) {
	const char* dot = strrchr(filename, '.');
	if (!dot || dot == filename) {
		return "";
	}
	return dot;
}

light_file light_io_open(const char* filename, const char* mode)
{
	if (!filename) {
		return NULL;
	}
	const char* ext = get_filename_ext(filename);

#if defined(USE_ZSTD)
	if (strcasecmp(ext, ".zst") == 0) {
		return light_io_zstd_open(filename, mode);
	}
#endif

	return light_io_file_open(filename, mode);
}

size_t light_io_read(light_file fd, void* buf, size_t count)
{
	if (fd->fn_read == NULL) {
		return 0;
	}
	return fd->fn_read(fd->context, buf, count);
}

size_t light_io_write(light_file fd, const void* buf, size_t count)
{
	if (fd->fn_write == NULL) {
		return 0;
	}
	return fd->fn_write(fd->context, buf, count);
}

int light_io_seek(light_file fd, long int offset, int origin)
{
	if (fd->fn_seek == NULL) {
		return -1;
	}
	return fd->fn_seek(fd->context, offset, origin);
}

int light_io_flush(light_file fd)
{
	if (fd->fn_flush == NULL) {
		return 0;
	}
	return fd->fn_flush(fd->context);
}

int light_io_close(light_file fd)
{
	int res = fd->fn_close(fd->context);
	free(fd);
	return res;
}
