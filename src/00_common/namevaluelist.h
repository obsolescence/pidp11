 /* namevaluelist.h: manage list of name=value pairs

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


   24-May-2012  JH      created
*/


#ifndef NAMEVALUELIST_H_
#define NAMEVALUELIST_H_

/*
 *  one variable
 *
 *
 */
typedef struct {
	char	*name ;
	int	value_int ;	// may only have int OR string value
	char	*value_string ;
} namevalue_t ;


#define NAMEVALUELIST_MAX_VARS	100


#ifndef NAMEVALUELIST_C_
// global list, terminated with NULL
extern namevalue_t namevaluelist[NAMEVALUELIST_MAX_VARS] ;
#endif

void namevaluelist_constructor() ; // init all

int	namevaluelist_get_int_value(char *name) ;
char *namevaluelist_get_string_value(char *name) ;

void	namevaluelist_set_int_value(char *name, int value) ;
void    namevaluelist_set_string_value(char *name, char *value) ;

#endif

