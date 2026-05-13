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
// Compresses the same highly compressible payload twice — once at level 1
// and once at level 9 — and asserts the level-9 output is strictly smaller
// than the level-1 output. If the level mapping is broken (or silently
// dropped under NDEBUG), both files come out the same size and the assert
// fails. This test must be built with NDEBUG defined to actually exercise
// the bug; tests/CMakeLists.txt does that explicitly.

#include "light_pcapng_ext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define NUM_PACKETS 1000
#define CAP_LEN 1500

static int write_packets(const char* filename, const char* mode)
{
	light_pcapng writer = light_pcapng_open(filename, mode);
	if (writer == NULL) {
		fprintf(stderr, "open(%s, %s) failed\n", filename, mode);
		return -1;
	}

	light_packet_interface iface = { 0 };
	iface.link_type = 1;  // ETHERNET
	iface.name = "interface1";
	iface.timestamp_resolution = 1000000000;
	light_write_interface_block(writer, &iface);

	uint8_t* pkt_data = (uint8_t*)malloc(CAP_LEN);
	if (pkt_data == NULL) {
		light_pcapng_close(writer);
		return -1;
	}
	// Compressible-but-non-trivial payload. Trivially compressible input
	// (e.g. all zeros) is a poor signal because zstd's higher levels add
	// frame overhead that can outweigh the marginal gains on tiny uniform
	// data. A short repeating pattern with some byte variety gives the
	// higher level a meaningful win.
	for (size_t i = 0; i < CAP_LEN; i++) {
		pkt_data[i] = (uint8_t)((i * 37) % 41);
	}

	for (int i = 0; i < NUM_PACKETS; i++) {
		light_packet_header hdr = { 0 };
		struct timespec ts = { i + 1, 0 };
		hdr.timestamp = ts;
		hdr.captured_length = CAP_LEN;
		hdr.original_length = CAP_LEN;
		light_write_packet(writer, &iface, &hdr, pkt_data);
	}

	free(pkt_data);
	light_pcapng_close(writer);
	return 0;
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
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <out_lvl1.zst> <out_lvl9.zst>\n", args[0]);
		return 1;
	}

	const char* out_lvl1 = args[1];
	const char* out_lvl9 = args[2];

	if (write_packets(out_lvl1, "wb1") != 0) return 1;
	if (write_packets(out_lvl9, "wb9") != 0) return 1;

	long size_lvl1 = file_size(out_lvl1);
	long size_lvl9 = file_size(out_lvl9);
	if (size_lvl1 < 0 || size_lvl9 < 0) return 1;

	fprintf(stderr, "lvl1=%ld bytes, lvl9=%ld bytes\n", size_lvl1, size_lvl9);

	// Higher level must yield smaller output on this payload. If equal, the
	// level parameter never reached zstd (the bug this test guards against).
	if (size_lvl9 >= size_lvl1) {
		fprintf(stderr,
		        "FAIL: expected lvl9 < lvl1, got lvl9=%ld >= lvl1=%ld. "
		        "ZSTD_c_compressionLevel may have been dropped (assert+NDEBUG bug).\n",
		        size_lvl9, size_lvl1);
		return 1;
	}

	return 0;
}
