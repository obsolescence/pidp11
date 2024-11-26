/* main.h: RPC Server for BlinkenBus panel hardware interface.

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


   08-Feb-2012  JH      created
*/

#ifndef MAIN_H_
#define MAIN_H_

#include "blinkenlight_panels.h"

#define MAX_FILENAME_LEN	1024

#ifndef MAIN_C_
// global panel config
extern blinkenlight_panel_list_t *blinkenlight_panel_list ;

extern char configfilename[MAX_FILENAME_LEN];
// global flags
extern int mode_test; // no server, just test config
extern int mode_panelsim; // do not access BLINKENBUS, no daemon, user oeprates simualted panel

//extern int stmode; // no server, just test config
extern char program_info[];
extern char program_name[]; // argv[0]
extern char program_options[]; // argv[1.argc-1]

#endif

#endif /* MAIN_H_ */
