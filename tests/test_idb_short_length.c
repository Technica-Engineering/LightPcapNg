// Copyright (c) 2026 Technica Engineering GmbH
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Regression test for the Interface Description Block length check in
// parse_by_type. An IDB whose block_total_length is 12 has an empty body,
// but the parser used to read link_type, reserved and snapshot_length
// (8 bytes) from it, reading past the end of a zero-size heap buffer.
// The block must now be returned with a zeroed body and no options.

#include "light_pcapng.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

int main(int argc, const char** args) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <infile>\n", args[0]);
		return 1;
	}

	const char* infile = args[1];
	light_file file = light_io_open(infile, "rb");
	if (file == NULL) {
		fprintf(stderr, "Unable to read pcapng: %s\n", infile);
		return 1;
	}

	light_block block = NULL;
	bool swap_endianness = false;
	int result = 1;

	light_read_block(file, &block, &swap_endianness);
	if (block == NULL || block->type != LIGHT_SECTION_HEADER_BLOCK) {
		fprintf(stderr, "Expected a section header block\n");
		goto done;
	}

	light_read_block(file, &block, &swap_endianness);
	if (block == NULL || block->type != LIGHT_INTERFACE_BLOCK || block->total_length != 12) {
		fprintf(stderr, "Expected an interface description block of length 12\n");
		goto done;
	}

	struct _light_interface_description_block* idb = (struct _light_interface_description_block*)block->body;
	if (idb == NULL || idb->link_type != 0 || idb->reserved != 0 || idb->snapshot_length != 0 || block->options != NULL) {
		fprintf(stderr, "Expected a zeroed interface description block without options\n");
		goto done;
	}

	light_read_block(file, &block, &swap_endianness);
	if (block != NULL) {
		fprintf(stderr, "Expected end of file\n");
		goto done;
	}

	result = 0;

done:
	light_free_block(block);
	light_io_close(file);
	return result;
}
