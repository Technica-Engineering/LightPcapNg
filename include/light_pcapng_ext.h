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

#ifndef LIGHT_PCAPNG_EXT_H_
#define LIGHT_PCAPNG_EXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "light_export.h"
#include "light_io.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef _WIN32  
#include <time.h>
#else
#include <sys/time.h>
#endif

struct light_pcapng_t;
typedef struct light_pcapng_t* light_pcapng;

typedef struct light_packet_interface {
	uint16_t link_type;
	char* name;
	char* description;
	uint64_t timestamp_resolution;

} light_packet_interface;

typedef struct light_packet_header {
	struct timespec timestamp;
	uint32_t captured_length;
	uint32_t original_length;

	char* comment;
	uint32_t flags;
	uint64_t dropcount;
	uint32_t queue;

} light_packet_header;

typedef struct light_file_info {
	uint16_t major_version;
	uint16_t minor_version;

	char *comment;
	char *hardware_desc;
	char *os_desc;
	char *app_desc;

} light_pcapng_file_info;


LIGHT_API light_pcapng LIGHT_API_CALL light_pcapng_open(const char* file_path, const char* mode);
LIGHT_API light_pcapng LIGHT_API_CALL light_pcapng_create(light_file file, const char* mode, light_pcapng_file_info* info);

LIGHT_API light_pcapng_file_info * LIGHT_API_CALL light_create_default_file_info();

LIGHT_API light_pcapng_file_info * LIGHT_API_CALL light_create_file_info(const char *os_desc, const char *hardware_desc, const char *app_desc, const char *comment);

LIGHT_API void LIGHT_API_CALL light_free_file_info(light_pcapng_file_info *info);

LIGHT_API light_pcapng_file_info * LIGHT_API_CALL light_pcang_get_file_info(light_pcapng pcapng);

LIGHT_API int LIGHT_API_CALL light_read_packet(light_pcapng pcapng, light_packet_interface* packet_interface, light_packet_header *packet_header, const uint8_t **packet_data);

LIGHT_API int LIGHT_API_CALL light_write_packet(light_pcapng pcapng, const light_packet_interface* packet_interface, const light_packet_header *packet_header, const uint8_t *packet_data);

LIGHT_API int LIGHT_API_CALL light_pcapng_close(light_pcapng pcapng);

LIGHT_API int LIGHT_API_CALL light_pcapng_flush(light_pcapng pcapng);

#ifdef __cplusplus
}
#endif

#endif /* LIGHT_PCAPNG_EXT_H_ */
