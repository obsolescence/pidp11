/* config.h: Loading and processing of config file

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


   14-Feb-2012  JH      created
*/


#ifndef CONFIG_PROCESS_H_
#define CONFIG_PROCESS_H_

#include "blinkenlight_panels.h"

#if !defined(CONFIG_C_)
// globals for use by ANTLR parser
extern blinkenlight_panel_t *parser_cur_panel; // current panel
extern blinkenlight_control_t *parser_cur_control; // current control ;
extern blinkenlight_control_blinkenbus_register_wiring_t *parser_cur_register_wiring;
#endif

char *parser_strip_quotes(char *parsed_string); // make "\"xxx\"" -> "xxx"
blinkenlight_control_blinkenbus_register_wiring_t *parser_add_register_wiring(
		blinkenlight_control_t *c);

// all function operate on the global static "blinkenlight_panel_list"
void blinkenlight_panels_config_load(char *filename);
int blinkenlight_panels_config_check(void);

#endif /* CONFIG_PROCESS_H_ */
