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

// Verifies the ",N" mode-string suffix that requests N zstd worker threads
// (ZSTD_c_nbWorkers). Writes "wb", "wb5", and "wb5,2", then reads each back.
// zstd frames are wire-identical regardless of nbWorkers, so this test
// verifies the parser accepts the new syntax and the file roundtrips — not
// that workers actually engaged.

#include "light_pcapng_ext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_PACKETS 16
#define CAP_LEN 256

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
	memset(pkt_data, '-', CAP_LEN);

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

static int count_packets(const char* filename)
{
	light_pcapng reader = light_pcapng_open(filename, "rb");
	if (reader == NULL) {
		fprintf(stderr, "open(%s, rb) failed\n", filename);
		return -1;
	}

	int count = 0;
	light_packet_interface iface = { 0 };
	light_packet_header hdr = { 0 };
	const uint8_t* data = NULL;
	// Matches the loop pattern in tests/test_packets_dump.c.
	while (light_read_packet(reader, &iface, &hdr, &data) == 0 && data != NULL) {
		count++;
	}
	light_pcapng_close(reader);
	return count;
}

int main(int argc, const char** args)
{
	if (argc < 4) {
		fprintf(stderr, "Usage: %s <out_default.zst> <out_lvl.zst> <out_mt.zst>\n", args[0]);
		return 1;
	}

	const char* out_default = args[1];
	const char* out_lvl = args[2];
	const char* out_mt = args[3];

	if (write_packets(out_default, "wb") != 0) return 1;
	if (write_packets(out_lvl, "wb5") != 0) return 1;
	if (write_packets(out_mt, "wb5,2") != 0) return 1;

	int n_default = count_packets(out_default);
	int n_lvl = count_packets(out_lvl);
	int n_mt = count_packets(out_mt);

	if (n_default != NUM_PACKETS || n_lvl != NUM_PACKETS || n_mt != NUM_PACKETS) {
		fprintf(stderr, "packet count mismatch: default=%d, lvl=%d, mt=%d (expected %d)\n",
		        n_default, n_lvl, n_mt, NUM_PACKETS);
		return 1;
	}

	return 0;
}
