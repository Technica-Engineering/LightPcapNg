// Copyright (c) 2026 Technica Engineering GmbH

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
#include "light_pcapng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

int verify_dsb_block(const char* filename, const light_packet_decryption expected_decryption_block) {
    const uint32_t expected_secret_type = expected_decryption_block.secret_type;
    const uint8_t* expected_key = expected_decryption_block.key;
    const uint32_t key_len = expected_decryption_block.key_size;
    const uint8_t* expected_comment = expected_decryption_block.comment;

    light_file fd = light_io_open(filename, "rb");
    if (fd == NULL) {
        printf("can not open output file %s \n", filename);
        return 1;
    }
    light_block block = NULL;
    bool swap_endianess = false;
    light_read_block(fd, &block, &swap_endianess);

    bool dsb_found = false;
    while (block != NULL) {
        if(block->type == LIGHT_DECRYPTION_SECRETS_BLOCK) {
            dsb_found = true;
            break;
        }
        light_read_block(fd, &block, &swap_endianess);
    }

    light_io_close(fd);

    if (!dsb_found) {
        if(block != NULL) {
            light_free_block(block);
        }
        printf("DSB not found\n");
        return 1;
    }

    //Verify Secrets Type and Key Length (Offsets  0 and 4)
    struct _light_decryption_secrets_block* dsb_block = (struct _light_decryption_secrets_block*) (block->body);
    struct light_option_t* dsb_options = (struct light_option_t*) (block->options);

    uint32_t written_secret_type = dsb_block->secrets_type;
    uint32_t written_key_len = dsb_block->secrets_len;

    if (written_secret_type != expected_secret_type) {
        printf("Secret Type mismatch\n");
        light_free_block(block);
        return 1;
    }
    
    if (written_key_len != key_len) {
        printf("Key Length mismatch\n");
        light_free_block(block);
        return 1;
    }

    //Verify the actual Key Data (Offset 8)
    if (memcmp(dsb_block->key_data, expected_key, key_len) != 0) {
        printf("Key data corruption\n");
        light_free_block(block);
        return 1;
    }

    //Verify 32-bit Padding logic
    // Pcapng requires blocks to be multiples of 4 bytes.
    uint32_t key_padding = (4 - (key_len % 4)) % 4;
    for (uint32_t i = 0; i < key_padding; i++) {
        if (dsb_block->key_data[key_len + i] != 0) {
            printf("Padding bytes must be zero\n");
            light_free_block(block);
            return 1;
        }
    }

    uint16_t option_code = dsb_options->code;
    uint16_t option_len = dsb_options->length;
    uint8_t* option_data = dsb_options->data;

    if(option_code != LIGHT_OPTION_COMMENT) {
        printf("option code mismatch\n");
        light_free_block(block);
        return 1;
    }

    if(option_len != strlen(expected_comment)) {
        printf("option length mismatch\n");
        light_free_block(block);
        return 1;
    }

    if (memcmp(dsb_options->data, expected_comment, option_len) != 0) {
        printf("option data corruption\n");
        light_free_block(block);
        return 1;
    }

    light_free_block(block);
    printf("Verification Success: DSB block at %s was written correctly.\n", filename);
    return 0;
}

int main(int argc, const char** args) {

    if (argc < 2) {
		fprintf(stderr, "Usage %s <outfile>", args[0]);
		return 1;
	}
	
    const char* outfile = args[1];
    // 1. create and open a file in write mode
    light_pcapng pcap = light_pcapng_open(outfile, "wb");
    assert(pcap != NULL);

    // Set packet header
    light_packet_header pkt_header = { 0 };
	struct timespec ts = { 1627228100 , 5000 };
	pkt_header.timestamp = ts;
	pkt_header.captured_length = 256;
	pkt_header.original_length = 1024;
	pkt_header.flags = 0x1; // direction indicator
	pkt_header.dropcount = 0;
	pkt_header.queue = 1;
	pkt_header.comment = "Packet comment";

    light_packet_interface pkt_interface = { 0 };
    pkt_interface.link_type = 1; // link_type: ETHERNET
	pkt_interface.name = "interface1";
	pkt_interface.description = "Interface description";
	pkt_interface.timestamp_resolution = 1000000000;

    uint8_t* pkt_data = (uint8_t*)malloc(pkt_header.captured_length * sizeof(uint8_t));
	memset(pkt_data, '-', pkt_header.captured_length);
    light_write_packet(pcap, &pkt_interface, &pkt_header, pkt_data);

    // define dummy data (A 5-byte key to trigger 3 bytes of padding)
    uint8_t dummy_key[] = { 'T', 'L', 'S', '_', '1' };  
    light_packet_decryption decryption_block;
    decryption_block.secret_type = LIGHT_DSB_SECRET_TLSK; // 'TLSK'
    decryption_block.key = dummy_key;
    decryption_block.key_size = 5;
    decryption_block.comment = "TLS key comment";

    // add dsb block
    int result = light_write_decryption_block(pcap, &decryption_block);
    // close the file
    light_pcapng_close(pcap);
    free(pkt_data);

    assert(result == 0 && "Error writing DSB block");

    result = verify_dsb_block(outfile, decryption_block);
    assert (result == 0 && "Check DSB block is failed");

    
    return 0;
}
