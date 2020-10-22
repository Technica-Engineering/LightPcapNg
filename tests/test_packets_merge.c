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

#include "light_pcapng_ext.h"

#include <stdio.h>
#include <stdlib.h>

void copy_file(const char* src_path, const char* dst_path) {
	FILE* src_fd = fopen(src_path, "rb");
	FILE* dst_fd = fopen(dst_path, "wb");
	size_t n, m;
	unsigned char buff[8192];
	do {
		n = fread(buff, 1, 8192, src_fd);
		if (n)
			m = fwrite(buff, 1, n, dst_fd);
		else
			m = 0;
	} while ((n > 0) && (n == m));

	fflush(dst_fd);
	fclose(src_fd);
	fclose(dst_fd);
}

int main(int argc, const char** args) {
	if (argc < 3) {
		fprintf(stderr, "Usage %s <outfile> <infile> [<infile> ...]", args[0]);
		return 1;
	}

	const char* outfile = args[1];
	light_pcapng writer = light_pcapng_open(outfile, "wb");

	for (int i = 2; i < argc; ++i) {

		const char* infile = args[i];
		light_pcapng reader = light_pcapng_open(infile, "rb");
		if (reader == NULL)
		{
			light_pcapng_close(writer);
			fprintf(stderr, "Unable to read pcapng: %s\n", infile);
			return 1;
		}

		while (1) {
			light_packet_interface pkt_interface;
			light_packet_header pkt_header;
			const uint8_t* pkt_data = NULL;
			int res = 0;

			res = light_read_packet(reader, &pkt_interface, &pkt_header, &pkt_data);
			if (!res || pkt_data == NULL) {
				break;
			}

			light_write_packet(writer, &pkt_interface, &pkt_header, pkt_data);
		}

		light_pcapng_close(reader);
	}
	light_pcapng_close(writer);
	return 0;
}
