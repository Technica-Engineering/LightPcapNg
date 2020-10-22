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

// Documentation from: https://github.com/pcapng/pcapng

static light_option __parse_options(uint32_t** memory, const int32_t max_len)
{
	if (max_len <= 0) {
		return NULL;
	}
	else {
		light_option opt = calloc(1, sizeof(struct light_option_t));
		uint16_t actual_length;
		uint16_t allignment = sizeof(uint32_t);

		uint16_t* local_memory = (uint16_t*)*memory;
		uint16_t remaining_size;

		opt->code = *local_memory++;
		opt->length = *local_memory++;

		actual_length = (opt->length % allignment) == 0 ?
			opt->length :
			(opt->length / allignment + 1) * allignment;

		if (actual_length > 0) {
			opt->data = calloc(1, actual_length);
			memcpy(opt->data, local_memory, actual_length);
			local_memory += (sizeof(**memory) / sizeof(*local_memory)) * (actual_length / allignment);
		}

		*memory = (uint32_t*)local_memory;
		remaining_size = max_len - actual_length - 2 * sizeof(*local_memory);

		if (opt->code == 0) {
			DCHECK_ASSERT(opt->length, 0);
			DCHECK_ASSERT(remaining_size, 0);

			if (remaining_size) {
				// XXX: Treat the remaining data as garbage and discard it form the trace.
				*memory += remaining_size / sizeof(uint32_t);
			}
		}
		else {
			opt->next_option = __parse_options(memory, remaining_size);
		}

		return opt;
	}
}

/// <summary>
/// Provided with a block pointer with populated type and length, this will actually parse out the body and options
/// </summary>
/// <param name="current">Block pointer with type and length filled out</param>
/// <param name="local_data">Pointer to data which constitutes block body</param>
/// <param name="block_start">Pointer to the start of the block data</param>
void parse_by_type(light_block current, const uint32_t* local_data, const uint32_t* block_start)
{
	switch (current->type)
	{
	case LIGHT_SECTION_HEADER_BLOCK:
	{
		struct _light_section_header* shb = calloc(1, sizeof(struct _light_section_header));
		light_option opt = NULL;
		uint32_t version;
		int32_t local_offset;

		shb->byteorder_magic = *local_data++;
		// TODO check byte order.
		version = *local_data++;
		shb->major_version = version & 0xFFFF;
		shb->minor_version = (version >> 16) & 0xFFFF;
		shb->section_length = *((uint64_t*)local_data);
		local_data += 2;

		current->body = (uint32_t*)shb;
		local_offset = (size_t)local_data - (size_t)block_start;
		opt = __parse_options((uint32_t**)&local_data, current->total_length - local_offset - sizeof(current->total_length));
		current->options = opt;
	}
	break;

	case LIGHT_INTERFACE_BLOCK:
	{
		struct _light_interface_description_block* idb = calloc(1, sizeof(struct _light_interface_description_block));
		light_option opt = NULL;
		uint32_t link_reserved = *local_data++;
		int32_t local_offset;

		idb->link_type = link_reserved & 0xFFFF;
		idb->reserved = (link_reserved >> 16) & 0xFFFF;
		idb->snapshot_length = *local_data++;
		current->body = (uint32_t*)idb;
		local_offset = (size_t)local_data - (size_t)block_start;
		opt = __parse_options((uint32_t**)&local_data, current->total_length - local_offset - sizeof(current->total_length));
		current->options = opt;
	}
	break;

	case LIGHT_ENHANCED_PACKET_BLOCK:
	{
		struct _light_enhanced_packet_block* epb = NULL;
		light_option opt = NULL;
		uint32_t interface_id = *local_data++;
		uint32_t timestamp_high = *local_data++;
		uint32_t timestamp_low = *local_data++;
		uint32_t captured_packet_length = *local_data++;
		uint32_t original_packet_length = *local_data++;
		int32_t local_offset;
		uint32_t actual_len = 0;

		PADD32(captured_packet_length, &actual_len);

		epb = calloc(1, sizeof(struct _light_enhanced_packet_block) + actual_len);
		epb->interface_id = interface_id;
		epb->timestamp_high = timestamp_high;
		epb->timestamp_low = timestamp_low;
		epb->capture_packet_length = captured_packet_length;
		epb->original_capture_length = original_packet_length;

		memcpy(epb->packet_data, local_data, captured_packet_length);
		local_data += actual_len / sizeof(uint32_t);
		current->body = (uint32_t*)epb;
		local_offset = (size_t)local_data - (size_t)block_start;
		opt = __parse_options((uint32_t**)&local_data, current->total_length - local_offset - sizeof(current->total_length));
		current->options = opt;
	}
	break;

	case LIGHT_SIMPLE_PACKET_BLOCK:
	{
		struct _light_simple_packet_block* spb = NULL;
		uint32_t original_packet_length = *local_data++;
		uint32_t actual_len = current->total_length - 2 * sizeof(current->total_length) - sizeof(current->type) - sizeof(original_packet_length);

		spb = calloc(1, sizeof(struct _light_enhanced_packet_block) + actual_len);
		spb->original_packet_length = original_packet_length;

		memcpy(spb->packet_data, local_data, actual_len);
		local_data += actual_len / sizeof(uint32_t);
		current->body = (uint32_t*)spb;
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
			local_data += raw_size / (sizeof(*local_data));
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
void light_read_block(light_file fd, light_block* block)
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
	uint32_t blockType, blockSize, bytesRead;
	bytesRead = light_io_read(fd, &blockType, sizeof(blockType));
	if (bytesRead != sizeof(blockType))
	{
		current = NULL;
		return;
	}

	//A block remains to be read so allocate here
	current = calloc(1, sizeof(struct light_block_t));
	DCHECK_NULLP(current, return);
	current->type = blockType;

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

	//rules for file say this must be on 32bit boundary
	assert((current->total_length % 4) == 0);

	//Pull out the block contents from the file
	const uint32_t bytesToRead = current->total_length - 2 * sizeof(blockSize) - sizeof(blockType);
	uint32_t* local_data = calloc(bytesToRead, 1);
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
	//Verify the two sizes match!!
	if (blockSize != current->total_length || bytesRead != sizeof(blockSize))
	{
		free(current);
		free(local_data);
		current = NULL;
		return;
	}

	//This funciton needs a pointer to the "start" of the block which we don't actually have, but the block body always just has 8 bytes before it
	//So we just cheat by decrementing the data pointer back 8 bytes;
	parse_by_type(current, local_data, local_data - 2);

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
