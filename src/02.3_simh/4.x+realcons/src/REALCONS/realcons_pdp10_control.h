/*  realcons_console_pd10_control.h: smarter controls than the "switch/lamp" representation of blinkenlight server.

   Copyright (c) 2014-2016, Joerg Hoppe
   j_hoppe@t-online.de, www.retrocmp.com

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   JOERG HOPPE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

      Dec-2015  JH      migration from SimH 3.82 to 4.x
                        CPU extensions moved to pdp10_cpu.c
   05-Sep-2014  JH      created
*/


#ifndef REALCONS_PDP10_CONTROL_H_
#define REALCONS_PDP10_CONTROL_H_

#include "realcons.h"

#define REALCONS_PDP10_CONTROL_MODE_NONE	0

// just a lamp without button
#define REALCONS_PDP10_CONTROL_MODE_OUTPUT	1
// a input control without lamps
#define REALCONS_PDP10_CONTROL_MODE_INPUT	2

// lamp only ON if button pressed: "momentary action"
#define REALCONS_PDP10_CONTROL_MODE_KEY	3
// lamp toggles if button pressed:
#define REALCONS_PDP10_CONTROL_MODE_SWITCH	4

/*
 * Describes a button combined with a lamp
 * These are two different blinkenlight controls!
 */
typedef struct
{
	blinkenlight_control_t *buttons;
	blinkenlight_control_t *lamps;

	unsigned mode; // one of REALCONS_CONSOLE_LAMPBUTTON_MODE_*

	unsigned enabled; // reacts button on keypress?
	// (console_lock, data_lock)
	struct realcons_struct *realcons; // link to parent

	// TODO: further ideas
	uint64_t pendingbuttons; // latched buttons, which not have been processed

	unsigned listindex; // index in list of controls

} realcons_pdp10_control_t;

t_stat realcons_pdp10_control_init(realcons_pdp10_control_t *_this,
		struct realcons_struct *realcons, char *buttoncontrolname, char *lampcontrolname,
		unsigned mode);

#ifndef REALCONS_PDP10_CONTROL_C_
extern unsigned realcons_pdp10_controls_count;
extern realcons_pdp10_control_t *realcons_pdp10_controls[];
#endif

uint64_t realcons_pdp10_control_get(realcons_pdp10_control_t *_this);
void realcons_pdp10_control_set(realcons_pdp10_control_t *_this, uint64_t value);

// void realcons_console_lampbutton_connect_signal(realcons_pdp10_control_t *_this);

unsigned realcons_pdp10_control_service(realcons_pdp10_control_t *_this);

unsigned realcons_pdp10_button_changed(realcons_pdp10_control_t *_this);

#endif /* REALCONS_CONSOLE_PDP10_CONTROL_H_ */
