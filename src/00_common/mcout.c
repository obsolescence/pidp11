/* mcout.c: module to print text output in multiple columns.

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


   30-Nov-2012  JH      created
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "mcout.h"

// setup module
//	'first_col_then_row': if 1, strings are printed in 1st column downwards, then in 2nd column downward, etc.
// if 0: strings are printed in 1st row, then in 2nd row, etc.
void mcout_init(mcout_t *_this, int max_strings)
{

	int i;

	_this->stringcache_size = max_strings;
	_this->stringcache = (char **) malloc(_this->stringcache_size * sizeof(char *));
	_this->stringcache_fill = 0;
	for (i = 0; i < _this->stringcache_size; i++)
		_this->stringcache[i] = NULL;
}

// save a string for later output
void mcout_puts(mcout_t *_this, char *line)
{

	assert(_this->stringcache_fill <= _this->stringcache_size);
	_this->stringcache[_this->stringcache_fill] = strdup(line);
	_this->stringcache_fill++;

}

void mcout_printf(mcout_t *_this, char *fmt, ...)
{
	static char buffer[MCOUT_MAXLINESIZE];
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	vsprintf(buffer, fmt, arg_ptr);
	va_end(arg_ptr);

	mcout_puts(_this, buffer);
}

// print strings in multi column array and free strings.
// Next call must be to mcout_init() again!
void mcout_flush(mcout_t *_this, FILE *fout, int max_linewidth, char *col_sep,
		int first_col_then_row)
{
	int i, j;
	static char linebuff[MCOUT_MAXLINESIZE];
	int linecount; // number of output lines produced
	int colsep_len = strlen(col_sep);
	int col_width;
	int col_count;

	int line;
	int col;

	assert(max_linewidth < MCOUT_MAXLINESIZE);

	if (_this->stringcache_fill == 0)
		return; // nothing todo

	// calculate column count from max_linewidth
	// 1. calc widest string
	col_width = 0;
	for (i = 0; i < _this->stringcache_fill; i++) {
		int n = strlen(_this->stringcache[i]);
		if (col_width < n)
			col_width = n;
	}
	assert(col_width > 0);
	// calc column count
	col_count = 1;
	while ((col_count - 1) * colsep_len + col_count * col_width <= max_linewidth)
		col_count++;
	col_count--; // stops, if too large

	// mount line after line

	linecount = (_this->stringcache_fill / col_count) + 1;
	for (line = 0; line < linecount; line++) {
		linebuff[0] = '\0';
		for (col = 0; col < col_count; col++) {
			if (first_col_then_row)
				i = col * linecount + line; //
			else
				i = line * col_count + col;
			if (i < _this->stringcache_fill) {
				// col/row position is not past cache end
				char *curstr = _this->stringcache[i]; // string to print in line, col
				int curstr_len = strlen(curstr);
				int linebuff_len = strlen(linebuff);
				char *linebuff_end = linebuff + linebuff_len; // point to end of cell

				// place s in to line buffer: indent right by ""
				int indent = col * col_width + col * colsep_len; // space for text and space for col seps
				while (linebuff_len < indent) {
					*linebuff_end++ = ' '; // pad with spaces up to 'indent'
					linebuff_len++;
				}
				*linebuff_end = '\0';

				// insert column separator, but not before first column
				if (linebuff_len > colsep_len)
					for (j = 0; j < colsep_len; j++)
						linebuff[linebuff_len - colsep_len + j] = col_sep[j];

				if (curstr_len > col_width)
					curstr_len = col_width; // trunc to colwidth
				strncat(linebuff_end, curstr, curstr_len);
				linebuff_end += curstr_len;
				*linebuff_end = '\0';
			}
		}
		fputs(linebuff, fout);
		fputc('\n', fout); // print CR
	}

	// free allocated cache
	for (i = 0; i < _this->stringcache_size; i++)
		if (_this->stringcache[i] != NULL )
			free(_this->stringcache[i]);

	free(_this->stringcache);
	_this->stringcache = NULL;
}

/*
 * Test program
 */
int mcout_selftest(void)
{
	mcout_t mcout; // formatting object
	int n = 11;
	int i;

	printf("          1         2         3         4         5         6         7         \n");
	printf("01234567890123456789012345678901234567890123456789012345678901234567890123456789\n");

	printf("Test 1: print %d strings in columns\n", n);
	// "line 0      line 4      line 8"
	// "line 1      line 5      line 9"
	// "line 2      line 6"
	// "line 3      line 7"
	// (cell is 17, separator is 3)
	mcout_init(&mcout, 100);
	for (i = 0; i < n; i++)
		mcout_printf(&mcout, "string %d", i);
	mcout_flush(&mcout, stdout, 80, " | ", /*first_col_then_row*/1);

	printf("Test 2: print %d strings in rows\n", n);
	// "line 0      line 1      line 2"
	// "line 3      line 4      line 5"
	// "line 6      line 7      line 8"
	// "line 9"
	mcout_init(&mcout, 100);
	for (i = 0; i < n; i++)
		mcout_printf(&mcout, "string %d", i);
	mcout_flush(&mcout, stdout, 80, " | ", /*first_col_then_row*/0);

	printf("Test 3: like #2, but with larger strings\n");
	mcout_init(&mcout, 100);
	for (i = 0; i < n; i++) {
		mcout_printf(&mcout, "string %d - %s", i, (i % 2) ? "abcdefghijkl" : "abc");
	}
	mcout_flush(&mcout, stdout, 80, " | ", /*first_col_then_row*/0);

	printf("          1         2         3         4         5         6         7         \n");
	printf("01234567890123456789012345678901234567890123456789012345678901234567890123456789\n");

	return EXIT_SUCCESS;
}

static char selector_chars[] =
{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A',
		'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '\0' };
/*
 * get a selector char for item <index>
 * Is used to produce single char for user selctable menu items
 */
char idx2selectorchar(int idx)
{
	assert(idx >= 0);
	assert(idx < sizeof(selector_chars));
	return selector_chars[idx];
}

/*
 * gibt den Index eines chars zurück.
 * -1 = not found
 */
unsigned selectorchar2idx(char c)
{
	int idx;
	for (idx = 0; idx < sizeof(selector_chars); idx++)
		if (selector_chars[idx] == c)
			return idx;
	return -1;
}
