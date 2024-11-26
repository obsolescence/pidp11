/* print.c: log file printer

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


#define LOGGING_C_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>	// struct timeval
#else
#include <syslog.h>
#include <sys/time.h>
#endif
#include <time.h>

#include "print.h"

int print_level; // global level to log

static int print_syslog = 0; // 1, wenn nach syslog geschrieben wird. Sonst stderr.

void print_open(int use_syslog)
{
#ifndef WIN32
	print_syslog = use_syslog;
	if (print_syslog) {
		openlog("blinkenlightd", LOG_CONS, LOG_USER);
	}
#endif
}

void print_close(void)
{
#ifndef WIN32
	if (print_syslog) {
		closelog();
	}
#endif
}

/*
 * Output into stderr or logfile
 */
void print(int level, const char* format, ...)
{
	va_list argptr;
	struct timeval tv;
	struct tm *tm;
	char buffer[256];
	va_start(argptr, format);
	if (level <= print_level) {
#ifndef WIN32
		if (print_syslog) {
			vsyslog(level, format, argptr);
		} else
#endif
		{
			gettimeofday(&tv, NULL );
			tm = gmtime(&(tv.tv_sec));
			// print with 10th of secs
			fprintf(stderr, "[%d:%02d:%02d.%03ld] ", tm->tm_hour, tm->tm_min, tm->tm_sec,
					tv.tv_usec / 1000);
			vfprintf(stderr, format, argptr);
			va_end(argptr);
		}
	}
}

/*
 * print a range of bytes, localated at some start address
 */
void print_memdump(int level, char *info, unsigned start_addr, unsigned count, unsigned char *data)
{
	char buff[1000], buff1[100];
	unsigned i;
	int  buff_empty ;
	buff[0] = 0;
	if (info)
		strcat(buff, info);
	sprintf(buff1, "start=0x%04x, data[0..%2u]=", start_addr, count - 1);
	strcat(buff, buff1);
	buff_empty = 0;
	// break every 16 bytes
	for (i = 0; i < count; i++) {
		if (i && (i % 16 == 0)) {
			print(level, "%s\n", buff);
			strcpy(buff, "        ");
		    buff_empty = 1;
		}
		if (i % 16 == 8)
		    strcat(buff, " ") ; // mark every 8
		sprintf(buff1, "%02x ", (unsigned) data[i]);
		strcat(buff, buff1);
        buff_empty = 0;
	}
	if (!buff_empty) // something left in buffer
		print(level, "%s\n", buff);
}

