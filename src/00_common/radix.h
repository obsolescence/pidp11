 /* radix.h: utilities for converting numbers in different systems

   Copyright (c) 2012-2016, Joerg Hoppe
   j_hoppe@t-online.de, www.retrocmp.com

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   JOERG HOPPE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


   22-Mar-2012  JH      created
*/

#ifndef RADIX_H_
#define RADIX_H_

#include <stdint.h>

char *radix_u642str(uint64_t value, int radix, int bitlen, int use_prefix);
char *radix_uint2str(unsigned value, int radix, int bitlen, int use_prefix);

int radix_str2u64(uint64_t *value, int radix, char *buffer);
int radix_str2uint(unsigned *value, int radix, char *buffer);

char *radix_getname_char(int radix);
char *radix_getname_short(int radix);
char *radix_getname_long(int radix);

#endif /* RADIX_H_ */
