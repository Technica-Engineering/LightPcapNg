// Copyright (c) 2021 woidpointer@gmail.com

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef INCLUDE_EXPORT_H_
#define INCLUDE_EXPORT_H_

/* =====   LIGHT_API : control library symbols visibility   ===== */
#ifndef LIGHT_VISIBILITY
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define LIGHT_VISIBILITY __attribute__((visibility("default")))
#else
#define LIGHT_VISIBILITY
#endif
#endif

/* API call convention*/
#if WIN32
#define LIGHT_API_CALL __cdecl
#else
#define LIGHT_API_CALL
#endif

#if defined LIGHT_EXPORTS && (LIGHT_EXPORTS == 1)
#ifdef _MSC_VER
#define LIGHT_API __declspec(dllexport) LIGHT_VISIBILITY
#else
#define LIGHT_API LIGHT_VISIBILITY
#endif
#elif defined LIGHT_IMPORTS && (LIGHT_IMPORTS == 1)
#ifdef _MSC_VER
#define LIGHT_API __declspec(dllimport) LIGHT_VISIBILITY
#else
#define LIGHT_API LIGHT_VISIBILITY
#endif
#else
#define LIGHT_API LIGHT_VISIBILITY
#endif

#endif /* INCLUDE_EXPORT_H_ */
