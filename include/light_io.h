// Copyright (c) 2020 Technica Engineering GmbH
// Copyright (c) 2019 TMEIC Corporation - Robert Kriener

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

#ifndef INCLUDE_LIGHT_IO_H_
#define INCLUDE_LIGHT_IO_H_

#include "light_export.h"

#include <stdio.h>

typedef struct light_file_t* light_file;

LIGHT_API light_file LIGHT_API_CALL light_io_open(const char* file_name, const char* mode);

LIGHT_API size_t LIGHT_API_CALL light_io_read(light_file fd, void* buf, size_t count);
LIGHT_API size_t LIGHT_API_CALL light_io_write(light_file fd, const void* buf, size_t count);

LIGHT_API int LIGHT_API_CALL light_io_seek(light_file fd, long int offset, int origin);
LIGHT_API int LIGHT_API_CALL light_io_flush(light_file fd);
LIGHT_API int LIGHT_API_CALL light_io_close(light_file fd);

#endif /* INCLUDE_LIGHT_IO_H_ */
