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

#include "light_io_mem.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "_util.h"

int read_file(const char* file, uint8_t** data, size_t* size)
{
	size_t tmp;
	struct stat info;
	FILE* f;

	f = fopen(file, "rb");
	stat(file, &info);
	*data = calloc(info.st_size, 1);
	tmp = fread(*data, 1, info.st_size, f);
	if (tmp != info.st_size) {
		free(*data);
		fclose(f);
		return -1;
	}

	fclose(f);
	*size = info.st_size;
	return 0;
}

int main(int argc, const char** args) {

	if (argc != 2) {
		fprintf(stderr, "Usage %s [infile]", args[0]);
		return 1;
	}

	const char* infile = args[1];
	uint8_t* data;
	size_t size;
	if (read_file(infile, &data, &size) != 0) {
		fprintf(stderr, "Unable to read pcapng: %s\n", infile);
		return 1;
	}

	light_file mem = light_io_mem_create(data, size);
	light_file file = light_io_open(infile, "rb");

	int res = light_file_diff(mem, file, stderr);

	light_io_close(mem);
	free(data);
	light_io_close(file);

	return res;
}

