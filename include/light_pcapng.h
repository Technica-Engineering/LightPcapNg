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

#ifndef LIGHT_PCAPNG_H_
#define LIGHT_PCAPNG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "light_special.h"
#include "light_io.h"
#include <stdbool.h>

	// block types

#define LIGHT_SECTION_HEADER_BLOCK  0x0A0D0D0A
#define LIGHT_INTERFACE_BLOCK       0x00000001
#define LIGHT_ENHANCED_PACKET_BLOCK 0x00000006
#define LIGHT_SIMPLE_PACKET_BLOCK   0x00000003

// option codes

#define LIGHT_OPTION_COMMENT               1
#define LIGHT_OPTION_SHB_HARDWARE          2
#define LIGHT_OPTION_SHB_OS                3
#define LIGHT_OPTION_SHB_APP               4
#define LIGHT_OPTION_IF_TSRESOL            9

#define LIGHT_OPTION_EPB_FLAGS             2
#define LIGHT_OPTION_EPB_DROPCOUNT         4
#define LIGHT_OPTION_EPB_QUEUE             6

#define BYTE_ORDER_MAGIC            0x1A2B3C4D

// error codes

#define LIGHT_SUCCESS           0
#define LIGHT_INVALID_SECTION  -1
#define LIGHT_OUT_OF_MEMORY    -2
#define LIGHT_INVALID_ARGUMENT -3
#define LIGHT_FAILURE          -5

// pcapng structures

	struct light_block_t {
		uint32_t type;
		uint32_t total_length;
		uint8_t* body;
		struct light_option_t* options;
	};

	struct light_option_t {
		uint16_t code;
		uint16_t length;
		uint8_t* data;
		struct light_option_t* next_option;
	};

	typedef struct light_block_t* light_block;
	typedef struct light_option_t* light_option;


	// endianness fixes
	void fix_endianness_section_header(struct _light_section_header* sh, const bool swap_endianness);
	void fix_endianness_interface_description_block(struct _light_interface_description_block* idb, const bool swap_endianness);
	void fix_endianness_enhanced_packet_block(struct _light_enhanced_packet_block* epb, const bool swap_endianness);
	void fix_endianness_simple_packet_block(struct _light_simple_packet_block* spb, const bool swap_endianness);

	// block functions

	// Read next record out of file, if you give an existing record I will free it for you
	// The returned record must be freed by either YOU or the next call to light_free_block!
	LIGHT_API void LIGHT_API_CALL light_read_block(light_file fd, light_block* block, bool *swap_endianess);

	LIGHT_API light_block LIGHT_API_CALL light_create_block(uint32_t type, const uint32_t* body, uint32_t body_length);

	LIGHT_API void LIGHT_API_CALL light_free_block(light_block pcapng);

	LIGHT_API size_t LIGHT_API_CALL light_write_block(light_file file, const light_block block);

	// option functions

	LIGHT_API light_option LIGHT_API_CALL light_create_option(const uint16_t option_code, const uint16_t option_length, const void* option_value);
	LIGHT_API void LIGHT_API_CALL light_free_option(light_option option);

	LIGHT_API light_option LIGHT_API_CALL light_find_option(const light_block block, uint16_t option_code);

	LIGHT_API int LIGHT_API_CALL light_add_option(light_block section, light_block block, light_option option, bool copy);
	LIGHT_API int LIGHT_API_CALL light_update_option(light_block section, light_block block, light_option option);


#ifdef __cplusplus
}
#endif

#endif /* LIGHT_PCAPNG_H_ */
