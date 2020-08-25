// light_manipulate.c
// Created on: Jul 23, 2016

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
#include "light_internal.h"
#include "light_util.h"

#include <stdlib.h>
#include <string.h>


light_option light_create_option(const uint16_t option_code, const uint16_t option_length, void *option_value)
{
	uint16_t size = 0;
	light_option option = calloc(1, sizeof(struct _light_option));

	PADD32(option_length, &size);
	option->custom_option_code = option_code;
	option->option_length = option_length;

	option->data = calloc(size, sizeof(uint8_t));
	memcpy(option->data, option_value, option_length);

	return option;
}

int light_add_option(light_pcapng section, light_pcapng pcapng, light_option option, light_boolean copy)
{
	size_t option_size = 0;
	light_option option_list = NULL;

	if (option == NULL) {
		return LIGHT_INVALID_ARGUMENT;
	}

	if (copy == LIGHT_TRUE) {
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

		if (iterator->custom_option_code != 0) {
			// Add terminator option.
			iterator->next_option = calloc(1, sizeof(struct _light_option));
			option_size += 4;
		}
		pcapng->options = option_list;
	}
	else {
		light_option current = pcapng->options;
		while (current->next_option && current->next_option->custom_option_code != 0) {
			current = current->next_option;
		}

		light_option opt_endofopt = current->next_option;
		current->next_option = option_list;
		option_list->next_option = opt_endofopt;
	}

	pcapng->block_total_length += option_size;

	if (__is_section_header(section) == 1) {
		struct _light_section_header *shb = (struct _light_section_header *)section->block_body;
		shb->section_length += option_size;
	}
	else if (section != NULL) {
		PCAPNG_WARNING("PCAPNG block is not section header!");
	}

	return LIGHT_SUCCESS;
}

int light_update_option(light_pcapng section, light_pcapng pcapng, light_option option)
{
	light_option iterator = pcapng->options;
	uint16_t old_data_size, new_data_size;

	while (iterator != NULL) {
		if (iterator->custom_option_code == option->custom_option_code) {
			break;
		}
		iterator = iterator->next_option;
	}

	if (iterator == NULL) {
		return light_add_option(section, pcapng, option, LIGHT_TRUE);
	}

	if (iterator->option_length != option->option_length) {
		PADD32(option->option_length, &new_data_size);
		PADD32(iterator->option_length, &old_data_size);

		int data_size_diff = (int)new_data_size - (int)old_data_size;
		pcapng->block_total_length += data_size_diff;

		if (__is_section_header(section) == 1) {
			struct _light_section_header *shb = (struct _light_section_header *)section->block_body;
			shb->section_length += data_size_diff;
		}
		else {
			PCAPNG_WARNING("PCAPNG block is not section header!");
		}

		iterator->option_length = option->option_length;
		free(iterator->data);
		iterator->data = calloc(new_data_size, sizeof(uint8_t));
	}

	memcpy(iterator->data, option->data, iterator->option_length);

	return LIGHT_SUCCESS;
}

int light_add_block(light_pcapng block, light_pcapng next_block)
{
	block->next_block = next_block;
	return LIGHT_SUCCESS;
}

int light_subcapture(const light_pcapng section, light_boolean (*predicate)(const light_pcapng), light_pcapng *subcapture)
{
	if (__is_section_header(section) == 0) {
		PCAPNG_ERROR("Invalid section header");
		return LIGHT_INVALID_SECTION;
	}

	// Root section header is automatically included into the subcapture.
	light_pcapng root = __copy_block(section, LIGHT_FALSE);
	light_pcapng iterator = root;
	light_pcapng next_block = section->next_block;

	while (next_block != NULL) {
		// Predicate functions applies to all block types, including section header blocks.
		if (!!predicate(next_block) == LIGHT_TRUE) {
			iterator->next_block = __copy_block(next_block, LIGHT_FALSE);
			iterator = iterator->next_block;
		}
		next_block = next_block->next_block;
	}

	*subcapture = root;
	return __validate_section(*subcapture);

}
