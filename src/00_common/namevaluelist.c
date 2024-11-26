 /* namevaluelist.c: manage list of name=value pairs

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#define strcasecmp _stricmp	// grmbl
#endif

#define NAMEVALUELIST_C_

#include "namevaluelist.h"

// global list, terminated with NULL
namevalue_t *namevaluelist[NAMEVALUELIST_MAX_VARS];

/*
 * get index of first empty element in list
 */
static int findEndOfListIndex()
{
	int i;
	for (i = 0; i < NAMEVALUELIST_MAX_VARS; i++)
		if (namevaluelist[i] == NULL)
			return i;
	return 0; // error!
}

static namevalue_t *searchByName(char *name)
{
	int i;
	for (i = 0; i < NAMEVALUELIST_MAX_VARS; i++)
	{
		namevalue_t *nv = namevaluelist[i];
		if (nv == NULL)
			return NULL ; // end of list
		if (!strcasecmp(nv->name, name))
			return nv; // found
	}
	return NULL; // not reached
}

/*
 * add empty var to end of list
 */
static namevalue_t *add(char *name)
{
	int i;
	namevalue_t *nv;

	i = findEndOfListIndex();
	nv = (namevalue_t *) malloc(sizeof(namevalue_t));
	nv->name = strdup(name);
	nv->value_int = 0;
	nv->value_string = NULL;
	namevaluelist[i] = nv ;
	return nv ;
}

// init all
void namevaluelist_constructor()
{
	int i;
	for (i = 0; i < NAMEVALUELIST_MAX_VARS; i++)
		namevaluelist[i] = NULL;
}

int namevaluelist_get_int_value(char *name)
{
	namevalue_t *nv;
	nv = searchByName(name);
	if (nv == NULL)
		return 0; // does not exist
	else
		return nv->value_int;
}

char *namevaluelist_get_string_value(char *name)
{
	namevalue_t *nv;
	nv = searchByName(name);
	if (nv == NULL)
		return NULL; // does not exist
	else
		return nv->value_string;
}

void namevaluelist_set_int_value(char *name, int value)
{
	namevalue_t *nv;
	nv = searchByName(name);
	if (nv == NULL)
		nv = add(name) ;
	nv->value_int = value ;
}

void namevaluelist_set_string_value(char *name, char *value)
{
	namevalue_t *nv;
	nv = searchByName(name);
	if (nv == NULL)
		nv = add(name) ;
	if (nv->value_string != NULL)
		free(nv->value_string) ;
	nv->value_string = strdup(value) ;
}

