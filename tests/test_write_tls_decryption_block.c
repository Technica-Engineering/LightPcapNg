// Copyright (c) 2026 Technica Engineering GmbH
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
#include <assert.h>

/**
 * validates that the DSB block was written correctly.
 */
int verify_dsb_block(const char* filename, const light_packet_decryption expected_decryption_block) {
    const uint32_t expected_secret_type = expected_decryption_block.secret_type;
    const uint8_t* expected_key = expected_decryption_block.key;
    const uint32_t key_len = expected_decryption_block.key_size;
    const uint8_t* expected_comment = expected_decryption_block.comment;

    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        printf("Failed to open file for verification\n");
        return 1;
    }

    //Read the very last 4 bytes of the file (Block Total Length)
    fseek(f, -4, SEEK_END);
    uint32_t trailing_len = 0;
    fread(&trailing_len, 4, 1, f);

    //Seek to the start of this block and read the whole dsb
    fseek(f, -((long)trailing_len), SEEK_END);
    uint8_t *buffer = (uint8_t *)malloc(trailing_len);

    if (buffer == NULL) {
        printf("Error allocating memory\n");
        fclose(f);
        return 1;
    }

    fread(buffer, 1, trailing_len, f);
    fclose(f);

    //Check Block Type (DSB is 0x0000000A)
    // We check both endian possibilities : big endian or little endian
    uint32_t block_type = *(uint32_t*)buffer;

    if (block_type != 0x0000000A && block_type != 0x0A000000) {
        printf("Invalid Block Type for DSB\n");
        free(buffer);
        return 1;
    }

    //Verify Secrets Type and Key Length (Offsets 8 and 12)
    uint32_t written_secret_type = *(uint32_t*)(buffer + 8);
    uint32_t written_key_len = *(uint32_t*)(buffer + 12);

    if (written_secret_type != expected_secret_type) {
        printf("Secret Type mismatch\n");
        free(buffer);
        return 1;
    }
    
    if (written_key_len != key_len) {
        printf("Key Length mismatch\n");
        free(buffer);
        return 1;
    }

    //Verify the actual Key Data (Offset 16)
    if (memcmp(buffer + 16, expected_key, key_len) != 0) {
        printf("Key data corruption\n");
        free(buffer);
        return 1;
    }

    //Verify 32-bit Padding logic
    // Pcapng requires blocks to be multiples of 4 bytes.
    uint32_t key_padding = (4 - (key_len % 4)) % 4;
    for (uint32_t i = 0; i < key_padding; i++) {
        if (buffer[16 + key_len + i] != 0) {
            printf("Padding bytes must be zero\n");
            free(buffer);
            return 1;
        }
    }

    uint32_t options_offset = 16 + key_len + key_padding;
    //Verify Comment Option (if provided)
    if (expected_comment != NULL) {
        uint16_t opt_code = *(uint16_t*)(buffer + options_offset);
        uint16_t opt_len = *(uint16_t*)(buffer + options_offset + 2);
        uint32_t expected_comment_len = (uint32_t)strlen(expected_comment);

        if (opt_code != 1) { // 1 is the code for 'opt_comment'
            printf("Comment option code mismatch\n");
            free(buffer);
            return 1;
        }
        if (opt_len != expected_comment_len) {
            printf("Comment length mismatch\n");
            free(buffer);
            return 1;
        }
        if (memcmp(buffer + options_offset + 4, expected_comment, opt_len) != 0) {
            printf("Comment text mismatch\n");
            free(buffer);
            return 1;
        }
        uint32_t opt_padding = (4 - (opt_len % 4)) % 4;
        options_offset += 4 + opt_len + opt_padding;
    }

    //Verify "End of options" (Must be 4 bytes of 0x00)
    uint32_t opt_end = *(uint32_t*)(buffer + options_offset);
    if (opt_end != 0) {
        printf("Missing or invalid end of options marker\n");
        free(buffer);
        return 1;
    }

    //Verify the trailing length matches the leading length
    uint32_t leading_len = *(uint32_t*)(buffer + 4);
    if (leading_len != trailing_len) {
        printf("Block length symmetry failed\n");
        free(buffer);
        return 1;
    }

    free(buffer);
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
