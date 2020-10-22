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
#include <string.h>

int main(int argc, const char** args) {
	if (argc != 3) {
		fprintf(stderr, "Usage %s [infile] [outfile]", args[0]);
		return 1;
	}
	FILE* outfile = strcmp(args[2], "-") ? fopen(args[2], "w") : stdout;
	light_pcapng infile = light_pcapng_open(args[1], "rb");
	if (infile == NULL)
	{
		fprintf(stderr, "Unable to read: %s\n", args[1]);
		return 1;
	}

	light_pcapng_file_info* info = light_pcang_get_file_info(infile);
	fprintf(outfile, "version: %d.%d\n", info->major_version, info->minor_version);
	fprintf(outfile, "comment: %s\n", info->comment);
	fprintf(outfile, "os: %s\n", info->os_desc);
	fprintf(outfile, "hardware: %s\n", info->hardware_desc);
	fprintf(outfile, "app: %s\n", info->app_desc);
	fprintf(outfile, "\n");

	int index = 1;

	while (1) {
		light_packet_interface pkt_interface;
		light_packet_header pkt_header;
		const uint8_t* pkt_data = NULL;
		int res = 0;

		res = light_read_packet(infile, &pkt_interface, &pkt_header, &pkt_data);
		if (!res || pkt_data == NULL) {
			break;
		}

		fprintf(outfile,
			"Packet #%d: cap_len=%d, iface=%s, link=%d, timestamp=%d.%09d",
			index,
			pkt_header.captured_length,
			pkt_interface.name,
			pkt_interface.link_type,
			(int)pkt_header.timestamp.tv_sec,
			(int)pkt_header.timestamp.tv_nsec);

		if (pkt_header.comment) {
			fprintf(outfile, ", comment=\"%s\"", pkt_header.comment);
		}

		fprintf(outfile, "\n");

		index++;

	}

	light_pcapng_close(infile);

	return 0;
}
