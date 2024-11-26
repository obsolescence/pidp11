/* inputline.c: Advanced routines for user text input

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


   23-Feb-2012  JH      created


   Not called "readline", because this is the big Unix function for the same task.
   It is used here.

*/


#include <stdlib.h>
#include <stdio.h>

#include "inputline.h"

/*
 * get input from user
 * - shall be run in seperate thread, with abort/query status fucntionality
 * - shall use readline() under *ix, DOS support under Windows
 * - can use fgets in early development stages
 * - can read input from a file
 */

static FILE *inputline_file = NULL;

// reset input source and internal states
void inputline_init()
{
	// close file, if open
	if (inputline_file)
		fclose(inputline_file);
	inputline_file = NULL;
}

char *inputline(char *buffer, int buffer_size)
{
	char *s;
	int read_from_file;
	read_from_file = (inputline_file != NULL );
	if (read_from_file) {
		/*** read line from text file ***/
		if (fgets(buffer, buffer_size, inputline_file) == NULL ) {
			fclose(inputline_file);
			inputline_file = NULL;
			read_from_file = 0; // file empty, or unreadable
		} else {
			// remove terminating "\n"
			for (s = buffer; *s; s++)
				if (*s == '\n')
					*s = '\0';
		}
	}
	if (!read_from_file) {
		/*** read interactive ***/
		fgets(buffer, buffer_size, stdin);
		// remove terminating "\n"
		for (s = buffer; *s; s++)
			if (*s == '\n')
				*s = '\0';
	}
	return buffer;
}

void inputline_fopen(char *filename)
{
	inputline_file = fopen(filename, "r");
}

