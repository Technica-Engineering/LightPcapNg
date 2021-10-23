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

#include "light_pcapng.h"

#include "light_debug.h"
#include "light_util.h"
#include "light_io.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "endianness.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// Documentation from: https://github.com/pcapng/pcapng

void fix_endianness_section_header(struct _light_section_header* sh, const bool swap_endianness)
{
	if (swap_endianness) {
		// we do not swap the magic
		// sh->byteorder_magic = bswap32(sh->byteorder_magic);
		sh->major_version = bswap16(sh->major_version);
		sh->minor_version = bswap16(sh->minor_version);
		sh->section_length = bswap64(sh->section_length);
	}
}

void fix_endianness_interface_description_block(struct _light_interface_description_block* idb, const bool swap_endianness)
{
	if (swap_endianness) {
		idb->link_type = bswap16(idb->link_type);
		idb->reserved = bswap16(idb->reserved);
		idb->snapshot_length = bswap32(idb->snapshot_length);
	}
}

void fix_endianness_enhanced_packet_block(struct _light_enhanced_packet_block* epb, const bool swap_endianness)
{
	if (swap_endianness) {
		epb->interface_id = bswap32(epb->interface_id);
		epb->timestamp_high = bswap32(epb->timestamp_high);
		epb->timestamp_low = bswap32(epb->timestamp_low);
		epb->capture_packet_length = bswap32(epb->capture_packet_length);
		epb->original_capture_length = bswap32(epb->original_capture_length);
	}
}

void fix_endianness_simple_packet_block(struct _light_simple_packet_block* spb, const bool swap_endianness)
{
	if (swap_endianness) {
		spb->original_packet_length = bswap32(spb->original_packet_length);
	}
}

void fix_endianness_option(struct light_option_t* opt, const bool swap_endianness)
{
	if (swap_endianness) {
		opt->code = bswap16(opt->code);
		opt->length = bswap16(opt->length);
	}
}

static light_option __parse_options(const uint8_t** memory, const int32_t max_len, const bool swap_endianness)
{
	if (max_len <= 0) {
		return NULL;
	}
	// size of code and length
	const int32_t header_size = sizeof(uint16_t) + sizeof(uint16_t);
	const int32_t allignment = sizeof(uint32_t);
	if (max_len < header_size) {
		// Garbage data, not enough to read header
		*memory += max_len;
		return NULL;
	}

	light_option opt = calloc(1, sizeof(struct light_option_t));
	
	int32_t remaining_size = max_len;

	opt->code = *(uint16_t*)*memory;
	*memory += sizeof(uint16_t);
	opt->length = *(uint16_t*)*memory;
	*memory += sizeof(uint16_t);
	remaining_size -= header_size;
	fix_endianness_option(opt, swap_endianness);

	int32_t actual_length = (opt->length % allignment) == 0 ?
		opt->length :
		(opt->length / allignment + 1) * allignment;

	if (actual_length > remaining_size) {
		// We got invalid length value
		// Discard the option and the rest of the memory
		free(opt);
		*memory += remaining_size;
		return NULL;
	}

	if (actual_length > 0) {
		opt->data = calloc(1, actual_length);
		memcpy(opt->data, *memory, actual_length);
		*memory += actual_length;
		remaining_size += actual_length;
	}

	if (opt->code == 0) {
		DCHECK_ASSERT(opt->length, 0);
		DCHECK_ASSERT(remaining_size, 0);

		if (remaining_size) {
			// XXX: Treat the remaining data as garbage and discard it form the trace.
			*memory += remaining_size;
		}
	}
	else {
		opt->next_option = __parse_options(memory, remaining_size, swap_endianness);
	}

	return opt;
}

/// <summary>
/// Provided with a block pointer with populated type and length, this will actually parse out the body and options
/// </summary>
/// <param name="current">Block pointer with type and length filled out</param>
/// <param name="local_data">Pointer to data which constitutes block body</param>
/// <param name="local_data">Pointer to data which constitutes block body</param>
/// <param name="byte_order_magic">Used only for Section Headers since we need to look ahead</param>
void parse_by_type(light_block current, const uint8_t* local_data, uint32_t byte_order_magic, const bool swap_endianness)
{
	const uint8_t* original_start = local_data;

	switch (current->type)
	{
	case LIGHT_SECTION_HEADER_BLOCK:
	{
		struct _light_section_header* shb = calloc(1, sizeof(struct _light_section_header));

		//shb->byteorder_magic = *(uint32_t*)local_data;
		//local_data += 4;
		shb->byteorder_magic = byte_order_magic;
		shb->major_version = *(uint16_t*)local_data;
		local_data += 2;
		shb->minor_version = *(uint16_t*)local_data;
		local_data += 2;
		shb->section_length = *((uint64_t*)local_data);
		local_data += 8;

		fix_endianness_section_header(shb, swap_endianness);

		current->body = (uint8_t*)shb;
		int32_t local_offset = (size_t)local_data - (size_t)original_start + (size_t)8 + (size_t)4;
		light_option opt = __parse_options(&local_data, current->total_length - local_offset - sizeof(current->total_length), swap_endianness);
		current->options = opt;
	}
	break;

	case LIGHT_INTERFACE_BLOCK:
	{
		struct _light_interface_description_block* idb = calloc(1, sizeof(struct _light_interface_description_block));

		idb->link_type = *(uint16_t*)local_data;
		local_data += 2;
		idb->reserved = *(uint16_t*)local_data;
		local_data += 2;
		idb->snapshot_length = *(uint32_t*)local_data;
		local_data += 4;

		fix_endianness_interface_description_block(idb, swap_endianness);

		current->body = (uint8_t*)idb;
		int32_t local_offset = (size_t)local_data - (size_t)original_start + (size_t)8;
		light_option opt = __parse_options(&local_data, current->total_length - local_offset - sizeof(current->total_length), swap_endianness);
		current->options = opt;
	}
	break;

	case LIGHT_ENHANCED_PACKET_BLOCK:
	{
		struct _light_enhanced_packet_block* epb = NULL;

		uint32_t len = *(uint32_t*)(local_data + 12);
		if (swap_endianness) len = bswap32(len);
		len = MIN(len, current->total_length - 32);
		uint32_t actual_len = 0;
		PADD32(len, &actual_len);

		epb = calloc(1, sizeof(struct _light_enhanced_packet_block) + actual_len);
		epb->interface_id = *(uint32_t*)local_data;
		local_data += 4;
		epb->timestamp_high = *(uint32_t*)local_data;
		local_data += 4;
		epb->timestamp_low = *(uint32_t*)local_data;
		local_data += 4;
		epb->capture_packet_length = *(uint32_t*)local_data;
		local_data += 4;
		epb->original_capture_length = *(uint32_t*)local_data;
		local_data += 4;

		fix_endianness_enhanced_packet_block(epb, swap_endianness);
		len = MIN(len, epb->capture_packet_length);
		len = MIN(len, epb->original_capture_length);
		memcpy(epb->packet_data, local_data, len);
		local_data += actual_len;

		current->body = (uint8_t*)epb;
		int32_t local_offset = (size_t)local_data - (size_t)original_start + (size_t)8;
		light_option opt = __parse_options(&local_data, current->total_length - local_offset - sizeof(current->total_length), swap_endianness);
		current->options = opt;
	}
	break;

	case LIGHT_SIMPLE_PACKET_BLOCK:
	{
		struct _light_simple_packet_block* spb = NULL;
		uint32_t original_packet_length = *(uint32_t*)(local_data);
		local_data += 4;
		uint32_t actual_len = current->total_length - 2 * sizeof(current->total_length) - sizeof(current->type) - sizeof(original_packet_length);

		spb = calloc(1, sizeof(struct _light_enhanced_packet_block) + actual_len);
		spb->original_packet_length = original_packet_length;
		fix_endianness_simple_packet_block(spb, swap_endianness);

		memcpy(spb->packet_data, local_data, actual_len);
		local_data += actual_len;

		current->body = (uint8_t*)spb;
		current->options = NULL; // No options defined by the standard for this block type.
	}
	break;

	default: // Could not find registered block type. Copying data as RAW.
	{
		uint32_t raw_size = current->total_length - 2 * sizeof(current->total_length) - sizeof(current->type);
		if (raw_size > 0)
		{
			current->body = calloc(raw_size, 1);
			memcpy(current->body, local_data, raw_size);
			local_data += raw_size;
		}
		else
		{
			current->body = NULL;
		}
	}
	break;
	}
}

/// <summary>
/// Returns a full record as read out of the file
/// </summary>
/// <param name="fd">File to read from</param>
/// <returns>The block read from the file - may contain sub blocks</returns>
void light_read_block(light_file fd, light_block* block, bool* swap_endianness)
{
	//FYI general block structure is like this

	//  0               1               2               3
	//  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// |                          Block Type                           |
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// |                     Block Total Length                        |
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// |                         Block Body                            |
	// |              variable length, padded to 32 bits               |
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	// |                     Block Total Length                        |
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	if (block && *block) {
		light_free_block(*block);
	}
	*block = NULL;

	light_block current;

	//See the block type, if end of file this will tell us
	uint32_t blockType, blockSize;
	size_t bytesRead;
	bytesRead = light_io_read(fd, &blockType, sizeof(blockType));
	bool section_header = (blockType == LIGHT_SECTION_HEADER_BLOCK);

	if (bytesRead != sizeof(blockType))
	{
		current = NULL;
		return;
	}

	//A block remains to be read so allocate here
	current = calloc(1, sizeof(struct light_block_t));
	DCHECK_NULLP(current, return);

	//From here on if there is malformed block data we need to release the block we just allocated!

	//Get block size
	bytesRead = light_io_read(fd, &current->total_length, sizeof(blockSize));
	if (bytesRead != sizeof(blockSize))
	{
		// EOF
		free(current);
		current = NULL;
		return;
	}

	// Lets peek ahead and figure out the endianess.
	uint32_t byte_order_magic = 0;
	if (section_header) {
		assert(current->total_length != 0);
		bytesRead = light_io_read(fd, &byte_order_magic, 4);
		assert(bytesRead == 4);
		bytesRead = light_io_seek(fd, -4L, SEEK_CUR);

		*swap_endianness = (byte_order_magic != BYTE_ORDER_MAGIC);

		if (*swap_endianness) {
			assert(byte_order_magic == 0x4D3C2B1A);
		}
	}

	if (*swap_endianness) {
		blockType = bswap32(blockType);
		current->total_length = bswap32(current->total_length);
	}
	current->type = blockType;

	//rules for file say this must be on 32bit boundary
	assert((current->total_length % 4) == 0);

	//Pull out the block contents from the file
	uint32_t bytesToRead = current->total_length - 2 * sizeof(blockSize) - sizeof(blockType);
	if (section_header) {
		bytesToRead -= 4;
	}
	uint8_t* local_data = calloc(bytesToRead, 1);
	bytesRead = light_io_read(fd, local_data, bytesToRead);
	if (bytesRead != bytesToRead)
	{
		// EOF
		free(current);
		free(local_data);
		current = NULL;
		return;
	}

	//Need to move file to next record so read the footer, which is just the record length repeated
	bytesRead = light_io_read(fd, &blockSize, sizeof(blockSize));
	if (*swap_endianness) blockSize = bswap32(blockSize);
	//Verify the two sizes match!!
	if (blockSize != current->total_length || bytesRead != sizeof(blockSize))
	{
		free(current);
		free(local_data);
		current = NULL;
		return;
	}

	parse_by_type(current, local_data, byte_order_magic, *swap_endianness);

	free(local_data);
	*block = current;
}

static void __free_option(light_option option)
{
	if (option == NULL)
		return;

	__free_option(option->next_option);

	option->next_option = NULL;
	free(option->data);
	free(option);
}

void light_free_block(light_block block)
{
	if (block != NULL) {
		__free_option(block->options);
		free(block->body);
		free(block);
	}
}

static int __option_count(light_option option)
{
	if (option == NULL)
		return 0;

	return 1 + __option_count(option->next_option);
}

uint32_t* __options_to_mem(const light_option option, size_t* size)
{
	if (option == NULL) {
		*size = 0;
		return NULL;
	}

	size_t next_size;
	uint32_t* next_option = __options_to_mem(option->next_option, &next_size);
	uint32_t* current_mem;
	size_t current_size = 0;

	PADD32(option->length, &current_size);

	current_mem = calloc(sizeof(uint32_t) + current_size + next_size, 1);
	current_mem[0] = option->code | (option->length << 16);
	memcpy(&current_mem[1], option->data, current_size);
	memcpy(&current_mem[1 + current_size / 4], next_option, next_size);

	current_size += sizeof(option->code) + sizeof(option->length) + next_size;
	*size = current_size;

	free(next_option);

	return current_mem;
}

size_t light_write_block(light_file file, const light_block block)
{
	size_t body_length = block->total_length - 12; // 2 lengths and type
	size_t options_length = 0;
	uint32_t* options_mem = __options_to_mem(block->options, &options_length);
	body_length -= options_length;

	light_io_write(file, &block->type, sizeof(block->type));
	light_io_write(file, &block->total_length, sizeof(block->total_length));

	light_io_write(file, block->body, body_length);
	light_io_write(file, options_mem, options_length);

	light_io_write(file, &block->total_length, sizeof(block->total_length));

	free(options_mem);

	return block->total_length;
}
