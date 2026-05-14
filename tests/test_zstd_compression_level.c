// Copyright (c) 2020 Technica Engineering GmbH

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

// Regression test for the bug where ZSTD_CCtx_setParameter for the
// compression level was wrapped directly inside assert(), so under
// -DNDEBUG the call was dropped and the writer silently fell back to
// zstd's default level (3) regardless of what the user asked for.
//
// Reads packets from a real pcapng fixture and writes them out twice —
// once at level 1 and once at level 9 — then asserts the level-9 output
// is strictly smaller than the level-1 output. If the level mapping is
// broken (or silently dropped under NDEBUG), both files come out the
// same size and the assert fails. Real packet data gives a much more
// reliable level-1 vs level-9 spread than synthetic patterns.

#include "light_pcapng_ext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int copy_packets(const char* infile, const char* outfile, const char* outmode)
{
	light_pcapng reader = light_pcapng_open(infile, "rb");
	if (reader == NULL) {
		fprintf(stderr, "open(%s, rb) failed\n", infile);
		return -1;
	}
	light_pcapng writer = light_pcapng_open(outfile, outmode);
	if (writer == NULL) {
		fprintf(stderr, "open(%s, %s) failed\n", outfile, outmode);
		light_pcapng_close(reader);
		return -1;
	}

	int written = 0;
	light_packet_interface iface = { 0 };
	light_packet_header hdr = { 0 };
	const uint8_t* data = NULL;
	while (light_read_packet(reader, &iface, &hdr, &data) == 0 && data != NULL) {
		light_write_packet(writer, &iface, &hdr, data);
		written++;
	}

	light_pcapng_close(reader);
	light_pcapng_close(writer);
	return written;
}

static long file_size(const char* filename)
{
	struct stat st;
	if (stat(filename, &st) != 0) {
		fprintf(stderr, "stat(%s) failed\n", filename);
		return -1;
	}
	return (long)st.st_size;
}

int main(int argc, const char** args)
{
	if (argc < 4) {
		fprintf(stderr, "Usage: %s <input.pcapng> <out_lvl1.zst> <out_lvl9.zst>\n", args[0]);
		return 1;
	}

	const char* infile = args[1];
	const char* out_lvl1 = args[2];
	const char* out_lvl9 = args[3];

	int n_lvl1 = copy_packets(infile, out_lvl1, "wb1");
	int n_lvl9 = copy_packets(infile, out_lvl9, "wb9");
	if (n_lvl1 < 0 || n_lvl9 < 0) return 1;
	if (n_lvl1 != n_lvl9) {
		fprintf(stderr, "FAIL: packet count mismatch: lvl1=%d, lvl9=%d\n", n_lvl1, n_lvl9);
		return 1;
	}

	long size_lvl1 = file_size(out_lvl1);
	long size_lvl9 = file_size(out_lvl9);
	if (size_lvl1 < 0 || size_lvl9 < 0) return 1;

	fprintf(stderr, "packets=%d, lvl1=%ld bytes, lvl9=%ld bytes\n", n_lvl1, size_lvl1, size_lvl9);

	// Higher level must yield smaller output on real packet data. If equal,
	// the level parameter never reached zstd (the bug this test guards against).
	if (size_lvl9 >= size_lvl1) {
		fprintf(stderr,
		        "FAIL: expected lvl9 < lvl1, got lvl9=%ld >= lvl1=%ld. "
		        "ZSTD_c_compressionLevel may have been dropped (assert+NDEBUG bug).\n",
		        size_lvl9, size_lvl1);
		return 1;
	}

	return 0;
}
