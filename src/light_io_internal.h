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

#ifndef INCLUDE_LIGHT_IO_INTERNAL_H_
#define INCLUDE_LIGHT_IO_INTERNAL_H_

#include "light_io.h"
#include <stdio.h> 

typedef size_t(*light_fn_read)(void* context, void* buf, size_t count);
typedef size_t(*light_fn_write)(void* context, const void* buf, size_t count);
typedef int(*light_fn_seek)(void* context, long int offset, int origin);
typedef long(*light_fn_tell)(void* context);
typedef int(*light_fn_flush)(void* context);
typedef int(*light_fn_close)(void* context);

struct light_file_t
{
	void* context;

	light_fn_read fn_read;
	light_fn_write fn_write;
	light_fn_seek fn_seek;
	light_fn_tell fn_tell;
	light_fn_flush fn_flush;
	light_fn_close fn_close;
};

#endif /* INCLUDE_LIGHT_IO_INTERNAL_H_ */
