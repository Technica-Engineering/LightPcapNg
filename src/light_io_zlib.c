// Copyright (c) 2021 Technica Engineering GmbH
// This code is licensed under MIT license (see LICENSE for details)

#ifdef LIGHT_USE_ZLIB

#include "light_io.h"
#include "light_io_internal.h"
#include "light_io_zlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>      // presumes zlib library is installed

size_t light_zlib_read(void* context, void* buf, size_t count)
{
	return gzread((gzFile)context, buf, count);
}

size_t light_zlib_write(void* context, const void* buf, size_t count)
{
	return gzwrite((gzFile)context, buf, count);
}

int light_zlib_flush(void* context)
{
	return gzflush((gzFile)context, Z_NO_FLUSH);
}

int light_zlib_seek(void* context, long int offset, int origin)
{
	return gzseek((gzFile)context, offset, origin);
}

int light_zlib_close(void* context)
{
	return gzclose((gzFile)context);
}

light_file light_io_zlib_open(const char* filename, const char* mode)
{
	gzFile file = gzopen(filename, mode);
	if (!file)
	{
		return NULL;
	}
	light_file fd = malloc(sizeof(struct light_file_t));

	fd->context = file;
	fd->fn_read = &light_zlib_read;
	fd->fn_write = &light_zlib_write;
	fd->fn_flush = &light_zlib_flush;
	fd->fn_seek = &light_zlib_seek;
	fd->fn_close = &light_zlib_close;

	return fd;
}
#endif // LIGHT_USE_zlib