// Copyright (c) 2021 Technica Engineering GmbH
// This code is licensed under MIT license (see LICENSE for details)

#ifndef INCLUDE_LIGHT_IO_ZLIB_H_
#define INCLUDE_LIGHT_IO_ZLIB_H_

#if defined(LIGHT_USE_ZLIB)

#include "light_io.h"

light_file light_io_zlib_open(const char* filename, const char* mode);

#endif // LIGHT_USE_ZLIB

#endif // INCLUDE_LIGHT_IO_ZLIB_H_