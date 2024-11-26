/* getopt2.c:  advanced commandline parsing

  Copyright (c) 2016, Joerg Hoppe
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

  20-Jul-2016  JH	added defaults for options
  17-Mar-2016  JH	allow "/" option marker only #ifdef WIN32
  01-Feb-2016  JH   created


  Adavanced getopt(), parses command lines,

  Argument pattern

	 commandline = [option, option, ...]  args ....
	 option = ( "-" | "/"  ) ( short_option_name | long_option_name )
			   [fix_arg fix_arg .... [ var_arg var_arg ]]


   API
   getopt_init()  - init data after start, only once
   getopt_def(id, short, long, fix_args, opt_args, info)
	   define a possible option
	   fix_args, opt_args: comma separated names


   getopt_first(argc, argv) parse and return arg of first option
	   result = id
	   value for args in static "argval" (NULL temriantd)

   getopt_next()	- ge



   Example
   Cmdline syntax: "-send id len [data .. data]   \
					   -flag \
					   -logfile logfile \
					   myfile \

	   getopt_init() ;
	   getopt_def("s", "send", "id,len", "data0,data1,data2,data3,data4,data5,data6,data7") ;
	   getopt_def("flag", NULL, NULL, NULL) ;
	   getopt_def("l", "logfile", "logfile", NULL) ;

	   res = getopt_first(argc, argv) ;
	   while (res > 0) {
			   if (getopt_is("send")) {
				// process "send" option with argval[0]= id, argval[1] = len, argval[2]= data0 ...
			   } else if (getopt_is("flag")) {
			   // process flag
	   } else if (getopt_is("logfile")) {
			   // process logfile, name = argval[0]
	   } else if (getopt_is(NULL)) {
			   // non-option commandline arguments in argval[]
	   }
   }
   if (res < 0) {
	   printf("Cmdline syntax error at ", curtoken) ;
	   getopt_help(stdout) ;
	   exit(1) ;
  }

  // res == 0: all OK, go on
  ...

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include "getopt2.h"

#ifdef WIN32
#include <windows.h>
#define strcasecmp _stricmp	// grmbl
#define snprintf  sprintf_s
#else
#include <ctype.h>
#include <unistd.h>
#endif

/*
 * first intialize, only once!
 * NO FREE() HERE !
 */


void getopt_init(getopt_t *_this, int ignore_case)
{
	_this->ignore_case = ignore_case;
	// clear list
	_this->nonoption_descr = NULL;
	_this->option_descrs[0] = NULL;
	_this->cur_option_argval[0] = NULL;
	_this->curerror = GETOPT_STATUS_OK;
	_this->curerrortext[0] = 0;
}

/* compare string with regard to "ignore_case*/
static int getopt_strcmp(getopt_t *_this, char *s1, char *s2) {
	if (_this->ignore_case)
		return strcasecmp(s1, s2);
	else
		return strcmp(s1, s2);
}

/*
  fix_args, opt_args: like  "name1,name2,name2"
  if short/long optionname = NULL
  its the definition of the non-option commandline arguments
*/
getopt_option_descr_t	*getopt_def(getopt_t *_this,
	char	*short_option_name,
	char	*long_option_name,
	char	*fix_args_csv,
	char	*opt_args_csv,
	char	*default_args,
	char	*info,
	char	*example_simple_cline, char *example_simple_info,
	char	*example_complex_cline, char *example_complex_info
	)
{
	getopt_option_descr_t	*res;
	unsigned	i, argc;
	char	*rp; // read pointer

	if (short_option_name == NULL && long_option_name == NULL) {
		// non-option commandline arg
		res = _this->nonoption_descr = (getopt_option_descr_t *)malloc(sizeof(getopt_option_descr_t));
	}
	else {
		// instantiate new option descr
		res = (getopt_option_descr_t *)malloc(sizeof(getopt_option_descr_t));
		// add this option description to global list
		for (i = 0; _this->option_descrs[i]; i++); // find end of list
		assert(i < GETOPT_MAX_OPTION_DESCR);
		_this->option_descrs[i++] = res;
		_this->option_descrs[i] = NULL;
	}
	res->short_name = short_option_name;
	res->long_name = long_option_name;
	res->default_args = default_args;
	res->info = info;
	res->example_simple_cline_args = example_simple_cline;
	res->example_simple_info = example_simple_info;
	res->example_complex_cline_args = example_complex_cline;
	res->example_complex_info = example_complex_info;

	res->fix_args[0] = NULL;
	res->var_args[0] = NULL;
	res->fix_arg_count = 0;
	res->max_arg_count = 0;

	// separate name lists

	// get own copies of argument name list
	if (fix_args_csv && strlen(fix_args_csv))
		res->fix_args_name_buff = strdup(fix_args_csv);
	else res->fix_args_name_buff = NULL;

	if (opt_args_csv && strlen(opt_args_csv))
		res->var_args_name_buff = strdup(opt_args_csv);
	else res->var_args_name_buff = NULL;

	// separate
	// i counts chars, argc counts arguments
	rp = res->fix_args_name_buff;
	argc = 0;
	while (rp && *rp) {
		res->fix_args[argc++] = rp; 		// start of arg name
		res->fix_args[argc] = NULL;
		assert(argc < GETOPT_MAX_OPTION_ARGS);
		while (*rp && *rp != ',') rp++; // skip arg name
		while (*rp && *rp == ',') *rp++ = 0; // terminate arg name
	}
	res->fix_arg_count = argc;

	// same for opt args
	rp = res->var_args_name_buff;
	argc = 0;
	while (rp && *rp) {
		res->var_args[argc++] = rp; 		// start of arg name
		res->var_args[argc] = NULL;
		assert(argc < GETOPT_MAX_OPTION_ARGS);
		while (*rp && *rp != ',') rp++; // skip arg name
		while (*rp && *rp == ',') *rp++ = 0; // terminate arg name
	}
	res->max_arg_count = res->fix_arg_count + argc;

	return res;
}




/*
 is the last parsed option the one with short or longname "name" ?
 Use for switch-processing
*/
int getopt_isoption(getopt_t *_this, char *name)
{
	if (name == NULL && _this->cur_option == _this->nonoption_descr)
		return 1; // were parsing the non-option args
	if (!_this->cur_option)
		return 0; // not equal
	if (!name) // nonoption args already tested
		return 0;
	// names NULL for nonoption-args
	if (_this->cur_option->short_name && !getopt_strcmp(_this, name, _this->cur_option->short_name))
		return 1;
	if (_this->cur_option->long_name && !getopt_strcmp(_this, name, _this->cur_option->long_name))
		return 1;
	return 0;
}


/*
 * getopt_first()	initialize commandline parser and return 1st option with args
 * getopt_next()	returns netx option from command line with its arguments

 *			list of parsed args in _this->argval

 *	result: > 0: OK
 *		 0 = GETOPT_STATUS_EOF		 cline processed
 *		other status codes <= 0
 *		GETOPT_STATUS_ARGS		no option returned, but non-option cline args
 *		GETOPT_STATUS_UNDEFINED	undefined -option
 *		GETOPT_STATUS_ARGS		not enough args for -option
 *
 */

 // if clinearg is -name or --name or /name: return name, else NULL
static char *get_dashed_option_name(char *clinearg) {
	char *res = NULL;
	if (!strncmp(clinearg, "--", 2))
		res = clinearg + 2;
	else if (!strncmp(clinearg, "-", 1))
		res = clinearg + 1;
#ifdef WIN32
	else if (!strncmp(clinearg, "/", 1))
		res = clinearg + 1;
#endif
	return res;
}

static int getopt_parse_error(getopt_t *_this, int error)
{
	_this->curerror = error;
	switch (error) {
	case GETOPT_STATUS_ILLEGALOPTION:
		snprintf(_this->curerrortext, sizeof(_this->curerrortext),
			"Undefined option at \"%s\"", _this->curtoken);
		break;
	case GETOPT_STATUS_MINARGCOUNT:
		if (_this->cur_option == _this->nonoption_descr)
			snprintf(_this->curerrortext, sizeof(_this->curerrortext),
				"Less than %d non-option arguments at \"%s\"",
				_this->cur_option->fix_arg_count, _this->curtoken);
		else
			snprintf(_this->curerrortext, sizeof(_this->curerrortext),
				"Less than %d arguments for option \"%s\" at \"%s\"",
				_this->cur_option->fix_arg_count, _this->cur_option->long_name, _this->curtoken);
		break;
	case GETOPT_STATUS_MAXARGCOUNT:
		if (_this->cur_option == _this->nonoption_descr)
			snprintf(_this->curerrortext, sizeof(_this->curerrortext),
				"More than %d non-option arguments at \"%s\"", _this->cur_option->max_arg_count, _this->curtoken);
		else
			snprintf(_this->curerrortext, sizeof(_this->curerrortext),
				"More than %d arguments for option \"%s\" at \"%s\"",
				_this->cur_option->max_arg_count, _this->cur_option->long_name, _this->curtoken);
		break;
	}
	return error;
}

static int getopt_arg_error(getopt_t *_this, getopt_option_descr_t *odesc, int error, char *argname, char *argval) {
	_this->curerror = error;
	switch (error) {
	case GETOPT_STATUS_ILLEGALARG:
		snprintf(_this->curerrortext, sizeof(_this->curerrortext),
			"Option \"%s\" has no argument \"%s\"", odesc->long_name, argname);
		break;
	case GETOPT_STATUS_ARGNOTSET:
		snprintf(_this->curerrortext, sizeof(_this->curerrortext),
			"Optional argument \"%s\" for option \"%s\" not set", argname, odesc->long_name);
		break;
	case GETOPT_STATUS_ARGFORMATINT:
		snprintf(_this->curerrortext, sizeof(_this->curerrortext),
			"Argument \"%s\" of option \"%s\" has value \"%s\", which is no integer", argname, odesc->long_name, argval);
		break;
	case GETOPT_STATUS_ARGFORMATHEX:
		snprintf(_this->curerrortext, sizeof(_this->curerrortext),
			"Argument \"%s\" of option \"%s\" has value \"%s\", which is no hex integer", argname, odesc->long_name, argval);
		break;
	}
	return error;
}


int getopt_next(getopt_t *_this)
{
	char	*s, *oname;
	getopt_option_descr_t *odesc;
	int	i;
	int max_scan_arg_count;

	// is it an option?
	if (_this->cur_cline_arg_idx >= _this->argc)
		return GETOPT_STATUS_EOF;

	_this->curtoken = s = _this->argv[_this->cur_cline_arg_idx];
	assert(s);
	assert(*s); // must be non empty



	oname = get_dashed_option_name(s);
	if (oname) {
		// it is an "-option": search options by name
		_this->cur_option = NULL;
		for (i = 0; odesc = _this->option_descrs[i]; i++)
			if (!getopt_strcmp(_this, oname, odesc->short_name) || !getopt_strcmp(_this, oname, odesc->long_name))
				_this->cur_option = odesc; // found by name
		if (_this->cur_option == NULL) {
			// its an option, but not found in the definitions
			return getopt_parse_error(_this, GETOPT_STATUS_ILLEGALOPTION);
		}
		_this->cur_cline_arg_idx++; // skip -option

		// if an option has no optional arguments, prevent it from
		// parsing into
	}
	else // its not an '-option: so its the "nonoption" rest of cline
		_this->cur_option = _this->nonoption_descr;

	// find the arg at wich to stop parsing args for this option
	// in case of nonoption-args: end of cmdlineor
	// else for options:
	//	it's either the next -option,
	//  or if no -option: collision with trailing non-option args.
	//	   if no variable: amount of fix args
	//		else: line end. So in case
	//		-option fix0 fix1 var0 var 1 nonopt0 nonopt1
	//			all also nonopt0/1 is read, resulting in an error
	//			this is intended to force unambiguous syntax declaration!
	if (_this->cur_option == _this->nonoption_descr)
		max_scan_arg_count = INT_MAX;
	else {
		// search next -option
		i = _this->cur_cline_arg_idx;
		while (i < _this->argc &&  get_dashed_option_name(_this->argv[i]) == NULL)
			i++;
		if (i < _this->argc) // terminating -option found
			max_scan_arg_count = i - _this->cur_cline_arg_idx;
		else if (_this->cur_option->fix_arg_count == _this->cur_option->max_arg_count)
			max_scan_arg_count = _this->cur_option->fix_arg_count;
		else max_scan_arg_count = INT_MAX;
	}

	// option (or rest of cline) valid, parse args
	// parse until
	// - end of commandline
	// - max argument count for option reached
	// - another "-option" is found
	i = 0;
	while (_this->cur_cline_arg_idx < _this->argc
		&& i < max_scan_arg_count) {
		_this->cur_option_argval[i++] = _this->curtoken = _this->argv[_this->cur_cline_arg_idx];
		_this->cur_option_argval[i] = NULL;
		assert(i < GETOPT_MAX_OPTION_ARGS);
		_this->cur_option_argvalcount = i;
		_this->cur_cline_arg_idx++;
	}

	if (i < _this->cur_option->fix_arg_count)
		return getopt_parse_error(_this, GETOPT_STATUS_MINARGCOUNT);
	if (i > _this->cur_option->max_arg_count)
		return getopt_parse_error(_this, GETOPT_STATUS_MAXARGCOUNT);
	return GETOPT_STATUS_OK;
}



int getopt_first(getopt_t *_this, int argc, char **argv)
{
	int	i;
	char	*cb = _this->default_cmdline_buff; // alias 
	getopt_option_descr_t *odesc;
	char	*token;

	// for all options with "default" args: 
	// put strings <option> <args> in front of commandline,
	// so they are parsed first and overwritten later by actual values
	// 1) build own cmdline
	*cb = 0; // clear
	for (i = 0; odesc = _this->option_descrs[i]; i++)
		if (odesc->default_args && strlen(odesc->default_args)) {
			strcat(cb, "--");
			strcat(cb, odesc->long_name);
			strcat(cb, " ");
			strcat(cb, odesc->default_args);
			strcat(cb, " ");
		}
	// 2) separate into words and add to argv
	// Input in cb like : char space char char space space char char char space 0
	token = cb;
	while (*token) {
		// last token terminated with 0.
		// clear white space before next token
		while (*token && isspace(*token)) *token++ = 0;
		if (*token) {
			// add to arg list
			assert(_this->argc < GETOPT_MAX_CMDLINE_TOKEN);
			_this->argv[_this->argc++] = token;
			// skip and terminate token
			while (*token && !isspace(*token)) token++;
			if (*token) // convert terminating space to 0
				*token++ = 0;
		}
	}

	// 3) append user commandline tokens, so they are processed after defaults
	for (i = 1; i < argc; i++) { // skip program name of 
		assert(_this->argc < GETOPT_MAX_CMDLINE_TOKEN);
		_this->argv[_this->argc++] = argv[i];
	}

	_this->cur_cline_arg_idx = 0;
	_this->cur_option = NULL;
	_this->cur_option_argval[0] = NULL;
	_this->cur_option_argvalcount = 0;

	return getopt_next(_this);
}


// find argument position in list by name.
// optargs[] are numbered behind fixargs[]
// < 0: not found
static int getopt_optionargidx(getopt_t *_this, getopt_option_descr_t  *odesc, char *argname)
{
	int  i;
	for (i = 0; i < odesc->fix_arg_count; i++)
		if (!getopt_strcmp(_this, argname, odesc->fix_args[i]))
			return i;
	for (i = odesc->fix_arg_count; i < odesc->max_arg_count; i++)
		if (!getopt_strcmp(_this, argname, odesc->var_args[i - odesc->fix_arg_count]))
			return i;
	return getopt_arg_error(_this, odesc, GETOPT_STATUS_ARGNOTSET, argname, NULL);
}



// argument of current option by name as string
// Only to be used after first() or next()
// result: < 0 = error, 0 = arg not set
int getopt_arg_s(getopt_t *_this, char *argname, char *val, unsigned valsize)
{
	int argidx;
	getopt_option_descr_t  *odesc = _this->cur_option;
	if (!odesc)
		return getopt_parse_error(_this, GETOPT_STATUS_ILLEGALOPTION);
	argidx = getopt_optionargidx(_this, odesc, argname);
	if (argidx < 0)
		return getopt_arg_error(_this, odesc, GETOPT_STATUS_ILLEGALARG, argname, NULL);
	if (argidx >= (int)_this->cur_option_argvalcount)
		// only n args specified, but this has list place > n
		// the optional argument [argidx] is not given in the arguument list
		return GETOPT_STATUS_EOF;
	//		return getopt_arg_error(_this, odesc, GETOPT_STATUS_ARGNOTSET, argname, NULL);
	strncpy(val, _this->cur_option_argval[argidx], valsize);
	val[valsize - 1] = 0;
	return GETOPT_STATUS_OK;
}

// argument of current option by name as  integer, with optional prefixes "0x" or "0".
// result: < 0 = error, 0 = arg not set
int getopt_arg_i(getopt_t *_this, char *argname, int *val)
{
	int res;
	char *endptr;
	char buff[GETOPT_MAX_LINELEN + 1];
	res = getopt_arg_s(_this, argname, buff, sizeof(buff));
	if (res <= 0) // error or EOF
		return res;
	*val = strtol(buff, &endptr, 0);
	if (*endptr) // stop char: error
		return getopt_arg_error(_this, _this->cur_option, GETOPT_STATUS_ARGFORMATINT, argname, buff);
	return GETOPT_STATUS_OK;
}

// argument of current option by name as unsigned integer, with optional prefixes "0x" or "0".
// result: < 0 = error, 0 = arg not set
int getopt_arg_u(getopt_t *_this, char *argname, unsigned *val)
{
	int res;
	char *endptr;
	char buff[GETOPT_MAX_LINELEN + 1];
	res = getopt_arg_s(_this, argname, buff, sizeof(buff));
	if (res <= 0) // error or EOF
		return res;
	*val = strtoul(buff, &endptr, 0);
	if (*endptr) // stop char: error
		return getopt_arg_error(_this, _this->cur_option, GETOPT_STATUS_ARGFORMATINT, argname, buff);
	return GETOPT_STATUS_OK;
}

// argument of current option by name as hex integer. No prefix "0x" allowed!
// result: < 0 = error, 0 = arg not set
int getopt_arg_h(getopt_t *_this, char *argname, int *val)
{
	int res;
	char *endptr;
	char buff[GETOPT_MAX_LINELEN + 1];
	res = getopt_arg_s(_this, argname, buff, sizeof(buff));
	if (res <= 0) // error or EOF
		return res;
	*val = strtol(buff, &endptr, 16);
	if (*endptr) // stop char: error
		return getopt_arg_error(_this, _this->cur_option, GETOPT_STATUS_ARGFORMATHEX, argname, buff);
	return GETOPT_STATUS_OK;
}



/* printhelp()
 * write the syntax and explanation out
*/

// add as string to outline. if overflow, flush an continue with indent
// "stream": must be defined in call context
static void output_append(FILE *stream, char *line, int linesize, char *s, int linebreak, unsigned linelen, unsigned indent)
{
	unsigned _i_;
	if (linebreak
		|| (strlen(line) > indent && (strlen(line) + strlen(s)) > linelen)) {
		// prevent the indent from prev line to be accounted for another line break
		fprintf(stream, "%s\n", line); \
			for (_i_ = 0; _i_ < indent; _i_++) line[_i_] = ' ';
		line[_i_] = 0;
	}
	strncat(line, s, linesize);
	line[linesize - 1] = 0;
}



// generate a string like "-option <arg1> <args> [<optarg>]"
// style 0: only --long_name, or (shortname)
// tsyle 1: "-short, --long .... "
static char *getopt_getoptionsyntax(getopt_option_descr_t *odesc, int style)
//	getopt_t *_this, getopt_option_descr_t *odesc)
{
	static char buffer[2 * GETOPT_MAX_LINELEN];
	unsigned i;
	char *s;

	// mount a single "-option arg arg [arg arg]"
	if (style == 0) {
		if (odesc->long_name)
			snprintf(buffer, sizeof(buffer), "--%s", odesc->long_name);
		else if (odesc->short_name)
			snprintf(buffer, sizeof(buffer), "-%s", odesc->short_name);
		else  buffer[0] = 0; // no option name: non-optopn commadnline arguments
	}
	else { // both names comma separated: "-short, --long"
		buffer[0] = 0;
		if (odesc->short_name) {
			strncat(buffer, "-", sizeof(buffer) - 1);
			strncat(buffer, odesc->short_name, sizeof(buffer) - 1);
		}
		if (odesc->long_name) {
			if (odesc->short_name)
				strncat(buffer, " | ", sizeof(buffer) - 1);
			strncat(buffer, "--", sizeof(buffer) - 1);
			strncat(buffer, odesc->long_name, sizeof(buffer) - 1);
		}
	}

	for (i = 0; s = odesc->fix_args[i]; i++) {
		strncat(buffer, " <", sizeof(buffer) - 1);
		strncat(buffer, s, sizeof(buffer) - 1);
		strncat(buffer, ">", sizeof(buffer) - 1);
	}
	for (i = 0; s = odesc->var_args[i]; i++) {
		strncat(buffer, " ", sizeof(buffer) - 1);
		if (i == 0) strncat(buffer, "[", sizeof(buffer) - 1);
		strncat(buffer, "<", sizeof(buffer) - 1);
		strncat(buffer, s, sizeof(buffer) - 1);
		strncat(buffer, ">", sizeof(buffer) - 1);
	}
	if (i > 0) strncat(buffer, "]", sizeof(buffer) - 1);

	return buffer;
}

/* print a multine string separated by \n, with indent and line break
 * lienebuff may already contain some text */
static void getopt_print_multilinestring(FILE *stream, char *linebuff, unsigned linebuffsize, char *text, unsigned linelen, unsigned indent)
{
	char *s_text;
	char *s_start, *s;

	s_start = s_text = strdup(text); // print multiline info. make writeable copy
	while (*s_start) {
		s = s_start;
		// seach end of substring
		while (*s && *s != '\n') s++;
		if (*s) { // substr seperator
			*s = '\0'; // terminate

			output_append(stream, linebuff, linebuffsize, s_start, /*linebreak*/s_start != s_text, linelen, indent);
			s_start = s + 1; // advance to start of next substr
		}
		else { // last substr
			output_append(stream, linebuff, linebuffsize, s_start, /*linebreak*/s_start != s_text, linelen, indent);
			s_start = s; // stop
		}
	}
	free(s_text);
}


static void getopt_help_option_intern(getopt_option_descr_t *odesc, FILE *stream, unsigned linelen, unsigned indent)
{
	char linebuff[2 * GETOPT_MAX_LINELEN];
	char phrase[2 * GETOPT_MAX_LINELEN];
	linebuff[0] = 0;

	// print syntax
	strncpy(phrase, getopt_getoptionsyntax(odesc, 1), sizeof(phrase));
	output_append(stream, linebuff, sizeof(linebuff), phrase, /*linebreak*/0, linelen, indent);
	output_append(stream, linebuff, sizeof(linebuff), "", /*linebreak*/1, linelen, indent); // newline
	if (odesc->info)
		getopt_print_multilinestring(stream, linebuff, sizeof(linebuff), odesc->info, linelen, indent);
	if (odesc->default_args) {
		snprintf(phrase, sizeof(phrase), " Default: \"%s\"", odesc->default_args);
		output_append(stream, linebuff, sizeof(linebuff), phrase, /*linebreak*/0, linelen, indent);
	}

	// print examples:
	if (odesc->example_simple_cline_args) {
		output_append(stream, linebuff, sizeof(linebuff), "Simple example:  ", 1, linelen, indent);
		if (odesc->short_name) {
			output_append(stream, linebuff, sizeof(linebuff), "-", 0, linelen, indent);
			output_append(stream, linebuff, sizeof(linebuff), odesc->short_name, 0, linelen, indent);
			output_append(stream, linebuff, sizeof(linebuff), " ", 0, linelen, indent);
		}
		getopt_print_multilinestring(stream, linebuff, sizeof(linebuff), odesc->example_simple_cline_args, linelen, indent + 4);
		output_append(stream, linebuff, sizeof(linebuff), "    ", /*linebreak*/1, linelen, indent); // newline + extra indent
//		output_append(stream, linebuff, sizeof(linebuff), "", /*linebreak*/1, linelen, indent); // newline + extra indent
		getopt_print_multilinestring(stream, linebuff, sizeof(linebuff), odesc->example_simple_info, linelen, indent + 4);
	}
	if (odesc->example_complex_cline_args) {
		output_append(stream, linebuff, sizeof(linebuff), "Complex example:  ", 1, linelen, indent);
		if (odesc->long_name) {
			output_append(stream, linebuff, sizeof(linebuff), "--", 0, linelen, indent);
			output_append(stream, linebuff, sizeof(linebuff), odesc->long_name, 0, linelen, indent);
			output_append(stream, linebuff, sizeof(linebuff), " ", 0, linelen, indent);
		}
		getopt_print_multilinestring(stream, linebuff, sizeof(linebuff), odesc->example_complex_cline_args, linelen, indent + 4);
		//		output_append(stream, linebuff, sizeof(linebuff), "", /*linebreak*/1, linelen, indent); // newline + extra indent
		output_append(stream, linebuff, sizeof(linebuff), "    ", /*linebreak*/1, linelen, indent); // newline + extra indent
		getopt_print_multilinestring(stream, linebuff, sizeof(linebuff), odesc->example_complex_info, linelen, indent + 4);
	}
	// flush
	fprintf(stream, "%s\n", linebuff);
}

// print cmdline syntax and help for all options
void getopt_help(getopt_t *_this, FILE *stream, unsigned linelen, unsigned indent, char *commandname)
{
	unsigned i;
	char linebuff[2 * GETOPT_MAX_LINELEN];
	char phrase[2 * GETOPT_MAX_LINELEN];
	getopt_option_descr_t *odesc;
	assert(linelen < GETOPT_MAX_LINELEN);

	// 1. print commandline summary
	linebuff[0] = 0;
	phrase[0] = 0;
	// 1.1. print options
	//fprintf(stream, "Command line summary:\n");
	snprintf(linebuff, sizeof(linebuff), "%s ", commandname);
	for (i = 0; odesc = _this->option_descrs[i]; i++) {
		// mount a single "-option arg arg [arg arg]"
		strncpy(phrase, getopt_getoptionsyntax(odesc, 0), sizeof(phrase));
		strncat(linebuff, " ", sizeof(linebuff));
		output_append(stream, linebuff, sizeof(linebuff), phrase, /*linebreak*/0, linelen, indent);
	}
	// 1.2. print non-option cline arguments
	if (_this->nonoption_descr) {
		strncpy(phrase, getopt_getoptionsyntax(_this->nonoption_descr, 0), sizeof(phrase));
		strncat(linebuff, " ", sizeof(linebuff));
		output_append(stream, linebuff, sizeof(linebuff), phrase, /*linebreak*/0, linelen, indent);
	}

	fprintf(stream, "%s\n", linebuff);

	// 2. print option info
	fprintf(stream, "\n");
	// fprintf(stream, "Command line options:\n");

	// first: nonoption arguments
	if (_this->nonoption_descr)
		getopt_help_option_intern(_this->nonoption_descr, stream, linelen, indent);
	// now options
	for (i = 0; odesc = _this->option_descrs[i]; i++)
		getopt_help_option_intern(_this->option_descrs[i], stream, linelen, indent);

	if (_this->ignore_case)
		fprintf(stream, "\nOption names are case insensitive.\n");
	else
		fprintf(stream, "\nOption names are case sensitive.\n");
}


// display evaluated commandline (defaults and user)
void getopt_help_commandline(getopt_t *_this, FILE *stream, unsigned linelen, unsigned indent)
{
	int	i;
	char linebuff[2 * GETOPT_MAX_LINELEN];
	char phrase[2 * GETOPT_MAX_LINELEN];
	linebuff[0] = 0;
	for (i = 0; i < _this->argc; i++) {
		if (i == 0)
			snprintf(phrase, sizeof(phrase), "\"%s\"", _this->argv[i]);
		else
			snprintf(phrase, sizeof(phrase), " \"%s\"", _this->argv[i]);
		output_append(stream, linebuff, sizeof(linebuff), phrase, /*linebreak*/0, linelen, indent);
	}

	fprintf(stream, "%s\n", linebuff);
}




// print help for current option
void getopt_help_option(getopt_t *_this, FILE *stream, unsigned linelen, unsigned indent)
{
	getopt_option_descr_t  *odesc = _this->cur_option;
	if (odesc)
		getopt_help_option_intern(odesc, stream, linelen, indent);
}

