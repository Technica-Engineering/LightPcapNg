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
#include "light_util.h"

#include <stdlib.h>
#include <string.h>

light_block light_create_block(uint32_t type, const uint32_t* body, uint32_t body_length)
{
	light_block pcapng_block = calloc(1, sizeof(struct light_block_t));
	uint32_t actual_size = 0;
	int32_t body_size;

	pcapng_block->type = type;

	PADD32(body_length, &actual_size);

	pcapng_block->total_length = actual_size; // This value MUST be a multiple of 4.
	body_size = actual_size - 2 * sizeof(pcapng_block->total_length) - sizeof(pcapng_block->type);

	if (body_size > 0) {
		pcapng_block->body = calloc(1, body_size);
		memcpy(pcapng_block->body, body, body_size);
	}

	return pcapng_block;
}

void light_free_option(light_option option)
{
	free(option->data);
	free(option);
}
