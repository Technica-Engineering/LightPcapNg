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
#include <time.h>

int main(int argc, const char** args) {
	if (argc < 2) {
		fprintf(stderr, "Usage %s <outfile>", args[0]);
		return 1;
	}

	const char* outfile = args[1];
	light_pcapng writer = light_pcapng_open(outfile, "wb");

	light_packet_interface pkt_interface1 = { 0 };
	light_packet_interface pkt_interface2 = { 0 };
	light_packet_interface pkt_interface3 = { 0 };
	light_packet_interface pkt_interface4 = { 0 };
	light_packet_interface pkt_interface5 = { 0 };
	light_packet_header pkt_header1 = { 0 };
	light_packet_header pkt_header2 = { 0 };
	light_packet_header pkt_header3 = { 0 };
	light_packet_header pkt_header4 = { 0 };
	light_packet_header pkt_header5 = { 0 };
	uint8_t* pkt_data = NULL;

	// Set interface properties 1
	pkt_interface1.link_type = 1; // link_type: ETHERNET
	pkt_interface1.name = "interface1";
	pkt_interface1.description = "Interface description";
	pkt_interface1.timestamp_resolution = 1000000000;

	// Set interface properties 2
	pkt_interface2.link_type = 1; // link_type: ETHERNET
	pkt_interface2.name = "interface2";
	pkt_interface2.description = "Interface description";
	pkt_interface2.timestamp_resolution = 1000000000;

	// Set interface properties 3
	pkt_interface3.link_type = 1; // link_type: ETHERNET
	pkt_interface3.name = "interface3";
	pkt_interface3.description = "Interface description";
	pkt_interface3.timestamp_resolution = 1000000000;

	// Set interface properties 4
	pkt_interface4.link_type = 1; // link_type: ETHERNET
	pkt_interface4.name = "interface4";
	pkt_interface4.description = "Interface description";
	pkt_interface4.timestamp_resolution = 1000000000;

	// Set interface properties 5
	pkt_interface5.link_type = 1; // link_type: ETHERNET
	pkt_interface5.name = "interface5";
	pkt_interface5.description = "Interface description";
	pkt_interface5.timestamp_resolution = 1000000000;

	// Set packet header 1
	struct timespec ts1 = { 1627228100 , 5000 };
	pkt_header1.timestamp = ts1;
	pkt_header1.captured_length = 256;
	pkt_header1.original_length = 1024;
	pkt_header1.flags = 0x1; // direction indicator
	pkt_header1.dropcount = 0;
	pkt_header1.queue = 1;
	pkt_header1.comment = "Packet comment";

	// Set packet header 2
	struct timespec ts2 = { 1627228200 , 5000 };
	pkt_header2.timestamp = ts2;
	pkt_header2.captured_length = 256;
	pkt_header2.original_length = 1024;
	pkt_header2.flags = 0x1; // direction indicator
	pkt_header2.dropcount = 0;
	pkt_header2.queue = 1;
	pkt_header2.comment = "Packet comment";

	// Set packet header 3
	struct timespec ts3 = { 1627228300 , 5000 };
	pkt_header3.timestamp = ts3;
	pkt_header3.captured_length = 256;
	pkt_header3.original_length = 1024;
	pkt_header3.flags = 0x1; // direction indicator
	pkt_header3.dropcount = 0;
	pkt_header3.queue = 1;
	pkt_header3.comment = "Packet comment";

	// Set packet header 4
	struct timespec ts4 = { 1627228400 , 5000 };
	pkt_header4.timestamp = ts4;
	pkt_header4.captured_length = 256;
	pkt_header4.original_length = 1024;
	pkt_header4.flags = 0x1; // direction indicator
	pkt_header4.dropcount = 0;
	pkt_header4.queue = 1;
	pkt_header4.comment = "Packet comment";

	// Set packet header 5
	struct timespec ts5 = { 1627228500 , 5000 };
	pkt_header5.timestamp = ts5;
	pkt_header5.captured_length = 256;
	pkt_header5.original_length = 1024;
	pkt_header5.flags = 0x1; // direction indicator
	pkt_header5.dropcount = 0;
	pkt_header5.queue = 1;
	pkt_header5.comment = "Packet comment";

	// Pkt content - all match
	pkt_data = (uint8_t*)malloc(pkt_header1.captured_length * sizeof(uint8_t));
	memset(pkt_data, '-', pkt_header1.captured_length);
        light_write_interface_block(writer, &pkt_interface1);
        light_write_interface_block(writer, &pkt_interface2);
        light_write_interface_block(writer, &pkt_interface3);
        light_write_interface_block(writer, &pkt_interface4);
        light_write_interface_block(writer, &pkt_interface5);

	light_write_packet(writer, &pkt_interface1, &pkt_header1, pkt_data);
	light_write_packet(writer, &pkt_interface2, &pkt_header2, pkt_data);
	light_write_packet(writer, &pkt_interface3, &pkt_header3, pkt_data);
	light_write_packet(writer, &pkt_interface4, &pkt_header4, pkt_data);
	light_write_packet(writer, &pkt_interface5, &pkt_header5, pkt_data);
	free(pkt_data);
	light_pcapng_close(writer);
	return 0;
}
