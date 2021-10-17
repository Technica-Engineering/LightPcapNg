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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

void dump_hex(FILE* out, uint8_t* data, size_t size)
{
	for (int i = 0; i < size; i++)
	{
		fprintf(out, "%02X", *(data + i));
	}
}

int is_ascii(uint8_t* data, size_t size) {
	for (int i = 0; i < size; i++) {
		if (data[i] == 0) {
			return 1;
		}
		if (!isprint(data[i])) {
			return 0;
		}
	}
	return 1;
}

void dump_block(FILE* out, light_block block)
{
	light_block iter = block;
	size_t buffer_size = 256;

	fprintf(out, "Type: 0x%08X\n", iter->type);
	fprintf(out, "Length: %u\n", iter->total_length);
	fprintf(out, "Data: ");
	dump_hex(out, iter->body, 8);
	fprintf(out, "...\n");

	fprintf(out, "Options:\n");
	light_option opt = iter->options;
	while (opt)
	{
		fprintf(out, "\t%u: ", opt->code);
		if (is_ascii((uint8_t*)opt->data, opt->length)) {
			fprintf(out, "%.*s", opt->length, (char*)opt->data);
		}
		else {
			dump_hex(out, opt->data, opt->length);
		}
		fprintf(out, "\n");
		opt = opt->next_option;
	}
	fprintf(out, "\n");
}

int main(int argc, const char** args)
{
	if (argc != 3) {
		fprintf(stderr, "Usage %s [infile] [outfile]", args[0]);
		return 1;
	}
	FILE* outfile = strcmp(args[2], "-") ? fopen(args[2], "w") : stdout;

	light_file infile = light_io_open(args[1], "rb");
	if (infile == NULL)
	{
		fprintf(stderr, "Unable to read: %s\n", args[1]);
		return 1;
	}

	light_block block = NULL;
	bool swap_endianness = false;
	light_read_block(infile, &block, &swap_endianness);
	while (block != NULL) {
		dump_block(outfile, block);
		light_read_block(infile, &block, &swap_endianness);
	}
	light_io_close(infile);

	if (outfile != stdout) {
		fclose(outfile);
	}

	return 0;
}
