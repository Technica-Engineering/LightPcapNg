// Copyright (c) 2020 Technica Engineering GmbH
// Copyright (c) 2019 TMEIC Corporation - Robert Kriener

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

#ifdef LIGHT_USE_ZSTD

#include "light_io.h"
#include "light_io_internal.h"
#include "light_io_zstd.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <zstd.h>      // presumes zstd library is installed


//An ethernet packet should only ever be up to 1500 bytes + some header crap
//We also expect some ovehead for the pcapng blocks which contain the ethernet packets
//so allocate 2048 bytes as the max input size we expect in a single shot
#define COMPRESSION_BUFFER_IN_MAX_SIZE 2048

#define MAX(a,b) (((a)>(b))?(a):(b))

//This is the z-std compression type I would call it z-std type and realias 
//2x but complier won't let me do that across bounds it seems
//So I gave it a generic "light" name....
struct zstd_compression_t
{
	FILE* file;

	uint32_t* buffer_in;
	size_t buffer_in_max_size;

	uint32_t* buffer_out;
	size_t buffer_out_max_size;

	ZSTD_CCtx* cctx;
};

struct zstd_decompression_t
{
	FILE* file;
	uint32_t* buffer_in;
	size_t buffer_in_max_size;

	uint32_t* buffer_out;
	size_t buffer_out_max_size;

	ZSTD_DCtx* dctx;
	int outputReady;

	ZSTD_outBuffer output;
	ZSTD_inBuffer input;
};

void* get_zstd_compression_context(FILE* file, int compression_level)
{
	struct zstd_compression_t* context = calloc(1, sizeof(struct zstd_compression_t));
	context->file = file;

	context->cctx = ZSTD_createCCtx();

	//Enough to handle a whole packet
	context->buffer_in_max_size = COMPRESSION_BUFFER_IN_MAX_SIZE;
	context->buffer_in = malloc(context->buffer_in_max_size);

	//If we don't compress to a smaller or equal size then we are we compressing at all!
	context->buffer_out_max_size = MAX(ZSTD_CStreamOutSize(), COMPRESSION_BUFFER_IN_MAX_SIZE);
	context->buffer_out = malloc(context->buffer_out_max_size);

	assert(!ZSTD_isError(ZSTD_CCtx_setParameter(context->cctx, ZSTD_c_compressionLevel, compression_level)));

	return context;
}

void* get_zstd_decompression_context(FILE* file)
{
	struct zstd_decompression_t* context = calloc(1, sizeof(struct zstd_decompression_t));
	context->file = file;

	context->dctx = ZSTD_createDCtx();
	// Enough to handle a whole packet
	context->buffer_in_max_size = ZSTD_DStreamInSize();
	context->buffer_in = malloc(context->buffer_in_max_size);

	// ZSTD_DStreamOutSize() is big enough to hold atleast 1 full frame, but we can go bigger
	context->buffer_out_max_size = MAX(ZSTD_DStreamOutSize(), COMPRESSION_BUFFER_IN_MAX_SIZE);
	context->buffer_out = malloc(context->buffer_out_max_size);

	context->output.dst = context->buffer_out;
	context->output.size = context->buffer_out_max_size;
	context->output.pos = 0;
	context->outputReady = 0;

	return context;
}

size_t light_zstd_read(void* context, void* buf, size_t count)
{
	struct zstd_decompression_t* decompression = context;
	//Decompression is a little more complex
	//Need to manage reading bytes from orignal file
	//Decompressing those into a buffer
	//Then reading the selected number of bytes from the buffer
	//Once whole buffer is consumed we need to read and decompress next chunk from file

	size_t bytes_read = 0;

	while (bytes_read < count)
	{
		if (decompression->outputReady == 0)
		{
			//Check if we need to grab a new chunk from the actual file
			//If we read all the input then yes, we need to do that
			if (decompression->input.pos >= decompression->input.size)
			{
				//Read a decompress a chunk
				size_t bytes_read_file = fread(decompression->buffer_in, 1, decompression->buffer_in_max_size, decompression->file);
				if (bytes_read_file < decompression->buffer_in_max_size && bytes_read_file == 0 && feof(decompression->file))
					return EOF;
				decompression->input.src = decompression->buffer_in;
				decompression->input.size = bytes_read_file;
				decompression->input.pos = 0;
			}
			//Decompress into the output buffer and use this buffer to actually get our results
			decompression->output.dst = decompression->buffer_out;
			decompression->output.size = decompression->buffer_out_max_size;
			decompression->output.pos = 0;

			size_t const remaining = ZSTD_decompressStream(decompression->dctx, &decompression->output, &decompression->input);
			assert(!ZSTD_isError(remaining));

			//Re-use the output class to track our own consumption
			decompression->output.size = decompression->output.pos;
			decompression->output.pos = 0;
			decompression->outputReady = 1;
		}

		//Now get bytes from our output buffer

		//Case we need everything or less than that which was decoded
		size_t needToRead = count - bytes_read;
		size_t remaining = (decompression->output.size - decompression->output.pos);
		if (needToRead <= remaining)
		{
			memcpy((uint8_t*)buf + bytes_read, (uint8_t*)decompression->output.dst + decompression->output.pos, needToRead);
			decompression->output.pos += needToRead;
			bytes_read += needToRead;
		}

		//Case we need more than that which was decoded
		if (needToRead > remaining)
		{
			memcpy((uint8_t*)buf + bytes_read, (uint8_t*)decompression->output.dst + decompression->output.pos, remaining);
			bytes_read += remaining;
			decompression->output.pos += remaining;
		}

		//We have consumed everything - set next call to decompress a new chunk
		if (decompression->output.pos == decompression->output.size)
			decompression->outputReady = 0;

	}

	return bytes_read;
}

size_t light_zstd_write(void* context, const void* buf, size_t count)
{
	struct zstd_compression_t* compression = context;
	// Do compression here!
	/* Set the input buffer to what we just read.
	* We compress until the input buffer is empty, each time flushing the
	* output.
	*/
	ZSTD_inBuffer input = { buf, count, 0 };
	int finished;
	do
	{
		/* Compress into the output buffer and write all of the output to
		* the file so we can reuse the buffer next iteration.
		*/
		ZSTD_outBuffer output = {
			compression->buffer_out,
			compression->buffer_out_max_size,
			0
		};
		size_t const remaining = ZSTD_compressStream2(compression->cctx, &output, &input, ZSTD_e_continue);
		assert(!ZSTD_isError(remaining));
		fwrite(output.dst, 1, output.pos, compression->file);
		// If we're on the last chunk we're finished when zstd returns 0,
		// We're finished when we've consumed all the input.
		finished = (input.pos == input.size);
	} while (!finished);

	return count;
}

long light_zstd_tell_w(void* context)
{
	struct zstd_compression_t* compression = context;

	return ftell(compression->file);
}

int light_zstd_flush_w(void* context)
{
	struct zstd_compression_t* compression = context;

	return fflush(compression->file);
}

int light_zstd_close_w(void* context)
{
	struct zstd_compression_t* compression = context;
	//Wrap up the compression here

	ZSTD_inBuffer input = { 0,0,0 };

	int remaining = 1;

	while (remaining != 0)
	{
		ZSTD_outBuffer output = { compression->buffer_out, compression->buffer_out_max_size, 0 };
		remaining = ZSTD_compressStream2(compression->cctx, &output, &input, ZSTD_e_end);
		fwrite(output.dst, 1, output.pos, compression->file);
	}

	ZSTD_freeCCtx(compression->cctx);
	free(compression->buffer_out);
	free(compression->buffer_in);

	int res = fclose(compression->file);
	free(compression);

	return res;
}

long light_zstd_tell_r(void* context)
{
	struct zstd_decompression_t* decompression = context;

	return ftell(decompression->file);
}

int light_zstd_close_r(void* context)
{
	struct zstd_decompression_t* decompression = context;

	ZSTD_freeDCtx(decompression->dctx);
	free(decompression->buffer_out);
	free(decompression->buffer_in);

	int res = fclose(decompression->file);
	free(decompression);

	return res;
}

light_file light_io_zstd_open(const char* filename, const char* mode)
{
	// 0 level means default
	int compression_level = 0;

	// parse mode
	bool read = false;
	bool write = false;
	while (mode && *mode)
	{
		if (*mode >= '0' && *mode <= '9') {
			compression_level = *mode - '0';
			//Input is scale 0-9 but zstd goes 1-22!
			compression_level = (compression_level * 2) + 1;
		}
		else {
			switch (*mode)
			{
			case 'b':
				break;
			case 'r':
				read = true;
				break;
			case 'w':
				write = true;
				break;
			default:
				return NULL;
			}
		}
		mode++;
	}
	if (read && write) {
		// we don't do that with a compressed file
		return NULL;
	}
	if (!read && !write) {
		// what to do with the file then?
		return NULL;
	}
	FILE* file = fopen(filename, read ? "rb" : "wb");

	if (!file)
	{
		return NULL;
	}

	light_file fd = calloc(1, sizeof(struct light_file_t));

	if (read) {
		fd->context = get_zstd_decompression_context(file);
		fd->fn_read = &light_zstd_read;
		fd->fn_tell = &light_zstd_tell_r;
		fd->fn_close = &light_zstd_close_r;
	}
	else {
		fd->context = get_zstd_compression_context(file, compression_level);
		fd->fn_write = &light_zstd_write;
		fd->fn_tell = &light_zstd_tell_w;
		fd->fn_flush = &light_zstd_flush_w;
		fd->fn_close = &light_zstd_close_w;
	}
	
	return fd;
}
#endif // LIGHT_USE_ZSTD