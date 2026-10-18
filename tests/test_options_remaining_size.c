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
#include <string.h>

static void write_u16(FILE* f, uint16_t val) {
	fwrite(&val, sizeof(val), 1, f);
}

static void write_u32(FILE* f, uint32_t val) {
	fwrite(&val, sizeof(val), 1, f);
}

static void write_zeros(FILE* f, size_t count) {
	static const uint8_t zero[1024] = { 0 };
	while (count > 0) {
		size_t chunk = count > sizeof(zero) ? sizeof(zero) : count;
		fwrite(zero, 1, chunk, f);
		count -= chunk;
	}
}

static int create_poc_pcapng(const char* path) {
	FILE* f = fopen(path, "wb");
	if (!f) return -1;

	// 1. Section Header Block (SHB) - total length 28
	write_u32(f, 0x0A0D0D0A); // Type: SHB
	write_u32(f, 28);         // Block Total Length
	write_u32(f, 0x1A2B3C4D); // Byte-Order Magic
	write_u16(f, 1);          // Major Version
	write_u16(f, 0);          // Minor Version
	write_u32(f, 0xFFFFFFFF); // Section Length (-1)
	write_u32(f, 0xFFFFFFFF);
	write_u32(f, 28);         // Block Total Length trailer

	// 2. Interface Description Block (IDB) - total length 20
	write_u32(f, 0x00000001); // Type: IDB
	write_u32(f, 20);         // Block Total Length
	write_u16(f, 1);          // LinkType (Ethernet)
	write_u16(f, 0);          // Reserved
	write_u32(f, 65535);      // SnapLen
	write_u32(f, 20);         // Block Total Length trailer

	// 3. Enhanced Packet Block (EPB) with chained options
	// Total options size: 200,000 bytes
	// 3 options of 65532 bytes (payload: 65532, header: 4 -> 65536 each)
	// 1 option of 3384 bytes (payload: 3384, header: 4 -> 3388)
	// 1 option header: code=0x1234, len=0xFFFF (header: 4 bytes, 0 payload bytes left in buffer)
	// Total options = 3*65536 + 3388 + 4 = 200,000 bytes.
	// EPB header: 32 bytes (type 4, len 4, iface 4, ts_hi 4, ts_lo 4, caplen 4, origlen 4, trailer 4)
	// Packet data: 60 bytes (already 4-byte aligned)
	// Total EPB length = 32 + 60 + 200000 = 200092 bytes.
	uint32_t epb_total_len = 200092;
	write_u32(f, 0x00000006);    // Type: EPB
	write_u32(f, epb_total_len); // Block Total Length
	write_u32(f, 0);             // Interface ID
	write_u32(f, 0);             // Timestamp High
	write_u32(f, 0);             // Timestamp Low
	write_u32(f, 60);            // Captured Packet Length
	write_u32(f, 60);            // Original Packet Length
	write_zeros(f, 60);          // Packet Data

	// Options area:
	for (int i = 0; i < 3; i++) {
		write_u16(f, 0x0002); // opt_comment
		write_u16(f, 65532);  // option length
		write_zeros(f, 65532);
	}
	write_u16(f, 0x0002);     // opt_comment
	write_u16(f, 3384);       // option length
	write_zeros(f, 3384);

	// Malicious header at the end: code 0x1234, length 0xFFFF (no payload follows)
	write_u16(f, 0x1234);
	write_u16(f, 0xFFFF);

	write_u32(f, epb_total_len); // Block Total Length trailer

	fclose(f);
	return 0;
}

int main(int argc, const char** args) {
	const char* test_file = argc > 1 ? args[1] : "test_options_oob.pcapng";
	if (create_poc_pcapng(test_file) != 0) {
		fprintf(stderr, "Failed to create test file: %s\n", test_file);
		return 1;
	}

	light_pcapng reader = light_pcapng_open(test_file, "rb");
	if (reader == NULL) {
		fprintf(stderr, "light_pcapng_open failed\n");
		remove(test_file);
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
	remove(test_file);

	if (count != 1) {
		fprintf(stderr, "Expected 1 packet parsed, got %d\n", count);
		return 1;
	}

	printf("test_options_remaining_size passed cleanly: packet read without crash.\n");
	return 0;
}
