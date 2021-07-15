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
#include "light_pcapng.h"
#include "light_io.h"
#include "light_debug.h"
#include "light_util.h"
#include "light_debug.h"

#include <stdlib.h>
#include <string.h>

#include "endianness.h"


struct light_pcapng_t
{
	light_file file;

	light_pcapng_file_info* file_info;
	light_block current;

	size_t interfaces_count;
	light_packet_interface* interfaces;
	uint32_t section_interface_offset;

	bool swap_endianness;
};

char* __alloc_option_string(light_block pcapng, uint16_t option_code) {

	light_option opt = light_find_option(pcapng, option_code);
	if (opt == NULL)
	{
		return NULL;
	}

	char* opt_string = calloc(opt->length + 1, sizeof(char));
	memcpy(opt_string, (char*)opt->data, opt->length);

	return opt_string;
}

static light_pcapng_file_info* __create_file_info(light_block block)
{
	if (block == NULL)
	{
		return NULL;
	}

	if (block->type != LIGHT_SECTION_HEADER_BLOCK)
		return NULL;

	light_pcapng_file_info* file_info = calloc(1, sizeof(light_pcapng_file_info));

	struct _light_section_header* section_header = (struct _light_section_header*)block->body;

	file_info->major_version = section_header->major_version;
	file_info->minor_version = section_header->minor_version;

	file_info->hardware_desc = __alloc_option_string(block, LIGHT_OPTION_SHB_HARDWARE);
	file_info->os_desc = __alloc_option_string(block, LIGHT_OPTION_SHB_OS);
	file_info->app_desc = __alloc_option_string(block, LIGHT_OPTION_SHB_APP);
	file_info->comment = __alloc_option_string(block, LIGHT_OPTION_COMMENT);

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

static void __append_interface_block(light_pcapng pcapng, const light_block interface_block, const bool swap_endian)
{
	struct _light_interface_description_block* idb = (struct _light_interface_description_block*)interface_block->body;
	// no endianness conversion here because idb was already fixed
	light_option ts_resolution_option = NULL;

	light_packet_interface lif = { 0 };

	lif.link_type = idb->link_type;
	lif.name = __alloc_option_string(interface_block, 2);
	lif.description = __alloc_option_string(interface_block, 3);

	ts_resolution_option = light_find_option(interface_block, LIGHT_OPTION_IF_TSRESOL);
	if (ts_resolution_option == NULL)
	{
		lif.timestamp_resolution = 1000000;
	}
	else
	{
		int8_t tsresol = *((int8_t*)ts_resolution_option->data);
		if (tsresol >= 0)
		{
			lif.timestamp_resolution = __power_of(10, tsresol);
		}
		else
		{
			lif.timestamp_resolution = __power_of(2, -tsresol);
		}
	}

	pcapng->interfaces = realloc(pcapng->interfaces, sizeof(lif) * (pcapng->interfaces_count + 1));
	pcapng->interfaces[pcapng->interfaces_count] = lif;
	pcapng->interfaces_count++;
}

int light_pcapng_init(light_pcapng pcapng, light_pcapng_file_info* file_info)
{
	DCHECK_NULLP(pcapng, return LIGHT_INVALID_ARGUMENT);
	DCHECK_NULLP(pcapng->file, return LIGHT_INVALID_ARGUMENT);
	DCHECK_NULLP(file_info, return LIGHT_INVALID_ARGUMENT);

	pcapng->file_info = file_info;

	struct _light_section_header section_header;
	section_header.byteorder_magic = BYTE_ORDER_MAGIC;
	section_header.major_version = file_info->major_version;
	section_header.minor_version = file_info->minor_version;
	section_header.section_length = 0xFFFFFFFFFFFFFFFFULL;
	light_block section = light_create_block(LIGHT_SECTION_HEADER_BLOCK, (const uint32_t*)&section_header, sizeof(section_header) + 3 * sizeof(uint32_t));

	if (file_info->comment)
	{
		light_option new_opt = light_create_option(LIGHT_OPTION_COMMENT, strlen(file_info->comment), file_info->comment);
		light_add_option(section, section, new_opt, false);
	}

	if (file_info->hardware_desc)
	{
		light_option new_opt = light_create_option(LIGHT_OPTION_SHB_HARDWARE, strlen(file_info->hardware_desc), file_info->hardware_desc);
		light_add_option(section, section, new_opt, false);
	}

	if (file_info->os_desc)
	{
		light_option new_opt = light_create_option(LIGHT_OPTION_SHB_OS, strlen(file_info->os_desc), file_info->os_desc);
		light_add_option(section, section, new_opt, false);
	}

	if (file_info->app_desc)
	{
		light_option new_opt = light_create_option(LIGHT_OPTION_SHB_APP, strlen(file_info->app_desc), file_info->app_desc);
		light_add_option(section, section, new_opt, false);
	}

	light_write_block(pcapng->file, section);

	light_free_block(section);

	return LIGHT_SUCCESS;
}


light_pcapng light_pcapng_create(light_file file, const char* mode, light_pcapng_file_info* info)
{
	if (!file) {
		return NULL;
	}
	bool read = strstr(mode, "r") != NULL;
	bool write = strstr(mode, "w") != NULL;
	bool append = strstr(mode, "a") != NULL;
	bool update = strstr(mode, "+") != NULL;

	light_pcapng pcapng = calloc(1, sizeof(struct light_pcapng_t));
	pcapng->swap_endianness = false;
	pcapng->file = file;

	bool* swap_endianness = &(pcapng->swap_endianness);

	if (read) {
		//The first thing inside an NG capture is the section header block
		//When the file is opened we need to go ahead and read that out
		light_block section = NULL;
		light_read_block(pcapng->file, &section, swap_endianness);
		//Prase stuff out of the section header
		pcapng->file_info = __create_file_info(section);
		light_free_block(section);
		return pcapng;
	}

	//If they requested to append to existing file
	// then we need to read all existing interfaces
	if (append && update)
	{
		light_block block = NULL;
		while (1)
		{
			// go to beginning of file
			light_io_seek(file, 0, SEEK_SET);
			light_read_block(pcapng->file, &block, swap_endianness);
			if (block == NULL) {
				return pcapng;
			}
			if (block->type == LIGHT_SECTION_HEADER_BLOCK) {
				pcapng->section_interface_offset = pcapng->interfaces_count;
			}
			if (block->type == LIGHT_INTERFACE_BLOCK) {
				__append_interface_block(pcapng, block, *swap_endianness);
			}
		}
	}

	if ((append && !update) || write) {
		if (!info) {
			info = light_create_default_file_info();
		}
		light_pcapng_init(pcapng, info);
		return pcapng;
	}

	// getting here means we got an invalid mode as input
	free(pcapng);
	return NULL;
}

light_pcapng light_pcapng_open(const char* filename, const char* mode)
{
	light_file file = light_io_open(filename, mode);
	light_pcapng pcapng = light_pcapng_create(file, mode, NULL);
	if (pcapng == NULL) {
		// We create the file, but we were not able to use it
		light_io_close(file);
	}
	return pcapng;
}

light_pcapng_file_info* light_create_default_file_info()
{
	light_pcapng_file_info* default_file_info = calloc(1, sizeof(light_pcapng_file_info));
	default_file_info->major_version = 1;
	return default_file_info;
}

light_pcapng_file_info* light_create_file_info(const char* os_desc, const char* hardware_desc, const char* app_desc, const char* file_comment)
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

	if (app_desc != NULL)
	{
		size_t app_len = strlen(app_desc);
		info->app_desc = calloc(app_len + 1, sizeof(char));
		memcpy(info->app_desc, app_desc, app_len);
	}

	if (file_comment != NULL)
	{
		size_t comment_len = strlen(file_comment);
		info->comment = calloc(comment_len + 1, sizeof(char));
		memcpy(info->comment, file_comment, comment_len);
	}

	return info;
}

void light_free_file_info(light_pcapng_file_info* info)
{
	if (info != NULL) {
		free(info->app_desc);
		free(info->comment);
		free(info->hardware_desc);
		free(info->os_desc);
		free(info);
	}
}

light_pcapng_file_info* light_pcang_get_file_info(light_pcapng pcapng)
{
	DCHECK_NULLP(pcapng, return NULL);
	return pcapng->file_info;
}

int light_read_packet(light_pcapng pcapng, light_packet_interface* packet_interface, light_packet_header* packet_header, const uint8_t** packet_data)
{
	DCHECK_NULLP(pcapng, return 0);
	DCHECK_NULLP(packet_interface, return 0);
	DCHECK_NULLP(packet_header, return 0);
	DCHECK_NULLP(packet_data, return 0);

	light_free_block(pcapng->current);
	pcapng->current = NULL;
	light_block block = NULL;
	while (1)
	{
		light_read_block(pcapng->file, &block, &(pcapng->swap_endianness));
		if (block == NULL) {
			//End of file or something is broken
			return 0;
		}
		if (block->type == LIGHT_ENHANCED_PACKET_BLOCK || block->type == LIGHT_SIMPLE_PACKET_BLOCK)
		{
			break;
		}
		if (block->type == LIGHT_SECTION_HEADER_BLOCK) {
			pcapng->section_interface_offset = pcapng->interfaces_count;
			continue;
		}
		if (block->type == LIGHT_INTERFACE_BLOCK) {
			__append_interface_block(pcapng, block, pcapng->swap_endianness);
			continue;
		}
	}

	*packet_data = NULL;

	if (block->type == LIGHT_ENHANCED_PACKET_BLOCK)
	{
		struct _light_enhanced_packet_block* epb = (struct _light_enhanced_packet_block*)block->body;
		// no endianness fix needed since fixed before

		packet_header->captured_length = epb->capture_packet_length;
		packet_header->original_length = epb->original_capture_length;
		uint64_t timestamp = epb->timestamp_high;
		timestamp = timestamp << 32;
		timestamp += epb->timestamp_low;

		*packet_interface = pcapng->interfaces[pcapng->section_interface_offset + epb->interface_id];
		uint64_t ts_res = packet_interface->timestamp_resolution;

		uint64_t ts_secs = timestamp / ts_res;
		uint64_t ts_frac = timestamp % ts_res;
		uint64_t ts_nsec = ts_frac * 1000000000 / ts_res;

		packet_header->timestamp.tv_sec = (time_t)ts_secs;
		packet_header->timestamp.tv_nsec = (long)ts_nsec;

		packet_header->flags = 0;
		light_option flags_opt = light_find_option(block, LIGHT_OPTION_EPB_FLAGS);
		if (flags_opt != NULL)
		{
			packet_header->flags = *(uint32_t*)(flags_opt->data);
			if (pcapng->swap_endianness) bswap32(packet_header->flags);
		}

		packet_header->dropcount = 0;
		light_option dropcount_opt = light_find_option(block, LIGHT_OPTION_EPB_DROPCOUNT);
		if (dropcount_opt != NULL && dropcount_opt->length != 0)
		{
			packet_header->dropcount = *(uint64_t*)(dropcount_opt->data);
			if (pcapng->swap_endianness) bswap64(packet_header->dropcount);
		}

		*packet_data = (uint8_t*)epb->packet_data;
	}

	if (block->type == LIGHT_SIMPLE_PACKET_BLOCK)
	{
		struct _light_simple_packet_block* spb = (struct _light_simple_packet_block*)block->body;
		// no endianness fix needed since fixed before

		*packet_header = (const light_packet_header){ 0 };
		packet_header->captured_length = spb->original_packet_length;
		packet_header->original_length = spb->original_packet_length;

		*packet_interface = pcapng->interfaces[pcapng->section_interface_offset];
		*packet_data = (uint8_t*)spb->packet_data;
	}

	packet_header->comment = __alloc_option_string(block, LIGHT_OPTION_COMMENT);

	pcapng->current = block;

	return 1;
}

int safe_strcmp(char const* str1, char const* str2) {
	if (!str1 || !str2) {
		return str1 - str2;
	}
	return strcmp(str1, str2);
}

static const uint8_t NSEC_PRECISION = 9;

uint8_t get_timestamp_resolution_precision(uint64_t timestamp_resolution) {
	uint64_t time_value = timestamp_resolution;
	uint8_t precision = 0;
	while (time_value > 1) {
		time_value = time_value / 10;
		precision = precision + 1;
	}
	return precision;
}

int light_write_packet(light_pcapng pcapng, const light_packet_interface* packet_interface, const light_packet_header* packet_header, const uint8_t* packet_data)
{
	DCHECK_NULLP(pcapng, return LIGHT_INVALID_ARGUMENT);
	DCHECK_NULLP(packet_header, return LIGHT_INVALID_ARGUMENT);
	DCHECK_NULLP(packet_data, return LIGHT_INVALID_ARGUMENT);

	if (pcapng->file == NULL) {
		return LIGHT_INVALID_ARGUMENT;
	}

	size_t iface_id = pcapng->section_interface_offset;
	for (; iface_id < pcapng->interfaces_count; iface_id++)
	{
		light_packet_interface iface = pcapng->interfaces[iface_id];
		bool match = true;
		match = match && iface.link_type == packet_interface->link_type;
		match = match && safe_strcmp(iface.name, packet_interface->name) == 0;
		match = match && safe_strcmp(iface.description, packet_interface->description) == 0;
		match = match && iface.timestamp_resolution == packet_interface->timestamp_resolution;
		if (match) {
			break;
		}
	}

	// in case interface ID of packet block to be written does not exist - was not read previously
	if (iface_id >= pcapng->interfaces_count)
	{
		struct _light_interface_description_block interface_block = { 0 };
		interface_block.link_type = packet_interface->link_type;

		light_block iface_block_pcapng = light_create_block(LIGHT_INTERFACE_BLOCK, (const uint32_t*)&interface_block, sizeof(struct _light_interface_description_block) + 3 * sizeof(uint32_t));

		if (packet_interface->timestamp_resolution) {
			// get precision from timestamp resolution (number of 0s)
			uint8_t timestamp_precision = get_timestamp_resolution_precision(packet_interface->timestamp_resolution);
			// add precision to options
			light_option resolution_option = light_create_option(LIGHT_OPTION_IF_TSRESOL, sizeof(timestamp_precision), (uint8_t*)&timestamp_precision);
			light_add_option(NULL, iface_block_pcapng, resolution_option, false);
		}
		// if packet interface has a name, add it to the light options
		if (packet_interface->name)
		{
			light_option name_option = light_create_option(2, strlen(packet_interface->name) + 1, packet_interface->name);
			light_add_option(NULL, iface_block_pcapng, name_option, false);
		}

		// if packet interface has a description, add it to the light options
		if (packet_interface->description)
		{
			light_option description_option = light_create_option(3, strlen(packet_interface->description) + 1, packet_interface->description);
			light_add_option(NULL, iface_block_pcapng, description_option, false);
		}

		__append_interface_block(pcapng, iface_block_pcapng, false);
		light_write_block(pcapng->file, iface_block_pcapng);

		light_free_block(iface_block_pcapng);
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

	light_block packet_block_pcapng = light_create_block(LIGHT_ENHANCED_PACKET_BLOCK, (const uint32_t*)epb_memory, option_size + 3 * sizeof(uint32_t));
	free(epb_memory);

	if (packet_header->comment)
	{
		light_option comment_opt = light_create_option(LIGHT_OPTION_COMMENT, strlen(packet_header->comment), packet_header->comment);
		light_add_option(NULL, packet_block_pcapng, comment_opt, false);
	}
	if (packet_header->flags)
	{
		light_option flags_opt = light_create_option(LIGHT_OPTION_EPB_FLAGS, 4, &packet_header->flags);
		light_add_option(NULL, packet_block_pcapng, flags_opt, false);
	}
	if (packet_header->dropcount)
	{
		light_option dropcount_opt = light_create_option(LIGHT_OPTION_EPB_DROPCOUNT, 8, &packet_header->dropcount);
		light_add_option(NULL, packet_block_pcapng, dropcount_opt, false);
	}

	light_write_block(pcapng->file, packet_block_pcapng);

	light_free_block(packet_block_pcapng);

	return LIGHT_SUCCESS;
}

int light_pcapng_close(light_pcapng pcapng)
{
	DCHECK_NULLP(pcapng, return 0);

	light_free_block(pcapng->current);
	light_free_file_info(pcapng->file_info);

	for (size_t i = 0; i < pcapng->interfaces_count; i++)
	{
		light_packet_interface lif = pcapng->interfaces[i];
		free(lif.name);
		free(lif.description);
	}
	free(pcapng->interfaces);

	int res = 0;
	if (pcapng->file != NULL)
	{
		res = light_io_close(pcapng->file);
	}

	free(pcapng);
	return res;
}

int light_pcapng_flush(light_pcapng pcapng)
{
	return light_io_flush(pcapng->file);
}
