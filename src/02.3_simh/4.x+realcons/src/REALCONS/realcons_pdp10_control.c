/*  realcons_console_pd10_control.c: smarter controls than the "switch/lamp" representation of blinkenlight server.

   Copyright (c) 2014-2016, Joerg Hoppe
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

   12-Mar-2016  JH      64bit printf/scanf fmt changed to PRI*64 (inttypes.h)
      Dec-2015  JH      migration from SimH 3.82 to 4.x
                        CPU extensions moved to pdp10_cpu.c
   05-Sep-2014  JH      created

   -  * A "lamp button" control is a lamp-control combined with a switch control.
     The lamps are used for feedback.
   - different lamp feed back on button touch
   - maintains all controls in a list.

   - !!! BIT ORDER !!!
    The multi-bit controls on the panel are labeled with bit positions 0..35 (or 18..35)
    Bit 0 is left, bit 35 is right.
    Example:
    LEDs:       00..17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35
    data:        x x x  1  0  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1
    the BlinkenBone server encodes values according to bit numbers:
     Blinkenbone: 2^18 + 2^20 + 2^35 = 400005000000o = 0x800140000
    The PDP-10 numbers bit inverse! Twos exponents are
    PDP-10 2^n: 35..18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    data:        x x x  1  0  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1
    PDP-10 val: 2^17 + 2^15 + 2^0 = 500001 = 0x28001

    So there's afunction to "bit-mirror" 36bit values.
 */


#define REALCONS_PDP10_CONTROL_C_

#include <inttypes.h>
#include <assert.h>
#include "realcons.h"
#include "realcons_pdp10_control.h"
#include "bitcalc.h"

#define REALCONS_PDP10_CONTROL_MAX_COUNT	1000

/** global list, terminated with NULL ***/
realcons_pdp10_control_t *realcons_pdp10_controls[REALCONS_PDP10_CONTROL_MAX_COUNT + 1];

unsigned realcons_pdp10_controls_count = 0;

/*
 * Get both blinkenlight controls and reset the state
 */
t_stat realcons_pdp10_control_init(realcons_pdp10_control_t *_this, realcons_t *realcons,
		char *buttoncontrolname, char *lampcontrolname, unsigned mode)
{
	_this->realcons = realcons;
	_this->mode = mode;
	_this->enabled = 1; // default
	_this->buttons = NULL;
	_this->lamps = NULL;
	if (buttoncontrolname) {
		if (!(_this->buttons = realcons_console_get_input_control(realcons, buttoncontrolname)))
			return SCPE_NOATT;
		_this->buttons->value = 0;
		_this->buttons->value_previous = 0;
		_this->pendingbuttons = 0 ;
	}

	if (lampcontrolname) {
		if (!(_this->lamps = realcons_console_get_output_control(realcons, lampcontrolname)))
			return SCPE_NOATT;
		_this->lamps->value = 0;
	}

	// add control to list
	assert(realcons_pdp10_controls_count < REALCONS_PDP10_CONTROL_MAX_COUNT);
	_this->listindex = realcons_pdp10_controls_count++; // count ...
	realcons_pdp10_controls[_this->listindex] = _this;
	realcons_pdp10_controls[_this->listindex + 1] = NULL; // ... terminate also with NULL

	return SCPE_OK;
}


// get state of buttons with account of enable logic
uint64_t realcons_pdp10_control_get(realcons_pdp10_control_t *_this)
{

	if (_this->lamps)
		// return lamp status: "enable" logic already evaluated
		return _this->lamps->value;
	if (!_this->buttons)
		return 0; // no lamps, no buttons?
	if (_this->enabled)
		// no lamps, but buttons: 0 if disabled
		return _this->buttons->value;
	else
		return 0;
}

void realcons_pdp10_control_set(realcons_pdp10_control_t *_this, uint64_t value)
// trunc value to bit mask
// does NOT produce a change-event in service()!
{
	if (_this->buttons) {
		_this->buttons->value = _this->buttons->value_previous =
				(value &  BitmaskFromLen64[_this->buttons->value_bitlen]) ;
	}
	if (_this->lamps)
		_this->lamps->value =
				(value &  BitmaskFromLen64[_this->lamps->value_bitlen]) ;
}

/*
 * monitor button action and set lamps
 * result: 1 = button changed
 * BEFORE: query input controls (buttons)
 * AFTER: set output controls (lamps)
 */
unsigned realcons_pdp10_control_service(realcons_pdp10_control_t *_this)
{
	uint64_t now_pressed, now_changed, now_released;

	// scan buttons for change
	if (!_this->buttons)
		return 0; // no buttons

	// enable logic:
	// if a button gets enabled=0 while it is pressed,
	// the feedback lamp remains ON until it is released

	now_changed = (_this->buttons->value ^ _this->buttons->value_previous);
	now_pressed = now_changed & _this->buttons->value; // changed and now pressed
	now_released = now_changed & ~_this->buttons->value;

	// no initialisation call!
	if (now_changed && _this->lamps) {
		switch (_this->mode) {
		case REALCONS_PDP10_CONTROL_MODE_NONE:
			break;
		case REALCONS_PDP10_CONTROL_MODE_KEY:
			// direct: set the lamps as long button pressed
			if (_this->enabled && now_pressed) {
				_this->lamps->value = _this->buttons->value;
				_this->pendingbuttons |= now_pressed;
			}
			_this->lamps->value &= ~now_released; // lamps OFF even if not enabled
			if (_this->realcons->debug)
				printf(
						"'Direct' button changed: %s, value= 0x%" PRIx64 ", now pressed = 0x%" PRIx64 ". pending = 0x%" PRIx64 "\n",
						_this->buttons->name, _this->lamps->value, now_pressed,
						_this->pendingbuttons);

			break;
		case REALCONS_PDP10_CONTROL_MODE_SWITCH: // switched:
			// react only on PRESS event. toggle the lamps, when a button gets pressed
			// if not enabled, lamp stays ON
			if (_this->enabled && now_pressed) {
				_this->lamps->value ^= now_pressed;
				_this->pendingbuttons |= now_pressed;
			}
			if (_this->realcons->debug)
				printf(
						"'Toggle' button changed: %s, value= 0x%" PRIx64  ", now pressed = 0x%" PRIx64 ". pending = 0x%" PRIx64 "\n",
						_this->buttons->name, _this->lamps->value, now_pressed,
						_this->pendingbuttons);

			break;
		}
	}
	return 1;
}

// button changed
unsigned realcons_pdp10_button_changed(realcons_pdp10_control_t *_this)
{
	if (!_this->buttons)
		return 0;
	else
		return (_this->buttons->value != _this->buttons->value_previous);
}

