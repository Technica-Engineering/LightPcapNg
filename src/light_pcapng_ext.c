// light_pcapng_ext.c
// Created on: Nov 14, 2016

// Copyright (c) 2016

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
#include "light_platform.h"
#include "light_debug.h"
#include "light_util.h"
#include "light_internal.h"
#include "light_debug.h"

#include <stdlib.h>
#include <string.h>


struct _light_pcapng_t
{
	light_pcapng pcapng;
	light_pcapng_file_info* file_info;
	light_file file;
};

char* __alloc_option_string(light_pcapng pcapng, uint16_t option_code) {

	light_option opt = light_get_option(pcapng, option_code);
	if (opt == NULL)
	{
		return NULL;
	}
	uint16_t length = light_get_option_length(opt);

	char* opt_string = calloc(length + 1, sizeof(char));
	memcpy(opt_string, (char*)light_get_option_data(opt), length);

	return opt_string;
}

static light_pcapng_file_info* __create_file_info(light_pcapng pcapng_head)
{
	uint32_t type = LIGHT_UNKNOWN_DATA_BLOCK;

	if (pcapng_head == NULL)
		return NULL;

	light_get_block_info(pcapng_head, LIGHT_INFO_TYPE, &type, NULL);

	if (type != LIGHT_SECTION_HEADER_BLOCK)
		return NULL;

	light_pcapng_file_info* file_info = calloc(1, sizeof(light_pcapng_file_info));

	struct _light_section_header* section_header;

	light_get_block_info(pcapng_head, LIGHT_INFO_BODY, &section_header, NULL);
	file_info->major_version = section_header->major_version;
	file_info->minor_version = section_header->minor_version;

	file_info->hardware_desc = __alloc_option_string(pcapng_head, LIGHT_OPTION_SHB_HARDWARE);
	file_info->os_desc = __alloc_option_string(pcapng_head, LIGHT_OPTION_SHB_OS);
	file_info->user_app_desc = __alloc_option_string(pcapng_head, LIGHT_OPTION_SHB_USERAPP);
	file_info->file_comment = __alloc_option_string(pcapng_head, LIGHT_OPTION_COMMENT);

	file_info->interfaces_count = 0;

	return file_info;
}

static uint64_t __power_of(uint64_t x, uint64_t y)
{
	int i;
	uint64_t res = 1;

	for (i = 0; i < y; i++)
		res *= x;

	return res;
}

static void __append_interface_block_to_file_info(const light_pcapng interface_block, light_pcapng_file_info* info)
{
	struct _light_interface_description_block* idb;
	light_option ts_resolution_option = NULL;

	if (info->interfaces_count >= MAX_SUPPORTED_INTERFACE_BLOCKS)
		return;

	light_get_block_info(interface_block, LIGHT_INFO_BODY, &idb, NULL);

	light_packet_interface lif = { 0 };

	lif.link_type = idb->link_type;
	lif.name = __alloc_option_string(interface_block, 2);
	lif.description = __alloc_option_string(interface_block, 3);

	ts_resolution_option = light_get_option(interface_block, LIGHT_OPTION_IF_TSRESOL);
	if (ts_resolution_option == NULL)
	{
		lif.timestamp_resolution = 1000000;
	}
	else
	{
		int8_t tsresol = *((int8_t*)light_get_option_data(ts_resolution_option));
		if (tsresol >= 0)
		{
			lif.timestamp_resolution = __power_of(10, tsresol);
		}
		else
		{
			lif.timestamp_resolution = __power_of(2, -tsresol);
		}
	}

	info->interfaces[info->interfaces_count++] = lif;
	info->interfaces_count++;
}

light_pcapng_t* light_pcapng_open_read(const char* file_path, bool read_all_interfaces)
{
	DCHECK_NULLP(file_path, return NULL);

	light_pcapng_t* pcapng = calloc(1, sizeof(struct _light_pcapng_t));
	pcapng->file = light_open(file_path, LIGHT_OREAD);

	if (pcapng->file == NULL) {
		return NULL;
	}

	//The first thing inside an NG capture is the section header block
	//When the file is opened we need to go ahead and read that out
	light_read_record(pcapng->file, &pcapng->pcapng);
	//Prase stuff out of the section header
	pcapng->file_info = __create_file_info(pcapng->pcapng);

	//If they requested to read all interfaces we must fast forward through file and find them all up front
	if (read_all_interfaces)
	{
		//Bookmark our current location
		long currentPos = ftell(pcapng->file->file);
		while (pcapng->pcapng != NULL)
		{
			light_read_record(pcapng->file, &pcapng->pcapng);
			uint32_t type = LIGHT_UNKNOWN_DATA_BLOCK;
			light_get_block_info(pcapng->pcapng, LIGHT_INFO_TYPE, &type, NULL);
			if (type == LIGHT_INTERFACE_BLOCK)
				__append_interface_block_to_file_info(pcapng->pcapng, pcapng->file_info);
		}
		//Should be at and of file now, if not something broke!!!
		if (!feof(pcapng->file->file))
		{
			light_pcapng_release(pcapng->pcapng);
			return NULL;
		}
		//Ok got to end of file so reset back to bookmark
		fseek(pcapng->file->file, currentPos, SEEK_SET);
	}

	light_pcapng_release(pcapng->pcapng);
	pcapng->pcapng = NULL;

	return pcapng;
}

light_pcapng_t* light_pcapng_open_write(const char* file_path, light_pcapng_file_info* file_info, int compression_level)
{
	DCHECK_NULLP(file_info, return NULL);
	DCHECK_NULLP(file_path, return NULL);

	light_pcapng_t* pcapng = calloc(1, sizeof(struct _light_pcapng_t));

	pcapng->file = light_open_compression(file_path, LIGHT_OWRITE, compression_level);
	pcapng->file_info = file_info;

	if (pcapng->file == NULL) {
		return NULL;
	}

	pcapng->pcapng = NULL;

	struct _light_section_header section_header;
	section_header.byteorder_magic = BYTE_ORDER_MAGIC;
	section_header.major_version = file_info->major_version;
	section_header.minor_version = file_info->minor_version;
	section_header.section_length = 0xFFFFFFFFFFFFFFFFULL;
	light_pcapng blocks_to_write = light_alloc_block(LIGHT_SECTION_HEADER_BLOCK, (const uint32_t*)&section_header, sizeof(section_header) + 3 * sizeof(uint32_t));

	if (file_info->file_comment)
	{
		light_option new_opt = light_create_option(LIGHT_OPTION_COMMENT, strlen(file_info->file_comment), file_info->file_comment);
		light_add_option(blocks_to_write, blocks_to_write, new_opt, false);
	}

	if (file_info->hardware_desc)
	{
		light_option new_opt = light_create_option(LIGHT_OPTION_SHB_HARDWARE, strlen(file_info->hardware_desc), file_info->hardware_desc);
		light_add_option(blocks_to_write, blocks_to_write, new_opt, false);
	}

	if (file_info->os_desc)
	{
		light_option new_opt = light_create_option(LIGHT_OPTION_SHB_OS, strlen(file_info->os_desc), file_info->os_desc);
		light_add_option(blocks_to_write, blocks_to_write, new_opt, false);
	}

	if (file_info->user_app_desc)
	{
		light_option new_opt = light_create_option(LIGHT_OPTION_SHB_USERAPP, strlen(file_info->user_app_desc), file_info->user_app_desc);
		light_add_option(blocks_to_write, blocks_to_write, new_opt, false);
	}

	light_pcapng next_block = blocks_to_write;
	int i = 0;
	for (i = 0; i < file_info->interfaces_count; i++)
	{
		light_packet_interface	lif = file_info->interfaces[i];
		struct _light_interface_description_block interface_block;
		interface_block.link_type = lif.link_type;
		interface_block.reserved = 0;
		interface_block.snapshot_length = 0;

		light_pcapng iface_block_pcapng = light_alloc_block(LIGHT_INTERFACE_BLOCK, (const uint32_t*)&interface_block, sizeof(struct _light_interface_description_block) + 3 * sizeof(uint32_t));
		light_add_block(next_block, iface_block_pcapng);
		next_block = iface_block_pcapng;
	}

	light_pcapng_to_file_stream(blocks_to_write, pcapng->file);


	light_pcapng_release(blocks_to_write);

	return pcapng;
}

light_pcapng_t* light_pcapng_open_append(const char* file_path)
{
	DCHECK_NULLP(file_path, return NULL);

	light_pcapng_t* pcapng = light_pcapng_open_read(file_path, true);
	DCHECK_NULLP(pcapng, return NULL);

	light_close(pcapng->file);
	pcapng->file = light_open(file_path, LIGHT_OAPPEND);

	light_pcapng_release(pcapng->pcapng);
	pcapng->pcapng = NULL;

	return pcapng;
}

light_pcapng_file_info* light_create_default_file_info()
{
	light_pcapng_file_info* default_file_info = calloc(1, sizeof(light_pcapng_file_info));
	memset(default_file_info, 0, sizeof(light_pcapng_file_info));
	default_file_info->major_version = 1;
	return default_file_info;
}

light_pcapng_file_info* light_create_file_info(const char* os_desc, const char* hardware_desc, const char* user_app_desc, const char* file_comment)
{
	light_pcapng_file_info* info = light_create_default_file_info();

	if (os_desc != NULL)
	{
		size_t os_len = strlen(os_desc);
		info->os_desc = calloc(os_len + 1, sizeof(char));
		memcpy(info->os_desc, os_desc, os_len);
	}

	if (hardware_desc != NULL)
	{
		size_t hw_len = strlen(hardware_desc);
		info->hardware_desc = calloc(hw_len + 1, sizeof(char));
		memcpy(info->hardware_desc, hardware_desc, hw_len);
	}

	if (user_app_desc != NULL)
	{
		size_t app_len = strlen(user_app_desc);
		info->user_app_desc = calloc(app_len + 1, sizeof(char));
		memcpy(info->user_app_desc, user_app_desc, app_len);
	}

	if (file_comment != NULL)
	{
		size_t comment_len = strlen(file_comment);
		info->file_comment = calloc(comment_len + 1, sizeof(char));
		memcpy(info->file_comment, file_comment, comment_len);
	}

	return info;
}

void light_free_file_info(light_pcapng_file_info* info)
{
	for (size_t i = 0; i < info->interfaces_count; i++)
	{
		light_packet_interface lif = info->interfaces[i];
		free(lif.name);
		free(lif.description);
	}
	free(info->user_app_desc);
	free(info->file_comment);
	free(info->hardware_desc);
	free(info->os_desc);
	free(info);
}

light_pcapng_file_info* light_pcang_get_file_info(light_pcapng_t* pcapng)
{
	DCHECK_NULLP(pcapng, return NULL);
	return pcapng->file_info;
}

int light_get_next_packet(light_pcapng_t* pcapng, light_packet_interface* lif, light_packet_header* packet_header, const uint8_t** packet_data)
{
	uint32_t type = LIGHT_UNKNOWN_DATA_BLOCK;

	light_read_record(pcapng->file, &pcapng->pcapng);

	//End of file or something is broken!
	if (pcapng == NULL)
		return 0;

	light_get_block_info(pcapng->pcapng, LIGHT_INFO_TYPE, &type, NULL);

	while (pcapng->pcapng != NULL && type != LIGHT_ENHANCED_PACKET_BLOCK && type != LIGHT_SIMPLE_PACKET_BLOCK)
	{
		if (type == LIGHT_INTERFACE_BLOCK)
			__append_interface_block_to_file_info(pcapng->pcapng, pcapng->file_info);

		light_read_record(pcapng->file, &pcapng->pcapng);
		if (pcapng->pcapng == NULL)
			break;
		light_get_block_info(pcapng->pcapng, LIGHT_INFO_TYPE, &type, NULL);
	}

	*packet_data = NULL;

	if (pcapng->pcapng == NULL)
		return 0;

	if (type == LIGHT_ENHANCED_PACKET_BLOCK)
	{
		struct _light_enhanced_packet_block* epb = NULL;

		light_get_block_info(pcapng->pcapng, LIGHT_INFO_BODY, &epb, NULL);

		packet_header->captured_length = epb->capture_packet_length;
		packet_header->original_length = epb->original_capture_length;
		uint64_t timestamp = epb->timestamp_high;
		timestamp = timestamp << 32;
		timestamp += epb->timestamp_low;

		*lif = pcapng->file_info->interfaces[epb->interface_id];
		uint64_t ts_res = lif->timestamp_resolution;

		uint64_t ts_secs = timestamp / ts_res;
		uint64_t ts_frac = timestamp % ts_res;
		uint64_t ts_nsec = ts_frac * 1000000000 / ts_res;

		packet_header->timestamp.tv_sec = ts_secs;
		packet_header->timestamp.tv_nsec = ts_nsec;

		packet_header->flags = 0;
		light_option flags_opt = light_get_option(pcapng->pcapng, LIGHT_OPTION_EPB_FLAGS);
		if (flags_opt != NULL)
		{
			packet_header->flags = *(uint32_t*)light_get_option_data(flags_opt);
		}

		packet_header->dropcount = 0;
		light_option dropcount_opt = light_get_option(pcapng->pcapng, LIGHT_OPTION_EPB_DROPCOUNT);
		if (dropcount_opt != NULL)
		{
			packet_header->dropcount = *(uint64_t*)light_get_option_data(dropcount_opt);
		}

		*packet_data = (uint8_t*)epb->packet_data;
	}

	else if (type == LIGHT_SIMPLE_PACKET_BLOCK)
	{
		struct _light_simple_packet_block* spb = NULL;
		light_get_block_info(pcapng->pcapng, LIGHT_INFO_BODY, &spb, NULL);

		*packet_header = (const light_packet_header){ 0 };
		packet_header->captured_length = spb->original_packet_length;
		packet_header->original_length = spb->original_packet_length;

		*lif = pcapng->file_info->interfaces[0];
		*packet_data = (uint8_t*)spb->packet_data;
	}

	packet_header->comment = __alloc_option_string(pcapng->pcapng, LIGHT_OPTION_COMMENT);

	return 1;
}

static const uint8_t NSEC_PRECISION = 9;

void light_write_packet(light_pcapng_t* pcapng, const light_packet_interface* lif, const light_packet_header* packet_header, const uint8_t* packet_data)
{
	DCHECK_NULLP(pcapng, return);
	DCHECK_NULLP(packet_header, return);
	DCHECK_NULLP(packet_data, return);

	if (pcapng->file == NULL) {
		return;
	}

	size_t iface_id = 0;
	for (iface_id = 0; iface_id < pcapng->file_info->interfaces_count; iface_id++)
	{
		light_packet_interface iface = pcapng->file_info->interfaces[iface_id];
		if (iface.link_type == lif->link_type && iface.timestamp_resolution == lif->timestamp_resolution)
			break;
	}

	light_pcapng blocks_to_write = NULL;

	// in case interface ID of packet block to be written does not exist - was not read previously
	if (iface_id >= pcapng->file_info->interfaces_count)
	{
		struct _light_interface_description_block interface_block = { 0 };
		interface_block.link_type = lif->link_type;

		light_pcapng iface_block_pcapng = light_alloc_block(LIGHT_INTERFACE_BLOCK, (const uint32_t*)&interface_block, sizeof(struct _light_interface_description_block) + 3 * sizeof(uint32_t));

		// let all written packets has a timestamp resolution in nsec - this way we will not loose the precision;
		// when a possibility to write interface blocks is added, the precision should be taken from them
		light_option resolution_option = light_create_option(LIGHT_OPTION_IF_TSRESOL, sizeof(NSEC_PRECISION), (uint8_t*)&NSEC_PRECISION);
		light_add_option(NULL, iface_block_pcapng, resolution_option, false);

		blocks_to_write = iface_block_pcapng;
		__append_interface_block_to_file_info(iface_block_pcapng, pcapng->file_info);
	}

	size_t option_size = sizeof(struct _light_enhanced_packet_block) + packet_header->captured_length;
	PADD32(option_size, &option_size);
	uint8_t* epb_memory = calloc(1, option_size);

	struct _light_enhanced_packet_block* epb = (struct _light_enhanced_packet_block*)epb_memory;
	epb->interface_id = iface_id;

	struct timespec ts = packet_header->timestamp;
	uint64_t timestamp = ts.tv_sec * (uint64_t)1e9 + (uint64_t)ts.tv_nsec;

	epb->timestamp_high = timestamp >> 32;
	epb->timestamp_low = timestamp & 0xFFFFFFFF;

	epb->capture_packet_length = packet_header->captured_length;
	epb->original_capture_length = packet_header->original_length;

	memcpy(epb->packet_data, packet_data, packet_header->captured_length);

	light_pcapng packet_block_pcapng = light_alloc_block(LIGHT_ENHANCED_PACKET_BLOCK, (const uint32_t*)epb_memory, option_size + 3 * sizeof(uint32_t));
	free(epb_memory);

	if (packet_header->comment)
	{
		light_option comment_opt = light_create_option(LIGHT_OPTION_COMMENT, strlen(packet_header->comment), packet_header->comment);
		light_add_option(NULL, packet_block_pcapng, comment_opt, false);
	}
	if (packet_header->flags)
	{
		light_option flags_opt = light_create_option(LIGHT_OPTION_EPB_FLAGS, 8, &packet_header->flags);
		light_add_option(NULL, packet_block_pcapng, flags_opt, false);
	}
	if (packet_header->dropcount)
	{
		light_option dropcount_opt = light_create_option(LIGHT_OPTION_EPB_DROPCOUNT, 8, &packet_header->dropcount);
		light_add_option(NULL, packet_block_pcapng, dropcount_opt, false);
	}

	if (blocks_to_write == NULL)
		blocks_to_write = packet_block_pcapng;
	else
		light_add_block(blocks_to_write, packet_block_pcapng);

	light_pcapng_to_file_stream(blocks_to_write, pcapng->file);

	light_pcapng_release(blocks_to_write);
}

void light_pcapng_close(light_pcapng_t* pcapng)
{
	DCHECK_NULLP(pcapng, return);

	light_pcapng_release(pcapng->pcapng);
	pcapng->pcapng = NULL;
	if (pcapng->file != NULL)
	{
		light_close(pcapng->file);
	}
	light_free_file_info(pcapng->file_info);
	free(pcapng);
}

void light_pcapng_flush(light_pcapng_t* pcapng)
{
	fflush(pcapng->file->file);
}
