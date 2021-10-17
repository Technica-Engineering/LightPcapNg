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

#include "light_pcapng.h"
#include "light_io_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, const char** args) {
	if (argc < 3) {
		fprintf(stderr, "Usage %s <outfile> <infile> [<infile> ...]", args[0]);
		return 1;
	}

	const char* outfile = args[1];
	light_file writer = light_io_open(outfile, "wb");

	for (int i = 2; i < argc; i++)
	{
		const char* infile = args[i];
		light_file reader = light_io_open(infile, "rb");
		if (reader == NULL)
		{
			light_io_close(writer);
			fprintf(stderr, "Unable to read pcapng: %s\n", infile);
			return 1;
		}
		light_block block = NULL;
		bool swap_endianness = false;
		light_read_block(reader, &block, &swap_endianness);
		while (block != NULL)
		{
			light_write_block(writer, block);
			light_read_block(reader, &block, &swap_endianness);
		}
		light_io_close(reader);
	}

	light_io_close(writer);
	return 0;
}
