/* mcout.h: module to print text output in multiple columns.

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

#ifndef MCOUT_H_
#define MCOUT_H_


#include <stdlib.h>

// maximum size for an output line
#define MCOUT_MAXLINESIZE	1024


typedef struct {


	// private
	int	stringcache_size ; // max amount of strings that are saved
	int	stringcache_fill ; // no of strings in cache
	char	**stringcache; // array of string
} mcout_t ;

// setup module
void mcout_init(mcout_t *_this, int max_strings) ;

// save a string for later output
void mcout_puts(mcout_t *_this, char	*txt) ;
void mcout_printf(mcout_t *_this, char *fmt, ...) ;

// print and free strings
void mcout_flush(mcout_t *_this, FILE *fout, int max_linewidth, char *col_sep, int first_col_then_row) ;

char idx2selectorchar(int idx) ;
unsigned selectorchar2idx(char c) ;


#endif /* MCOUT_H_ */
