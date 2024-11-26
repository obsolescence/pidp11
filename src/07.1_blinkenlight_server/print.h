/* print.h: log file printer

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


   10-Feb-2012  JH      created
*/

#ifndef LOGGING_H_
#define LOGGING_H_

#include <stdio.h>
#ifdef WIN32
// no syslogging option under Windows. levels still needed:
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */
#else
#include <syslog.h>
#endif
// use these message levels:
// 	LOG_EMERG	system is unusable
//	LOG_ALERT	action must be taken immediately
//	LOG_CRIT	critical conditions
// 	LOG_ERR		error conditions
//	LOG_WARNING	warning conditions
//	LOG_NOTICE	normal, but significant, condition
//	LOG_INFO	informational message
//	LOG_DEBUG	debug-level message

#if !defined(LOGGING_C_)
extern int print_level; // print call of remote procedures
#endif

/*
 * Output into stderr or logfile
 */
void print_open(int use_syslog) ;
void print(int level, const char* format, ...);
void print_memdump(int level, char *info, unsigned start_addr, unsigned count, unsigned char *data) ;
void print_close(void) ;

#endif /* LOGGING_H_ */
