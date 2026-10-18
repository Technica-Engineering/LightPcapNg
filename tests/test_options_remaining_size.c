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

// Regression test for __parse_options remaining_size accounting.
// When parsing block options, remaining_size was incorrectly incremented
// (remaining_size += actual_length) instead of decremented, which caused
// an inflated budget for subsequent options and heap out-of-bounds reads
// when encountering an option with excessive declared length at the end
// of the options block.

#include "light_pcapng_ext.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, const char** args) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <infile>\n", args[0]);
		return 1;
	}

	const char* infile = args[1];
	light_pcapng reader = light_pcapng_open(infile, "rb");
	if (reader == NULL) {
		fprintf(stderr, "Unable to read pcapng: %s\n", infile);
		return 1;
	}

	light_packet_interface iface = { 0 };
	light_packet_header hdr = { 0 };
	const uint8_t* data = NULL;
	int count = 0;

	// With the fix, parsing must safely discard the malformed option without crashing or OOB read.
	while (light_read_packet(reader, &iface, &hdr, &data) == 0 && data != NULL) {
		count++;
	}

	light_pcapng_close(reader);

	if (count != 1) {
		fprintf(stderr, "Expected 1 packet parsed, got %d\n", count);
		return 1;
	}

	return 0;
}
