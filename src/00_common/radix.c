 /* radix.c: utilities for converting numbers in different systems

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

   12-Mar-2016  JH      64bit printf/scanf fmt changed to PRI*64 (inttypes.h)
   22-Mar-2012  JH      created
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include "bitcalc.h"
#include "radix.h"

/*
 * convert to string
 * bitlen: if > 0 produce always as many digits as bit count of value needs
 * ocatal and hex numbers are file4ld with '0', deccimal ist filled with  ' '
 */
char *radix_u642str(uint64_t value, int radix, int bitlen, int use_prefix)
{
	static char buffer[80];
	int digitcount = 0;

	// run bitcalc_init()!
	if (bitlen > 0)
		digitcount = digitcount_from_bitlen(radix, bitlen);

	if (radix == 10)
	{
		sprintf(buffer, "%*"PRId64, digitcount, value);
	}
	else if (radix == 8)
		sprintf(buffer, "%s%0*"PRIo64, use_prefix ? "0" : "", digitcount, value);
	else if (radix == 16)
		sprintf(buffer, "%s%0*"PRIx64, use_prefix ? "0x" : "", digitcount, value);
	else
	{
		fprintf(stderr, "radix_u642str(): radix must be 10, 8 or 16\n");
		exit(1);
	}
	return buffer;
}

char *radix_uint2str(unsigned value, int radix, int bitlen, int use_prefix)
{
	static char buffer[80];
	// run bitcalc_init()!
	int digitcount = 0;
	if (bitlen > 0)
		digitcount = digitcount_from_bitlen(radix, bitlen);
	if (radix == 10)
		sprintf(buffer, "%0*u", digitcount, value);
	else if (radix == 8)
		sprintf(buffer, "%s%0*o", use_prefix ? "0" : "", digitcount, value);
	else if (radix == 16)
		sprintf(buffer, "%s%0*x", use_prefix ? "0x" : "", digitcount, value);
	else
	{
		fprintf(stderr, "radix_uint2str(): radix must be 10, 8 or 16\n");
		exit(1);
	}
	return buffer;
}

/*
 * parse string, with or without prefix
 * result: 0 = error, 1 = ok
 */
int radix_str2u64(uint64_t *value, int radix, char *buffer)
{
	int i, n;

	char *bufferlocase = strdup(buffer);
	for (i = 0; bufferlocase[i]; i++)
		bufferlocase[i] = tolower(bufferlocase[i]);

	if (radix == 10)
	{
		n = sscanf(bufferlocase, "%"PRId64, value);
	}
	else if (radix == 8)
	{
		n = sscanf(bufferlocase, "%"PRIo64, value);
		if (!n)
			n = sscanf(bufferlocase, "0%"PRIo64, value);
	}
	else if (radix == 16)
	{
		n = sscanf(bufferlocase, "%"PRIx64, value);
		if (!n)
			n = sscanf(bufferlocase, "0x%"PRIx64, value);
	}
	else
	{
		fprintf(stderr, "radix_str2u64(): radix must be 10, 8 or 16\n");
		exit(1);
	}
	free(bufferlocase);
	return (n > 0);
}

int radix_str2uint(unsigned *value, int radix, char *buffer)
{
	int i, n;

	char *bufferlocase = strdup(buffer);
	for (i = 0; bufferlocase[i]; i++)
		bufferlocase[i] = tolower(bufferlocase[i]);

	if (radix == 10)
	{
		n = sscanf(bufferlocase, "%u", value);
	}
	else if (radix == 8)
	{
		n = sscanf(bufferlocase, "%o", value);
		if (!n)
			n = sscanf(bufferlocase, "0%o", value);
	}
	else if (radix == 16)
	{
		n = sscanf(bufferlocase, "%x", value);
		if (!n)
			n = sscanf(bufferlocase, "0x%x", value);
	}
	else
	{
		fprintf(stderr, "radix_str2uint(): radix must be 10, 8 or 16\n");
		exit(1);
	}
	free(bufferlocase);
	return (n > 0);
}

char *radix_getname_char(int radix)
{
	if (radix == 10)
		return "d";
	else if (radix == 8)
		return "o";
	else if (radix == 16)
		return "h";
	else
	{
		fprintf(stderr, "radix_getname_char(): radix must be 10, 8 or 16\n");
		exit(1);
	}
}


/*
 * string representation of a radix
 */
char *radix_getname_short(int radix)
{
	if (radix == 10)
		return "dec";
	else if (radix == 8)
		return "oct";
	else if (radix == 16)
		return "hex";
	else
	{
		fprintf(stderr, "radix_getname_short(): radix must be 10, 8 or 16\n");
		exit(1);
	}
}

char *radix_getname_long(int radix)
{
	if (radix == 10)
		return "decimal";
	else if (radix == 8)
		return "octal";
	else if (radix == 16)
		return "hexadecimal";
	else
	{
		fprintf(stderr, "radix_getname_long(): radix must be 10, 8 or 16\n");
		exit(1);
	}
}
