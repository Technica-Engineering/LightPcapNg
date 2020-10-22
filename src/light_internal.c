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

#include "light_debug.h"
#include "light_pcapng.h"
#include "light_util.h"

#include <stdlib.h>
#include <string.h>

size_t __get_option_total_size(const light_option option)
{
	size_t size = 0;
	light_option iter = option;
	while (iter != NULL) {
		uint16_t actual_length;
		PADD32(option->length, &actual_length);
		size += 4 + actual_length;
		iter = option->next_option;
	}
	return size;
}

light_option __copy_option(const light_option option)
{
	if (option == NULL) {
		return NULL;
	}

	size_t current_size = 0;
	light_option copy = calloc(1, sizeof(struct light_option_t));

	PADD32(option->length, &current_size);

	copy->code = option->code;
	copy->length = option->length;
	copy->data = calloc(1, current_size);
	memcpy(copy->data, option->data, option->length);

	copy->next_option = __copy_option(option->next_option);

	return copy;
}

bool __is_section_header(const light_block section)
{
	if (section != NULL) {
		return section->type == LIGHT_SECTION_HEADER_BLOCK;
	}
	return false;
}

light_option light_create_option(const uint16_t option_code, const uint16_t option_length, const void* option_value)
{
	uint16_t size = 0;
	light_option option = calloc(1, sizeof(struct light_option_t));

	PADD32(option_length, &size);
	option->code = option_code;
	option->length = option_length;

	option->data = calloc(size, sizeof(uint8_t));
	memcpy(option->data, option_value, option_length);

	return option;
}

int light_add_option(light_block section, light_block pcapng, light_option option, bool copy)
{
	size_t option_size = 0;
	light_option option_list = NULL;

	if (option == NULL) {
		return LIGHT_INVALID_ARGUMENT;
	}

	if (copy == true) {
		option_list = __copy_option(option);
	}
	else {
		option_list = option;
	}

	option_size = __get_option_total_size(option_list);

	if (pcapng->options == NULL) {
		light_option iterator = option_list;
		while (iterator->next_option != NULL) {
			iterator = iterator->next_option;
		}

		if (iterator->code != 0) {
			// Add terminator option.
			iterator->next_option = calloc(1, sizeof(struct light_option_t));
			option_size += 4;
		}
		pcapng->options = option_list;
	}
	else {
		light_option current = pcapng->options;
		while (current->next_option && current->next_option->code != 0) {
			current = current->next_option;
		}

		light_option opt_endofopt = current->next_option;
		current->next_option = option_list;
		option_list->next_option = opt_endofopt;
	}

	pcapng->total_length += option_size;

	if (__is_section_header(section)) {
		struct _light_section_header* shb = (struct _light_section_header*)section->body;
		shb->section_length += option_size;
	}
	else if (section != NULL) {
		PCAPNG_ERROR("PCAPNG block is not section header!");
	}

	return LIGHT_SUCCESS;
}

int light_update_option(light_block section, light_block block, light_option option)
{
	light_option iterator = block->options;
	uint16_t old_data_size, new_data_size;

	while (iterator != NULL) {
		if (iterator->code == option->code) {
			break;
		}
		iterator = iterator->next_option;
	}

	if (iterator == NULL) {
		return light_add_option(section, block, option, true);
	}

	if (iterator->length != option->length) {
		PADD32(option->length, &new_data_size);
		PADD32(iterator->length, &old_data_size);

		int data_size_diff = (int)new_data_size - (int)old_data_size;
		block->total_length += data_size_diff;

		if (__is_section_header(section)) {
			struct _light_section_header* shb = (struct _light_section_header*)section->body;
			shb->section_length += data_size_diff;
		}
		else {
			PCAPNG_ERROR("PCAPNG block is not section header!");
		}

		iterator->length = option->length;
		free(iterator->data);
		iterator->data = calloc(new_data_size, sizeof(uint8_t));
	}

	memcpy(iterator->data, option->data, iterator->length);

	return LIGHT_SUCCESS;
}
