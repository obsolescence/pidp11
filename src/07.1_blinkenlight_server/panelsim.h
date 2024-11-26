/* panelsim.h: simulated panel implemented with interactive GUI

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


   17-Mar-2012  JH      created
*/


#ifndef PANELSIM_H_
#define PANELSIM_H_

#include <stdio.h>
#include "blinkenlight_panels.h"

void panelsim_init(int i_panel);
void panelsim_service(void);

void panelsim_read_panel_input_controls(blinkenlight_panel_t *p);
void panelsim_write_panel_output_controls(blinkenlight_panel_t *p, int force_all);
int panelsim_get_blinkenboards_state(blinkenlight_panel_t *p);
void panelsim_set_blinkenboards_state(blinkenlight_panel_t *p, int new_state);

#endif /* PANELSIM_H_ */
